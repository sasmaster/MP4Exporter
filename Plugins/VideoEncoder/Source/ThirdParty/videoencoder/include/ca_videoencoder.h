#ifndef CA_VIDEO_ENCODER
#define CA_VIDEO_ENCODER

#include <stdbool.h>
#include <stdint.h>

typedef enum 
{
	CA_RGBA,
	CA_RGB,
	CA_BGRA,
	CA_BGR
}InputImageFormat;

 
typedef struct VideoEncoderContext VideoEncoderContext;


VideoEncoderContext* VideoEncoderCreate();


bool VideoEncoderInit(VideoEncoderContext* encoder,const char* videoFilePath,
	const char* audioTrackFilePath, uint32_t videoWidth, uint32_t videoHeight,
	uint64_t bitRate, uint32_t frameRate, InputImageFormat inputFormat, bool flipVertically);

bool VideoEncoderEncodeFrame(VideoEncoderContext* encoder,uint8_t* data);

void VideoEncoderFinalize(VideoEncoderContext* encoder);

void VideoEncoderDestroy(VideoEncoderContext* encoder);


#endif