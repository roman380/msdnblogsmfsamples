// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

namespace MFManagedEncode.MediaFoundation
{
    using System;
    using System.Runtime.InteropServices;
    using System.Timers;
    using MFManagedEncode.MediaFoundation.Com;
    using MFManagedEncode.MediaFoundation.Com.Classes;
    using MFManagedEncode.MediaFoundation.Com.Interfaces;
    using MFManagedEncode.MediaFoundation.Com.Structures;

    /// <summary>
    ///     Encodes content using the Windows 7 Transcode API.
    /// </summary>
    internal class SimpleFastEncode
        : ISimpleEncode
    {
        private Timer progressTimer;
        private IMFPresentationClock presentationClock;
        private IMFMediaSession mediaSession;
        private IMFMediaSource mediaSource;
        private ulong startPosition;
        private ulong endPosition;
        private ulong duration;

        public SimpleFastEncode()
        {
            // Create objects and bind the background worker events
            this.EncodeProgress = null;
            this.presentationClock = null;
            this.mediaSession = null;
            this.mediaSource = null;
            this.progressTimer = new Timer(500);
            this.progressTimer.Elapsed += this.ProgressTimer_Tick;
        }

        public event EncodeProgressHandler EncodeProgress;

        public event EncodeErrorHandler EncodeError;

        public event EventHandler EncodeCompleted;

        /// <summary>
        ///     Starts the asychronous encode operation 
        /// </summary>
        /// <param name="inputURL">Source filename</param>
        /// <param name="outputURL">Targe filename</param>
        /// <param name="audioOutput">Audio format that will be used for audio streams</param>
        /// <param name="videoOutput">Video format that will be used for video streams</param>
        /// <param name="startPosition">Starting position of the contet</param>
        /// <param name="endPosition">Position where the new content will end</param>
        public void Encode(string inputURL, string outputURL, AudioFormat audioOutput, VideoFormat videoOutput, ulong startPosition, ulong endPosition)
        {
            // If busy with other operation ignore and return
            if (this.IsBusy())
            {
                return;
            }

            try
            {

                this.presentationClock = null;
                this.startPosition = startPosition;
                this.endPosition = endPosition;

                object objectSource = null;

                // Create the media source using source resolver and the input URL
                uint objectType = default(uint);
                this.mediaSource = null;

                // Init source resolver
                IMFSourceResolver sourceResolver = null;
                MFHelper.MFCreateSourceResolver(out sourceResolver);

                sourceResolver.CreateObjectFromURL(inputURL, Consts.MF_RESOLUTION_MEDIASOURCE, null, out objectType, out objectSource);

                this.mediaSource = (IMFMediaSource)objectSource;

                // Create the media session using a global start time so MF_TOPOLOGY_PROJECTSTOP can be used to stop the session
                this.mediaSession = null;
                IMFAttributes mediaSessionAttributes = null;

                MFHelper.MFCreateAttributes(out mediaSessionAttributes, 1);
                mediaSessionAttributes.SetUINT32(new Guid(Consts.MF_SESSION_GLOBAL_TIME), 1);

                MFHelper.MFCreateMediaSession(mediaSessionAttributes, out this.mediaSession);

                // Create the event handler
                AsyncEventHandler mediaEventHandler = new AsyncEventHandler(this.mediaSession);
                mediaEventHandler.MediaEvent += this.MediaEvent;

                // Get the stream descriptor
                IMFPresentationDescriptor presentationDescriptor = null;
                mediaSource.CreatePresentationDescriptor(out presentationDescriptor);

                // Get the duration
                presentationDescriptor.GetUINT64(new Guid(Consts.MF_PD_DURATION), out this.duration);
                IMFTranscodeProfile transcodeProfile = null;

                Guid containerType = new Guid(Consts.MFTranscodeContainerType_MPEG4);
                if (outputURL.EndsWith(".wmv", StringComparison.OrdinalIgnoreCase) || outputURL.EndsWith(".wma", StringComparison.OrdinalIgnoreCase))
                {
                    containerType = new Guid(Consts.MFTranscodeContainerType_ASF);
                }

                // Generate the transcoding profile
                transcodeProfile = SimpleFastEncode.CreateProfile(audioOutput, videoOutput, containerType);

                // Create the MF topology using the profile
                IMFTopology topology = null;
                MFHelper.MFCreateTranscodeTopology(this.mediaSource, outputURL, transcodeProfile, out topology);

                // Set the end position
                topology.SetUINT64(new Guid(Consts.MF_TOPOLOGY_PROJECTSTART), 0);
                topology.SetUINT64(new Guid(Consts.MF_TOPOLOGY_PROJECTSTOP), (endPosition == 0) ? this.duration : endPosition);

                // Set the session topology
                this.mediaSession.SetTopology((uint)Enums.MFSESSION_SETTOPOLOGY_FLAGS.None, topology);
            }
            catch (Exception ex)
            {
                this.mediaSession = null;

                // Fire the EncodeError event
                if (this.EncodeError != null)
                {
                    this.EncodeError(new Exception(ex.Message, ex));
                }
            }
        }

        /// <summary>
        ///     Gets a value indicating whether the object is running an asynchronous operation.
        /// </summary>
        /// <returns>Value indicating whether the object is busy</returns>
        public bool IsBusy()
        {
            return this.mediaSession != null;
        }

        /// <summary>
        ///     Requests cancellation of a background operation.
        /// </summary>
        public void CancelAsync()
        {
            if (this.IsBusy())
            {
                // If the Background Worker requests cancellation stop the session
                this.mediaSession.Stop();
                this.progressTimer.Stop();
            }
        }

        private static IMFTranscodeProfile CreateProfile(AudioFormat audioOutput, VideoFormat videoOutput, Guid containerType)
        {
            IMFTranscodeProfile profile = null;

            // Create a transcode profile
            MFHelper.MFCreateTranscodeProfile(out profile);

            // Create and set the audio attributes
            profile.SetAudioAttributes(SimpleFastEncode.CreateAudioAttributes(audioOutput));

            // Create and set the video attributes
            profile.SetVideoAttributes(SimpleFastEncode.CreateVideoAttributes(videoOutput));

            // Create the container attributes
            IMFAttributes containerAttributes = null;
            
            MFHelper.MFCreateAttributes(out containerAttributes, 2);
            containerAttributes.SetUINT32(new Guid(Consts.MF_TRANSCODE_TOPOLOGYMODE), 1);
            containerAttributes.SetGUID(new Guid(Consts.MF_TRANSCODE_CONTAINERTYPE), containerType);

            // Set them in the transcoding profile
            profile.SetContainerAttributes(containerAttributes);

            return profile;
        }

        private static IMFAttributes CreateVideoAttributes(VideoFormat videoOutput)
        {
            IMFAttributes videoAttributes = null;
            MFHelper.MFCreateAttributes(out videoAttributes, 0);
            videoAttributes.SetGUID(new Guid(Consts.MF_MT_SUBTYPE), videoOutput.Subtype);

            PackedINT32 pixelAspectRatio = new PackedINT32(1, 1);

            // Set the argument attributes
            videoAttributes.SetUINT64(new Guid(Consts.MF_MT_FRAME_RATE), videoOutput.FrameRate.Packed);
            videoAttributes.SetUINT64(new Guid(Consts.MF_MT_FRAME_SIZE), videoOutput.FrameSize.Packed);
            videoAttributes.SetUINT64(new Guid(Consts.MF_MT_PIXEL_ASPECT_RATIO), pixelAspectRatio.Packed);
            videoAttributes.SetUINT32(new Guid(Consts.MF_MT_INTERLACE_MODE), videoOutput.InterlaceMode);
            videoAttributes.SetUINT32(new Guid(Consts.MF_MT_AVG_BITRATE), videoOutput.AvgBitRate);

            return videoAttributes;
        }

        private static IMFAttributes CreateAudioAttributes(AudioFormat audioOutput)
        {
            uint codecConfig = 0;
            object supportedAttributes = null;

            // Create the audio attributes
            IMFAttributes audioAttributes = null;

            // Generate the audio media type
            uint elementsNumber = 0;
            uint selectedType = 0;
            int avgBitrateDiff = int.MaxValue;
            uint avgBytePerSecond = uint.MaxValue;

            // Get the available audio ouput types for the required sub type
            IMFCollection availableTypes = null;
            MFHelper.MFTranscodeGetAudioOutputAvailableTypes(audioOutput.Subtype, (uint)Enums.MFT_ENUM_FLAG.MFT_ENUM_FLAG_ALL, codecConfig, out availableTypes);

            // Get the number of types
            availableTypes.GetElementCount(out elementsNumber);

            // Find the best match for our needs
            for (uint elementIndex = 0; elementIndex < elementsNumber; elementIndex++)
            {
                // Get the next element
                availableTypes.GetElement(elementIndex, out supportedAttributes);
                audioAttributes = (IMFAttributes)supportedAttributes;

                // Get the byte per second
                audioAttributes.GetUINT32(new Guid(Consts.MF_MT_AUDIO_AVG_BYTES_PER_SECOND), out avgBytePerSecond);

                // If this is better than the last one found remember the index
                if (Math.Abs((int)avgBytePerSecond - (int)audioOutput.AvgBytePerSecond) < avgBitrateDiff)
                {
                    selectedType = elementIndex;
                    avgBitrateDiff = Math.Abs((int)avgBytePerSecond - (int)audioOutput.AvgBytePerSecond);
                }

                audioAttributes = null;
            }

            // Get the best audio type found
            availableTypes.GetElement(selectedType, out supportedAttributes);
            audioAttributes = (IMFAttributes)supportedAttributes;

            return audioAttributes;
        }

        private void ProgressTimer_Tick(object sender, ElapsedEventArgs e)
        {
            if (this.presentationClock != null)
            {
                // Report the conversion progress
                if (this.EncodeProgress != null)
                {
                    long timeStamp = 0;
                    this.presentationClock.GetTime(out timeStamp);

                    ulong denominator = (this.endPosition != 0) ? this.endPosition - this.startPosition : this.duration - this.startPosition;

                    this.ReportProgress(((double)(timeStamp - (long)this.startPosition) / (double)denominator) * 100.0f);
                }
            }
        }

        private void ReportProgress(double progress)
        {
            if (this.EncodeProgress != null)
            {
                this.EncodeProgress(progress);                
            }
        }

        private void MediaEvent(uint eventType, int eventStatus)
        {
            if (eventStatus < 0)
            {
                this.mediaSession = null;

                // A session event reported an error
                if (this.EncodeError != null)
                {
                    this.EncodeError(new COMException("Exception from HRESULT: 0x" + eventStatus.ToString("X", System.Globalization.NumberFormatInfo.InvariantInfo) + " (Media session event #" + eventType + ").", eventStatus));
                }
            }
            else
            {
                switch (eventType)
                {
                    case Consts.MESessionTopologySet:
                        // Start playback from the start position
                        MediaSessionStartPosition startPositionVar = new MediaSessionStartPosition((long)this.startPosition);
                        this.mediaSession.Start(Guid.Empty, startPositionVar);
                        break;
                    case Consts.MESessionStarted:
                        // Get the presentation clock
                        IMFClock clock = null;
                        this.mediaSession.GetClock(out clock);
                        this.presentationClock = (IMFPresentationClock)clock;
                        this.progressTimer.Start();
                        break;
                    case Consts.MESessionEnded:
                        // Close the media session.
                        this.presentationClock = null;
                        this.mediaSession.Close();
                        break;
                    case Consts.MESessionStopped:
                        // Close the media session.
                        this.presentationClock = null;
                        this.mediaSession.Close();
                        break;
                    case Consts.MESessionClosed:
                        // Stop the progress timer
                        this.presentationClock = null;
                        this.progressTimer.Stop();

                        // Shutdown the media source and session 
                        this.mediaSource.Shutdown();
                        this.mediaSession.Shutdown();
                        this.mediaSource = null;
                        this.mediaSession = null;

                        // Fire the EncodeCompleted event
                        if (this.EncodeCompleted != null)
                        {
                            this.EncodeCompleted(this, null);
                        }

                        break;
                }
            }
        }

        private class AsyncEventHandler
            : IMFAsyncCallback
        {
            private IMFMediaSession mediaSession;

            public AsyncEventHandler(IMFMediaSession mediaSession)
            {
                // Strart getting session events
                this.mediaSession = mediaSession;
                this.mediaSession.BeginGetEvent(this, null);
            }

            public delegate void MediaEventHandler(uint EventType, int EventStatus);

            public event MediaEventHandler MediaEvent;

            public int GetParameters(out uint flags, out uint queue)
            {
                flags = 0;
                queue = 0;

                return -2147467263; // E_NOTIMPL
            }

            public int Invoke(IMFAsyncResult asyncResult)
            {
                IMFMediaEvent mediaEvent = null;
                this.mediaSession.EndGetEvent(asyncResult, out mediaEvent);

                // Get the session event type
                uint type = Consts.MESessionUnknown;
                mediaEvent.GetType(out type);

                // Get the session event HRESULT
                int status = 0;
                mediaEvent.GetStatus(out status);

                // Fire the C# event 
                if (this.MediaEvent != null)
                {
                    this.MediaEvent(type, status);
                }

                // Get the next session event
                this.mediaSession.BeginGetEvent(this, null);

                return 0;
            }
        }
    }
}
