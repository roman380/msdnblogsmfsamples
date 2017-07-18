MFDub is a simple linear video editor built on Media Foundation.  It combines scrubbing,
playback, transcode, and media processing into one application.  Using MFDub, a person
can apply simple effects to a video, preview the results, and save the results to an output
file in a matter of minutes.


Installation:
-------------

MFVEUTIL.dll must be registered in order for the default MFDub transforms to be available from the UI.
Run 'regsvr32 MFVEUTIL.dll' from an elevated command prompt.

Usage:
------

Main toolbar:

- Use the Open button to open a video file for editing.  Any video file supported by Media Foundation,
such as WMV, MP4, and AVI, can be opened by MFDub.  As well, if a byte stream handler is installed for
a custom media source, MFDub can open any files associated with that byte stream handler automatically.

- Use the Save button to encode the output video file.  Any effects currently applied to the output
stream will be encoded into the result.  ASF (WMV) and MP4 are the supported output formats.  The output
format is selected based on the file extension (ASF for .wmv, .asf and MP4 for .mp4, .m4v).

- Use the Video Transforms button to add a new video transform.  Some transforms support configuration and
will prompt for further configuration after you add them.

- Use the Audio Transform button to add a new audio transform.

- Use the Encode Options button to open the encoding options dialog.  You can specify the bitrate of the output file
and change the output file frame rate if you so desire.

- Use the Properties button to view metadata about the source format.


Video displays:

- The left video window displays the original source video, while the right video window displays a preview of the
transformed video.


Video Transforms / Audio Transforms list:

- These panels display all of the video transforms and audio transforms that will be applied to the output file, and
that are currently being used to transform the preview video.

- Click and drag a transform to move it up or down in the list.  This will change the order that transforms are
applied to the video.

- Click and drag a transform outside of the transform panel to remove the transform.


Scrub bar:

- Click anywhere on the scrub bar to seek to that point in the video.

- Drag the pointer (thumb) along the scrub bar to view frames along the pointer's position.


Controls bar:

- Click the Play (O) button to play back the original source file.

- Click the Play (P) button to play back the preview.

- Click the Stop button during play or transcode to stop and return to scrub mode.

- The button with the arrow pointing towards the beginning of the file seeks to the first frame in the file.

- The button with the arrow pointing towards the end of the file seeks to the last frame in the file.


Build instructions:
-------------------

Build with Visual Studio + the Windows 7 SDK.

API demonstrated:
-----------------

- IMFSourceReader
- IMFSinkWriter
- IMFMediaSession
- IMFTransform (use and implemenation)
- IMFTopology
- IMFTopologyNode
- IMFSample
- IMFMediaBuffer

Requirements:
-------------

OS: Windows 7

Requires ATL.

Known limitations:
------------------

MFDub's scrubbing support is a rather naive and non-performant implementation.  It is quite
possible to implement a better scrubbing solution than this, but for the purposes of demonstrating
MF capabilities the current scrubbing implementation works okay.

MFDub only has one audio transform by default, and only 5 video transforms.  Though it is fun to play
around with some of them, more transforms are needed to be truly useful as a linear video editor.

