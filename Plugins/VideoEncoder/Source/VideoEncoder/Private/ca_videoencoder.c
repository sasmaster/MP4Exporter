#include "ca_videoencoder.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>

#include <assert.h>

struct VideoEncoderContext
{
	AVCodec*            mCodec;
	AVCodec*            mAudioCodec;
	AVCodecContext*     mCodecContext;
	AVCodecContext*     mAudioCodecContext;
	AVOutputFormat*     mOformat;
	AVFormatContext*    mOFormatContext;
	AVFormatContext*    mAudioInputContext;
	AVFrame*            mVideoFrame;
	AVStream*           mVideoStream;
	AVStream*           mAudioInputStream;
	AVStream*           mAudioOutputStream;
	struct SwsContext*  mSwsCtx;
	AVPacket*           mPacket;
	AVPacket*           mAudioPacket;
	uint64_t            mFrameCount;
	int64_t             next_pts;
	int64_t             next_pts_audio;
	bool                mFlipVertically;
	double              mAUdioTimeLimit;//stop encoding audio stream after this time

};


static bool InitAudio(VideoEncoderContext* encoder, const char* filePath, uint32_t audoDurationLimit)
{

	encoder->mAUdioTimeLimit = (audoDurationLimit / (double)encoder->mCodecContext->framerate.num); //make it external param to allow stop audio encode at specific time

	AVOutputFormat* outputF = encoder->mOFormatContext->oformat;
	enum AVCodecID codecId = outputF->audio_codec;
	if (codecId == AV_CODEC_ID_NONE)
	{
		return false;
	}
	// find audio encoder
	encoder->mAudioCodec = avcodec_find_encoder(codecId);
	if (!encoder->mAudioCodec)
	{
		return false;
	}
	//relative paths cause the next method call to return with file not found error.
	// and it happens only when I load external 3d mode. Weird..

	int errcode = avformat_open_input(&encoder->mAudioInputContext, filePath, 0, 0);
	//DEBUG CODE:
	// char errStr[1024] = {0};
	// av_strerror(errcode, errStr, 1024 - 1);
	if (errcode < 0)
	{
		return false;
	}

	if (avformat_find_stream_info(encoder->mAudioInputContext, 0) < 0)
	{
		return false;
	}

	av_dump_format(encoder->mAudioInputContext, 0, filePath, 0);


	int audioindex_a = -1, audioindex_out = -1;
	for (size_t i = 0; i < encoder->mAudioInputContext->nb_streams; i++)
	{
		if (encoder->mAudioInputContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audioindex_a = (int)i;

			encoder->mAudioInputStream = encoder->mAudioInputContext->streams[i];

			AVCodecParameters* in_codecpar = encoder->mAudioInputStream->codecpar;

			//create new audio output stream
			encoder->mAudioOutputStream = avformat_new_stream(encoder->mOFormatContext, NULL);
			if (!encoder->mAudioOutputStream)
			{
				return false;
			}
			encoder->mAudioOutputStream->id = encoder->mOFormatContext->nb_streams - 1;
			audioindex_out = encoder->mAudioOutputStream->index;

			//======================   CREATE OUPUT CONTEXT ===========================//
			AVCodecContext* c = avcodec_alloc_context3(encoder->mAudioCodec);
			if (!c)
			{
				return false;
			}

			encoder->mAudioCodecContext = c;

			avcodec_parameters_to_context(c, encoder->mAudioInputStream->codecpar);

			assert(c->sample_rate == encoder->mAudioInputStream->codecpar->sample_rate);
			assert(c->channels == encoder->mAudioInputStream->codecpar->channels);
			assert(c->channel_layout == encoder->mAudioInputStream->codecpar->channel_layout);
			assert(c->bit_rate == encoder->mAudioInputStream->codecpar->bit_rate);

			encoder->mAudioOutputStream->time_base.num = 1;
			encoder->mAudioOutputStream->time_base.den = c->sample_rate;

			//copyparams from input to autput audio stream:
			if (avcodec_parameters_copy(encoder->mAudioOutputStream->codecpar, encoder->mAudioInputStream->codecpar) < 0)
			{

				return false;
			}

			if (encoder->mOFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
			{
				//  printf("global header audio\n");
				c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
			}

			if (avcodec_open2(c, encoder->mAudioCodec, NULL) < 0)
			{
				return false;
			}

			break;
		}

	}

	encoder->mAudioPacket = av_packet_alloc();//TODO: Optimize

	return true;
}

static bool EncodeAudioFrame(VideoEncoderContext* encoder)
{



	const AVRational r = { 1 ,1 };

	const int rr = av_compare_ts(encoder->next_pts_audio, encoder->mAudioCodecContext->time_base, (int64_t)encoder->mAUdioTimeLimit, r);

	if (rr >= 0)
	{

		encoder->next_pts_audio = INT64_MAX;
		return true;
	}


	int ret = 0;
	ret = av_read_frame(encoder->mAudioInputContext, encoder->mAudioPacket);
	if (encoder->mAudioPacket->pts == AV_NOPTS_VALUE)
	{
		encoder->mAudioPacket->pts = 0;
		encoder->mAudioPacket->dts = 0;
	}



	//when we reach the end of audio file we continue muxing
	//only the video track from that moment.Hence we set
	//audio time to infinite to cancel out future audioframe writes
	if (ret == AVERROR_EOF)
	{

		encoder->next_pts_audio = INT64_MAX;
		return true;
	}


	encoder->mAudioPacket->stream_index = encoder->mAudioOutputStream->index;

	av_packet_rescale_ts(encoder->mAudioPacket, encoder->mAudioInputStream->time_base, encoder->mAudioOutputStream->time_base);

	encoder->next_pts_audio = encoder->mAudioPacket->pts;

	ret = av_interleaved_write_frame(encoder->mOFormatContext, encoder->mAudioPacket);

	av_packet_unref(encoder->mAudioPacket);

	return true;

}
 
VideoEncoderContext* VideoEncoderCreate()
{
	return calloc(1, sizeof(VideoEncoderContext));
}

bool VideoEncoderInit(VideoEncoderContext* encoder, const char* videoFilePath,
	const char* audioTrackFilePath, uint32_t videoWidth, uint32_t videoHeight,
	uint64_t bitRate, uint32_t frameRate, InputImageFormat inputFormat, bool flipVertically)
{

	encoder->mFlipVertically = flipVertically;
	encoder->mOformat = av_guess_format(NULL, videoFilePath, NULL);

	if (!encoder->mOformat)
	{
		printf("can't create output format");
		return false;
	}

	int err = avformat_alloc_output_context2(&encoder->mOFormatContext, encoder->mOformat, NULL, videoFilePath);

	if (err)
	{
		printf("can't create output context");
		return false;
	}

	encoder->mCodec = avcodec_find_encoder(encoder->mOformat->video_codec);
	if (!encoder->mCodec)
	{
		printf("can't create codec");
		return false;
	}

	encoder->mVideoStream = avformat_new_stream(encoder->mOFormatContext, encoder->mCodec);

	if (!encoder->mVideoStream)
	{
		printf("can't find format\n");
		return false;
	}

	encoder->mCodecContext = avcodec_alloc_context3(encoder->mCodec);

	if (!encoder->mCodecContext)
	{
		printf("can't create codec context\n");
		return false;
	}

	encoder->mVideoStream->codecpar->codec_id = encoder->mOformat->video_codec;
	encoder->mVideoStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
	encoder->mVideoStream->codecpar->width = videoWidth;
	encoder->mVideoStream->codecpar->height = videoHeight;
	encoder->mVideoStream->codecpar->format = AV_PIX_FMT_YUV420P;
	encoder->mVideoStream->codecpar->bit_rate = bitRate;// bitrate * 1000;

	avcodec_parameters_to_context(encoder->mCodecContext, encoder->mVideoStream->codecpar);


	encoder->mCodecContext->profile = FF_PROFILE_H264_MAIN;
	encoder->mCodecContext->max_b_frames = 2;
	encoder->mCodecContext->gop_size = (int)frameRate;
	encoder->mCodecContext->framerate = (AVRational){ (int)frameRate, 1 };
	encoder->mCodecContext->time_base = (AVRational){ 1,  (int)frameRate };
	//must remove the following
	encoder->mCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	if (encoder->mVideoStream->codecpar->codec_id == AV_CODEC_ID_H264)
	{
		av_opt_set(encoder->mCodecContext, "preset", "ultrafast", 0);
	}
	else if (encoder->mVideoStream->codecpar->codec_id == AV_CODEC_ID_H265)
	{
		av_opt_set(encoder->mCodecContext, "preset", "ultrafast", 0);
	}

	//err = av_opt_set(mCodecContext->priv_data, "crf", "12", 0);
	//if (err < 0)
	//{
		//std::cerr << "Error : " << AVException(err, "av_opt_set crf").what() << std::endl;
	//}
	//err = av_opt_set(mCodecContext->priv_data, "profile", "main", 0);
//	if (err < 0)
	//{
		//std::cerr << "Error : " << AVException(err, "av_opt_set profile").what() << std::endl;
	//}

	// disable b-pyramid. CLI options for this is "-b-pyramid 0"
	//Because Quicktime (ie. iOS) doesn't support this option
	err = av_opt_set(encoder->mCodecContext->priv_data, "b-pyramid", "0", 0);
	if (err < 0)
	{
		//std::cerr << "Error : " << AVException(err, "av_opt_set b-pyramid").what() << std::endl;
	}
	//	err = avcodec_parameters_to_context(mCodecContext, mVideoStream->codecpar);

	if (encoder->mOFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
	{

		encoder->mCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		//	mVideoStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}



	if ((err = avcodec_open2(encoder->mCodecContext, encoder->mCodec, NULL)) < 0)
	{
		printf("Failed to open codec\n");
		return false;
	}

	avcodec_parameters_from_context(encoder->mVideoStream->codecpar, encoder->mCodecContext);

	if (!(encoder->mOformat->flags & AVFMT_NOFILE))
	{
		if ((err = avio_open(&encoder->mOFormatContext->pb, videoFilePath, AVIO_FLAG_WRITE)) < 0)
		{
			printf("Failed to open file: %i\n", err);
			return false;
		}
	}



	if (audioTrackFilePath && strlen(audioTrackFilePath))
	{
		if (!InitAudio(encoder, audioTrackFilePath, UINT32_MAX))
		{
			return false;
		}
	}


	if ((err = avformat_write_header(encoder->mOFormatContext, NULL)) < 0)
	{
		printf("Failed to write header: %i\n", err);
		return false;
	}

	encoder->mPacket = av_packet_alloc();

	av_dump_format(encoder->mOFormatContext, 0, videoFilePath, 1);

	//!!! here input implied to be  RGB,we must be ready for RGBA
	enum AVPixelFormat avPixelFormat = AV_PIX_FMT_RGBA;
	switch (inputFormat)
	{

	case CA_RGB:
		avPixelFormat = AV_PIX_FMT_RGB24;
		break;
	case CA_BGRA:
		avPixelFormat = AV_PIX_FMT_BGRA;
		break;
	case CA_BGR:
		avPixelFormat = AV_PIX_FMT_BGR24;
		break;

	}
	if (!encoder->mSwsCtx)
	{
		encoder->mSwsCtx = sws_getContext(encoder->mCodecContext->width, encoder->mCodecContext->height,
			avPixelFormat, encoder->mCodecContext->width,
			encoder->mCodecContext->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, 0, 0, 0);
	}

	printf("FFMPEG encoder setup is ready\n");

	return true;
}

bool VideoEncoderEncodeFrame(VideoEncoderContext* encoder, uint8_t* data)
{

	int err;
	if (!encoder->mVideoFrame)
	{
		encoder->mVideoFrame = av_frame_alloc();
		encoder->mVideoFrame->format = encoder->mVideoStream->codecpar->format; // AV_PIX_FMT_YUV420P;

		encoder->mVideoFrame->width = encoder->mCodecContext->width;
		encoder->mVideoFrame->height = encoder->mCodecContext->height;
		if ((err = av_frame_get_buffer(encoder->mVideoFrame, 0)) < 0)
		{
			printf("Failed to allocate picture: %i\n", err);//  
			return false;
		}
	}

	err = av_frame_make_writable(encoder->mVideoFrame);


	int inLinesize[1] = { 4 * encoder->mCodecContext->width };

	/// in UE4  FBO comes flipped already so we are ok

	if (encoder->mFlipVertically)
	{
		data += inLinesize[0] * (encoder->mCodecContext->height - 1);
		inLinesize[0] = -inLinesize[0];
	}


	// From RGB to YUV,SLOW!
	sws_scale(encoder->mSwsCtx, (const uint8_t* const*)&data, inLinesize, 0, encoder->mCodecContext->height, encoder->mVideoFrame->data, encoder->mVideoFrame->linesize);

	encoder->mVideoFrame->pts = encoder->mFrameCount++;
	//Encoding
	err = avcodec_send_frame(encoder->mCodecContext, encoder->mVideoFrame);
	if (err < 0)
	{
		printf("Failed to send frame: %i\n", err);
		return false;
	}



	while (err >= 0)
	{

		err = avcodec_receive_packet(encoder->mCodecContext, encoder->mPacket);
		if (err == AVERROR(EAGAIN))
		{

			return true;
		}
		else if (err == AVERROR_EOF)
		{
			return true;
		}
		else if (err < 0)
		{
			fprintf(stderr, "Error during encoding,shutting down\n");
			exit(1);
		}

		if (encoder->mPacket->pts != AV_NOPTS_VALUE)
		{
			encoder->mPacket->pts = av_rescale_q(encoder->mPacket->pts, encoder->mCodecContext->time_base, encoder->mVideoStream->time_base);
		}

		if (encoder->mPacket->dts != AV_NOPTS_VALUE)
		{
			//we don't need to calc DTS at all,ffmpeg also spits some weird warnings
			encoder->mPacket->dts = av_rescale_q(encoder->mPacket->dts, encoder->mCodecContext->time_base, encoder->mVideoStream->time_base);
		}


		encoder->mPacket->duration = av_rescale_q(1, encoder->mCodecContext->time_base, encoder->mVideoStream->time_base);


		// const int64_t duration = av_rescale_q(1, mCodecContext->time_base, mVideoStream->time_base);
		// mPacket->duration = duration;
		//mPacket->pts = next_pts;
		//mPacket->dts = next_pts;
		encoder->next_pts += encoder->mPacket->duration;


		encoder->mPacket->stream_index = encoder->mVideoStream->index;

		av_interleaved_write_frame(encoder->mOFormatContext, encoder->mPacket);
		av_packet_unref(encoder->mPacket);

		if (encoder->mAudioOutputStream)
		{
			while (av_compare_ts(encoder->next_pts, encoder->mVideoStream->time_base,
				encoder->next_pts_audio, encoder->mAudioOutputStream->time_base) > 0)
			{
				EncodeAudioFrame(encoder);
			}
		}
	}

	return true;
}

void VideoEncoderFinalize(VideoEncoderContext* encoder)
{
	//DELAYED FRAMES

	for (;;)
	{
		avcodec_send_frame(encoder->mCodecContext, NULL);
		if (avcodec_receive_packet(encoder->mCodecContext, encoder->mPacket) == 0)
		{

			if (encoder->mPacket->pts != AV_NOPTS_VALUE)
			{
				//rounding versons works okay too
				encoder->mPacket->pts = av_rescale_q(encoder->mPacket->pts, encoder->mCodecContext->time_base, encoder->mVideoStream->time_base); 
			}

			if (encoder->mPacket->dts != AV_NOPTS_VALUE)
			{
				//we don't need to calc DTS at all,ffmpeg also spits some weird warnings
				encoder->mPacket->dts = av_rescale_q(encoder->mPacket->dts, encoder->mCodecContext->time_base, encoder->mVideoStream->time_base); 
			}

			encoder->mPacket->duration = av_rescale_q(1, encoder->mCodecContext->time_base, encoder->mVideoStream->time_base); 

			encoder->next_pts += encoder->mPacket->duration;


			encoder->mPacket->stream_index = encoder->mVideoStream->index;
			av_interleaved_write_frame(encoder->mOFormatContext, encoder->mPacket);
			av_packet_unref(encoder->mPacket);

			//don't forget to add audio to the flushed video frames
			if (encoder->mAudioCodecContext)
			{
				while (av_compare_ts(encoder->next_pts, encoder->mVideoStream->time_base,
					encoder->next_pts_audio, encoder->mAudioOutputStream->time_base) > 0)
				{
					EncodeAudioFrame(encoder);
				}
			}
		}
		else
		{
			break;
		}
	}



	av_write_trailer(encoder->mOFormatContext);
	if (!(encoder->mOformat->flags & AVFMT_NOFILE))
	{
		int err = avio_close(encoder->mOFormatContext->pb);
		if (err < 0)
		{
			printf("Failed to close file: %i\n", err);
		}
	}
}

void VideoEncoderDestroy(VideoEncoderContext* encoder)
{

	av_packet_free(&encoder->mPacket);

	if (encoder->mVideoFrame)
	{
		av_frame_free(&encoder->mVideoFrame);
	}
	if (encoder->mCodecContext)
	{
		avcodec_close(encoder->mCodecContext);
		avcodec_free_context(&encoder->mCodecContext);

	}
	if (encoder->mOFormatContext)
	{
		avformat_free_context(encoder->mOFormatContext);

	}
	if (encoder->mSwsCtx)
	{
		sws_freeContext(encoder->mSwsCtx);

	}

	//audio
	if (encoder->mAudioCodecContext)
	{
		av_packet_free(&encoder->mAudioPacket);
		avformat_close_input(&encoder->mAudioInputContext);
		avcodec_close(encoder->mAudioCodecContext);
		avcodec_free_context(&encoder->mAudioCodecContext);
		avformat_free_context(encoder->mAudioInputContext);
	}

	if (encoder)
	{
		free(encoder);
	}
}