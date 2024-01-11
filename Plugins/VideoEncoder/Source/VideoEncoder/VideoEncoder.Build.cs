// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class VideoEncoder : ModuleRules
{
	public VideoEncoder(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Engine",
				"Projects",
				"CoreUObject",
				"MovieRenderPipelineCore",
				"MovieRenderPipelineSettings",
				"MovieRenderPipelineRenderPasses",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Engine",
			 
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

		PrivateIncludePaths.Add(Path.Combine(PluginDirectory, "Source/ThirdParty/ffmpeg/include"));
        PrivateIncludePaths.Add(Path.Combine(PluginDirectory, "Source/ThirdParty/videoencoder/include"));

        // Add any import libraries or static libraries
        PublicAdditionalLibraries.Add(Path.Combine(PluginDirectory, "Source/ThirdParty/ffmpeg/lib", "avcodec.lib"));
		PublicAdditionalLibraries.Add(Path.Combine(PluginDirectory, "Source/ThirdParty/ffmpeg/lib", "avformat.lib"));
		PublicAdditionalLibraries.Add(Path.Combine(PluginDirectory, "Source/ThirdParty/ffmpeg/lib", "avutil.lib"));
		PublicAdditionalLibraries.Add(Path.Combine(PluginDirectory, "Source/ThirdParty/ffmpeg/lib", "swscale.lib"));
		//uncomment if prefer to use precompiled lib instead of source
	//	PublicAdditionalLibraries.Add(Path.Combine(PluginDirectory, "Source/ThirdParty/videoencoder/lib/Win32/Release", "LibVideoEncoder1.lib"));

	 

		//preload the DLLs
		PublicDelayLoadDLLs.Add("avcodec-58.dll");
		PublicDelayLoadDLLs.Add("avformat-58.dll");
		PublicDelayLoadDLLs.Add("avutil-56.dll");
		PublicDelayLoadDLLs.Add("swscale-5.dll");
		PublicDelayLoadDLLs.Add("swresample-3.dll");

		//https://github.com/ue4plugins/VlcMedia/blob/master/Source/VlcMedia/VlcMedia.Build.cs

		string FFMPEGBinDirectory = Path.Combine(PluginDirectory, "Source/ThirdParty/ffmpeg/bin");

		RuntimeDependencies.Add("$(TargetOutputDir)/avcodec-58.dll", Path.Combine(FFMPEGBinDirectory, "avcodec-58.dll"));
		RuntimeDependencies.Add("$(TargetOutputDir)/avformat-58.dll", Path.Combine(FFMPEGBinDirectory, "avformat-58.dll"));
		RuntimeDependencies.Add("$(TargetOutputDir)/avutil-56.dll", Path.Combine(FFMPEGBinDirectory, "avutil-56.dll"));
		RuntimeDependencies.Add("$(TargetOutputDir)/swscale-5.dll", Path.Combine(FFMPEGBinDirectory, "swscale-5.dll"));
		RuntimeDependencies.Add("$(TargetOutputDir)/swresample-3.dll", Path.Combine(FFMPEGBinDirectory, "swresample-3.dll"));

	}
}
