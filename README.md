# Samples from Media Foundation Team Blog

The repository contains samples published on [Media Foundation Team Blog](https://blogs.msdn.microsoft.com/mf). The links to actual source code got broken at some point, hence this repository.

## MFCopy

[MFCopy.exe](https://blogs.msdn.microsoft.com/mf/2009/12/16/mfcopy/) - This is a command line tool showcasing the Windows 7 Media Foundation source reader and sink writer APIs. This tool copies multimedia files from one multimedia container to another. Copying can involve simply remultiplexing the streams, transcoding to convert the streams to a different format. The source code is provided to use as a reference for learning about the source reader and sink writer.

## MFDub

[MFDub](https://blogs.msdn.microsoft.com/mf/2010/03/12/mfdub/) is a simple linear video editor built on Media Foundation, modeled after the free video editor VirtualDub. MFDub can open any file format that can be resolved by the MF source resolver, display frames of video from the file, apply a series of video or audio effects implemented as MF transforms, and then save the file with effects applied to the ASF (WMV, WMA) file format or the MPEG4 file format. For example, one might open up a WMV file with blurry video using MFDub, apply the unsharp mask transform to create a video with a sharper image, and then save the output video to an MPEG4 file. 

## MFMediaPropDump

[This](https://blogs.msdn.microsoft.com/mf/2010/01/11/mfmediapropdump/ is a command line tool showcasing the Windows 7 Media Foundation APIs to read media source attributes and metadata. It also shows how to leverage the Windows Media Format SDK to read DRM properties. The source code can be useful for learning about Media Foundation. The built executable can be useful for dumping media file properties for debugging, or simply to get information about a media file, such as the frame size, frame rate, bit rate, or sample rate.

This sample can display properties for any media file format that is supported by Media Foundation.

Media Foundation does not currently have support for reading DRM properties, so the legacy Format SDK is used for that reason in the sample. Using Format SDK is otherwise discouraged, as it supports only Windows Media file formats and has some other key disadvantages. Media Foundation supports a much wider set of media file formats.

### Acknowledgements

1. [Media Foundation Team Blog](https://blogs.msdn.microsoft.com/mf)
2. mofo77's [`MFNode`](https://sourceforge.net/projects/mfnode/) at SourceForge
