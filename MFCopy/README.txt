//////////////////////////////////////////////////////////////////////////
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////

This sample copies multimedia content by remuxing and/or
transcoding the individual streams to a destination file.

The sample uses the Media Foundation source reader to read samples
from the source file and optionally decode compressed samples.  The
sample then uses the Media Foundation sink writer to optionally
encode the uncompressed sample data and write the multimedia data
out to the destination file.

Usage:
------

Command line options:

    -?

        display usage information

    -a audio_format

        set the audio format (defaults to the native format)
            eg. AAC, WMAudioV8, WMAudioV9, WMAudio_Lossless

    -nc num_channels

        set the number of audio channels
            eg. 2

    -sr sample_rate

        resample the audio to the specified rate
            eg. 44100, 48000

    -v video_format

        set the video format (defaults to the native format)
            eg. H264, WMV2, WMV3, WVC1

    -fs WxH

        resize the video to the specified width and height
            eg. 640x480

    -fr N:D

        convert the video framerate to the rate specified
        in terms of the numerator and denominator
            eg. 30:1

    -im mode

        set the video interlace mode (one of the MFVideoInterlace settings)
            eg. Progressive

    -br bitrate

        set the average bitrate for the video stream

    -q

        quiet mode

    -s ms

        specify a start position in milliseconds

    -d ms

        specify a duration in milliseconds

    -hw

        enable hardware transforms

    -xa

        exclude audio streams

    -xv

        exclude video streams

    -xo

        exclude non audio/video streams

    -xm

        exclude metadata



Examples:
---------
    
    Remux 20 seconds of a WMV file:

        mfcopy.exe -d 20000 input.wmv output.wmv


    Resize the video of a WMV file while preserving the original audio stream:

        mfcopy.exe -fs 320x240 input.wmv output.wmv
      

    Transcode WMV to MP4 ensuring progressive video:

        mfcopy.exe -a AAC -v H264 -im Progressive input.wmv output.mp4



Known limitations/assumptions of this sample:
---------------------------------------------

* The sample does not support encoding VBR content
  (except WMA Lossless which is VBR at 100% quality)

* Attempting to specify a start position when any of the audio or
  video streams are getting remuxed may result in the undesired 
  behavior where the start position is set back to the location
  of the previous video key frame.
