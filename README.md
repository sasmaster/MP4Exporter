# MP4ExporterUE4
MP4 Video exporter pluging for Unreal Movie Render Queue.
The plugin allows encoding the animation in the RenderQueue into mp4,with optional AAC audio track.
The plugin can be used in editor as well as in runtime builds.
The plugin is still pretty raw,I have been developing it as a part of a bigger project I have been working on.
Any feedbacks,feature requests,bug reports are much welcommed,but I cannot gurantee fast addressing of those.


## Supported platforms
This specific distribution has been test with UE4 V4.27, Windows. Some of the source code is precompiled into static library.
 
## Dependencies
The following libav libraries are linked dynamically with this plugin's source code: avcodec-58.dll,avformat-58.dll,avutil-56.dll,swscale-5.dll,swresample-3.dll
The encoding/multiplexing logic that uses the above libraries resides in 2 .c files, which are provided separately: ca_videoencoder.h,ca_videoencoder.c
These two files have been precompiled with MSVC 2019 for 64bit target into static librariy which is linked with UE4 plugin of the example project.



## License
The plugin contains a code which links with LIBAV project so all the relevant licenses apply herein as well.
This software uses libraries from the Libav project under the LGPLv2.1 or GPL v2 or later, depending what kind of LIBAV libraries you
are planning to use in your project. This specific version is linked with LIBAV libraries compiled with --enable-gpl on. In case you 
decide to use this project as is, GPL license is also applicable to thhis project and where it is going to be incorporated.
Please check [HERE](https://libav.org/legal/) the LIBAV license comppilance terms & conditions before you decide to sell and disitrbute your UE4
project with this plugin. Also note that UE4 marketplace forbids distribution of plugins with LGPL/GPL licensing types.
I strongly recomend to consult to consult your lawyers if you mean to sell or distribute your own project along with this code.

Any Epic Games / UnrealEngine related code is covered under license outlined in EpicGames UE4 EULA.



 



