// Fill out your copyright notice in the Description page of Project Settings.


#include "MP4MoviePipelineVideoOutput.h"

#include "MoviePipeline.h"
#include "MoviePipelineOutputSetting.h"
#include "MoviePipelineMasterConfig.h"
#include "MoviePipelineImageQuantization.h"
#include "ImageWriteTask.h"
#include "Misc/Paths.h"

 
extern "C"
{
    #include "ca_videoencoder.h"
}




// For logs
#include "MovieRenderPipelineCoreModule.h"
//impl reference:
//MoviePipelineAppleProResOutput in 
//C:\Program Files (x86)\Epic Games\UE_4.27\Engine\Plugins\Media\AppleProResMedia\Source\AppleProResMedia\Private

TUniquePtr<MovieRenderPipeline::IVideoCodecWriter> 
UMP4MoviePipelineVideoOutput::Initialize_GameThread(const FString& InFileName, FIntPoint InResolution,
	EImagePixelType InPixelType, ERGBFormat InPixelFormat, uint8 InBitDepth, uint8 InNumChannels)
{

	const UMoviePipelineOutputSetting* OutputSettings = GetPipeline()->GetPipelineMasterConfig()->FindSetting<UMoviePipelineOutputSetting>();
	if (!OutputSettings)
	{
		return nullptr;
	}
	 
	mFFMPEGOptions.filePath = InFileName;

	//if the AudioFilePath is not set we have to omit the below formatting as it will build the string for the location
	//of the engine executables which we don't want

	 
	if (!AudioFilePath.FilePath.IsEmpty())
	{
		mFFMPEGOptions.audioFilePath = FPaths::ConvertRelativePathToFull(AudioFilePath.FilePath);
	}
	mFFMPEGOptions.videoWidth = InResolution.X;
	mFFMPEGOptions.videoHeight = InResolution.Y;
	mFFMPEGOptions.fps = GetPipeline()->GetPipelineMasterConfig()->GetEffectiveFrameRate(GetPipeline()->GetTargetSequence()).Numerator;

	if (BitRate == 0)
	{
		BitRate = 5000000;
	}
	mFFMPEGOptions.bitrate = BitRate;

#if 0
	FAppleProResEncoderOptions Options;
	Options.OutputFilename = InFileName;
	Options.Width = InResolution.X;
	Options.Height = InResolution.Y;
	Options.FrameRate = GetPipeline()->GetPipelineMasterConfig()->GetEffectiveFrameRate(GetPipeline()->GetTargetSequence());
	Options.MaxNumberOfEncodingThreads = bOverrideMaximumEncodingThreads ? MaxNumberOfEncodingThreads : 0; // Hardware Determine
	Options.Codec = Codec;
	Options.ColorPrimaries = EAppleProResEncoderColorPrimaries::CD_HDREC709; // Force Rec 709 for now
	Options.ScanMode = EAppleProResEncoderScanMode::IM_PROGRESSIVE_SCAN; // No interlace sources.
	Options.bWriteAlpha = true;

#endif
	
	VideoEncoderContext* Encoder = VideoEncoderCreate();
	check(Encoder);
	TUniquePtr<FFMPEGWriter> OutWriter = MakeUnique<FFMPEGWriter>();
	OutWriter->Writer = Encoder; 
	OutWriter->FileName = InFileName;

	return OutWriter;
 
}



bool UMP4MoviePipelineVideoOutput::Initialize_EncodeThread(MovieRenderPipeline::IVideoCodecWriter* InWriter)
{
	//PURE_VIRTUAL(UMoviePipelineVideoOutputBase::Initialize_EncodeThread, return true;);
	check(mFFMPEGOptions.filePath.Len());
	check(mFFMPEGOptions.videoWidth);
	check(mFFMPEGOptions.videoHeight);
	check(mFFMPEGOptions.bitrate);
	check(mFFMPEGOptions.fps);

	FFMPEGWriter* CodecWriter = static_cast<FFMPEGWriter*>(InWriter);
	auto videoPathStr =   TCHAR_TO_UTF8(*mFFMPEGOptions.filePath);
	auto audioStr = TCHAR_TO_UTF8(*mFFMPEGOptions.audioFilePath);

	const FString audioExtension = FPaths::GetExtension(mFFMPEGOptions.audioFilePath);
	if (audioExtension.IsEmpty() == false)
	{
		if (audioExtension.Equals(TEXT("wav"), ESearchCase::IgnoreCase))
		{
			UE_LOG(LogMovieRenderPipeline, Error, TEXT("MP4 format expects AAC audio.Instead wav provided"));

			return false;
		}

		if (audioExtension.Equals(TEXT("aac"), ESearchCase::IgnoreCase) == false)
		{
			UE_LOG(LogMovieRenderPipeline, Error, TEXT("MP4 format expects AAC audio.Unsupported audio format"));
			return false;
		}

		//Now check that the file exists

		if (!FPaths::FileExists(mFFMPEGOptions.audioFilePath))
		{
			UE_LOG(LogMovieRenderPipeline, Error, TEXT("File path: %s doesn't exist"), *mFFMPEGOptions.audioFilePath);
			return false;
		}
	}

	
	if (!VideoEncoderInit(CodecWriter->Writer, videoPathStr, audioStr, mFFMPEGOptions.videoWidth, mFFMPEGOptions.videoHeight,
		mFFMPEGOptions.bitrate, mFFMPEGOptions.fps, CA_BGRA, false))
	{
		UE_LOG(LogMovieRenderPipeline, Error, TEXT("Failed to initialize MP4 Writer."));
		VideoEncoderDestroy(CodecWriter->Writer);
		return false;
	}
	 
	return true;
}

void UMP4MoviePipelineVideoOutput::WriteFrame_EncodeThread(MovieRenderPipeline::IVideoCodecWriter* InWriter, FImagePixelData* InPixelData,
	TArray<MoviePipeline::FCompositePassInfo>&& InCompositePasses)
{

 

	//PURE_VIRTUAL(UMoviePipelineVideoOutputBase::WriteFrame_EncodeThread);
	FFMPEGWriter* CodecWriter = static_cast<FFMPEGWriter*>(InWriter);

	FImagePixelDataPayload* PipelinePayload = InPixelData->GetPayload<FImagePixelDataPayload>();

	TSharedPtr<FImagePixelDataPayload, ESPMode::ThreadSafe>PayloadPtr = MakeShared<FImagePixelDataPayload, ESPMode::ThreadSafe>();

	TUniquePtr<FImagePixelData> QuantizedPixelData = UE::MoviePipeline::QuantizeImagePixelDataToBitDepth(InPixelData, 8, PayloadPtr, InWriter->bConvertToSrgb);//
	 
	// Do a quick composite of renders/burn-ins.
	TArray<FPixelPreProcessor> PixelPreProcessors;
	for (const MoviePipeline::FCompositePassInfo& CompositePass : InCompositePasses)
	{
		// We don't need to copy the data here (even though it's being passed to a async system) because we already made a unique copy of the
		// burn in/widget data when we decided to composite it.
		switch (QuantizedPixelData->GetType())
		{
		case EImagePixelType::Color:
			PixelPreProcessors.Add(TAsyncCompositeImage<FColor>(CompositePass.PixelData->MoveImageDataToNew()));
			break;
		case EImagePixelType::Float16:
			PixelPreProcessors.Add(TAsyncCompositeImage<FFloat16Color>(CompositePass.PixelData->MoveImageDataToNew()));
			break;
		case EImagePixelType::Float32:
			PixelPreProcessors.Add(TAsyncCompositeImage<FLinearColor>(CompositePass.PixelData->MoveImageDataToNew()));
			break;
		}
	}

	// This is done on the main thread for simplicity but the composite itself is parallaleized.
	FImagePixelData* PixelData = QuantizedPixelData.Get();
	for (const FPixelPreProcessor& PreProcessor : PixelPreProcessors)
	{
		// PreProcessors are assumed to be valid.
		PreProcessor(PixelData);
	}

	 
	const void* data = nullptr;
	int64 dataSize = 0;
	QuantizedPixelData.Get()->GetRawData(data, dataSize);
	check(data);
	check(dataSize);

	VideoEncoderEncodeFrame(CodecWriter->Writer, (uint8*)data);
  

}

void UMP4MoviePipelineVideoOutput::BeginFinalize_EncodeThread(MovieRenderPipeline::IVideoCodecWriter* InWriter)
{
	//PURE_VIRTUAL(UMoviePipelineVideoOutputBase::BeginFinalize_EncodeThread)
}

void UMP4MoviePipelineVideoOutput::Finalize_EncodeThread(MovieRenderPipeline::IVideoCodecWriter* InWriter)
{
	//PURE_VIRTUAL(UMoviePipelineVideoOutputBase::Finalize_EncodeThread);

	FFMPEGWriter* CodecWriter = static_cast<FFMPEGWriter*>(InWriter);
	if (CodecWriter->Writer)
	{
		VideoEncoderFinalize(CodecWriter->Writer);
		VideoEncoderDestroy(CodecWriter->Writer);
		CodecWriter->Writer = NULL;
    }

 
	
}

const TCHAR* UMP4MoviePipelineVideoOutput::GetFilenameExtension()const
{
	return TEXT("mp4");
}

bool UMP4MoviePipelineVideoOutput::IsAudioSupported() const
{
	return false;
}

 




