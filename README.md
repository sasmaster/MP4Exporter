# MP4Exporter for Unreal Engine 4/5
LibAV based MP4 Video exporter plugin for Unreal Movie Render Queue. Supports Unreal Engine 4.27 and 5.3.
The plugin allows encoding sequencer animation via RenderQueue into mp4 video,with optional AAC audio track.
The plugin can be used in editor as well as in runtime builds.
Any feedbacks,feature requests,bug reports are welcomed.

## Supported platforms
All the source code is provided here and completely portable to any platform supported by Unreal Engine.I tested only on Windows.

## Dependencies
The following libav libraries are linked dynamically with this plugin's source code: avcodec-58.dll,avformat-58.dll,avutil-56.dll,swscale-5.dll,swresample-3.dll
The encoding/multiplexing logic that uses the above libraries resides in 2 .c files: ca_videoencoder.h,ca_videoencoder.c
These two files were precompiled with MSVC 2019 for 64bit target into static librariy which is optionally provided and can be linked
with the plugin instead of the source files.


## Usage

1. All the libav shared libs are provided as bin.zip file in 'Plugins\VideoEncoder\Source\ThirdParty\ffmpeg' directory. Unzip it in the same location so that all the files end up in 'bin' folder.
2. Right click VideoEncoderUE.uproject and press 'Generate Visual Studio project files'. VS 2017,2019,2022 should work fine,but let me know if there is a problem.
3. Compile the solution,make sure the shared libs (DLL files) are put into the same location where your project executable file is, then launch the engine test project either via VS or via the launcher file.
4. From now on I assume you know how to create level sequence and setup the movie render queue. The test project contains a preset for the exporter:

![MP4 exporter preset](/Docs/p1.JPG)

5. The MP4 exporter's interface at this point of time is very simple.It allows the user to provide the bitrate (in bits),which is very important for the video quality.
   You may check YOUTUBE resolution/bitrate reference table for recommended bitrates. And the optional audio file location is provided,so one can add a soundtrack to the encoded movie queue.
   The audio file must be provided in AAC format. Support for WAV and MP3 are on TODO list

![MP4 exporter settings](/Docs/p2.JPG)

6. Once the exporter is set,just press the "Render" button and the video export will begin. The rest of the settings in the Render Queue are as usual.

![MP4 exporter export](/Docs/p3.JPG)


## License
The plugin contains a code which links with LIBAV project so all the relevant licenses apply herein as well.
This software uses libraries from the Libav project under the LGPLv2.1 or GPL v2 or later, depending what kind of LIBAV libraries you
are planning to use in your project. This specific version is linked with LIBAV libraries compiled with --enable-gpl on. In case you 
decide to use this project as is then GPL license is in force instead of LGPL.
Please check [HERE](https://libav.org/legal/) the LIBAV license comppilance terms & conditions before you decide to sell and disitrbute your UE
project with this plugin. Also note that Unreal marketplace forbids distribution of plugins with LGPL/GPL licensing types.
I strongly recomend consulting with a lawyer if you mean to sell or distribute your own project along with this code.

Any Epic Games / UnrealEngine related code is covered under license outlined in EpicGames Unreal Engine EULA.



 



