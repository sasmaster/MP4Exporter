// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MoviePipelineVideoOutputBase.h" 
#include "MP4MoviePipelineVideoOutput.generated.h"

/**
 * 
 */
UCLASS()
class VIDEOENCODER_API UMP4MoviePipelineVideoOutput : public UMoviePipelineVideoOutputBase
{
	GENERATED_BODY()


	struct FFMPEGOptions
	{
		FString filePath;
		FString audioFilePath;
		uint32 videoWidth = 0;
		uint32 videoHeight = 0;
		uint32 fps = 0;
		uint32 bitrate = 0;
	}mFFMPEGOptions;

protected:
	TUniquePtr<MovieRenderPipeline::IVideoCodecWriter>
		Initialize_GameThread(const FString& InFileName, FIntPoint InResolution,
			EImagePixelType InPixelType, ERGBFormat InPixelFormat, uint8 InBitDepth, uint8 InNumChannels)override final;

	bool Initialize_EncodeThread(MovieRenderPipeline::IVideoCodecWriter* InWriter)override final;

	void WriteFrame_EncodeThread(MovieRenderPipeline::IVideoCodecWriter* InWriter,
		FImagePixelData* InPixelData, TArray<MoviePipeline::FCompositePassInfo>&& InCompositePasses)override final;

	void BeginFinalize_EncodeThread(MovieRenderPipeline::IVideoCodecWriter* InWriter)override final;

	void Finalize_EncodeThread(MovieRenderPipeline::IVideoCodecWriter* InWriter)override final;

	const TCHAR* GetFilenameExtension() const override final;

	bool IsAudioSupported()const override final; 



protected:
	struct FFMPEGWriter : public MovieRenderPipeline::IVideoCodecWriter
	{
		  struct VideoEncoderContext* Writer = nullptr;

		// ~FFMPEGWriter();
		 
	};

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	    int64 BitRate;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	//FString AudioFilePath;

	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category = Settings)
	
	FFilePath AudioFilePath;
};

 
