// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

namespace MFManagedEncode.MediaFoundation
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;
    using MFManagedEncode.MediaFoundation.Com;
    using MFManagedEncode.MediaFoundation.Com.Classes;
    using MFManagedEncode.MediaFoundation.Com.Interfaces;

    /// <summary>
    ///     Encode content using Source Reader and Sink Writer
    /// </summary>
    internal class SimpleSinkWriterEncode
        : ISimpleEncode
    {
        private const double UPDATE_PROGRESS_INTERVAL_MS = 500;

        private System.ComponentModel.BackgroundWorker encodeWorker;

        private AudioFormat audioOutput;
        private VideoFormat videoOutput;

        private ulong startTime;
        private ulong endTime;

        private uint selectedStreams;

        private string sourceFilename;
        private string targetFilename;

        private IMFSourceReader sourceReader;
        private IMFSinkWriter sinkWriter;

        private Dictionary<uint, StreamInfo> streamsInfo;
        private long lastProgressUpdate;

        public SimpleSinkWriterEncode()
        {
            // Create objects and bind the background worker events
            this.EncodeProgress = null;
            this.lastProgressUpdate = DateTime.Now.Ticks;
            this.encodeWorker = new System.ComponentModel.BackgroundWorker();
            this.encodeWorker.WorkerReportsProgress = true;
            this.encodeWorker.WorkerSupportsCancellation = true;
            this.encodeWorker.DoWork += this.Encode_DoWork;
            this.encodeWorker.ProgressChanged += this.Encode_ProgressChanged;
            this.encodeWorker.RunWorkerCompleted += this.Encode_RunWorkerCompleted;

            this.streamsInfo = new Dictionary<uint, StreamInfo>();

            ClearAll();
        }

        public event EncodeProgressHandler EncodeProgress;

        public event EncodeErrorHandler EncodeError;

        public event EventHandler EncodeCompleted;

        // Stream information type
        private enum StreamType
        {
            UNKNOWN_STREAM = 0,
            AUDIO_STREAM = 1,
            VIDEO_STREAM = 2,
            NON_AV_STREAM = 3,
        }

        /// <summary>
        ///     Gets a value indicating whether the object is running an asynchronous operation.
        /// </summary>
        /// <returns>Value indicating whether the object is busy</returns>
        public bool IsBusy()
        {
            return this.encodeWorker.IsBusy;
        }

        /// <summary>
        ///     Requests cancellation of a background operation.
        /// </summary>
        public void CancelAsync()
        {
            this.encodeWorker.CancelAsync();
        }

        /// <summary>
        ///     Starts the encode operation 
        /// </summary>
        /// <param name="inputURL">Source filename</param>
        /// <param name="outputURL">Target filename</param>
        /// <param name="audioOutput">Audio format that will be used for audio streams</param>
        /// <param name="videoOutput">Video format that will be used for video streams</param>
        /// <param name="startPosition">Starting position of the new contet</param>
        /// <param name="endPosition">Position where the new content will end</param>
        public void Encode(string inputURL, string outputURL, AudioFormat audioOutput, VideoFormat videoOutput, ulong startPosition, ulong endPosition)
        {
            // If busy with other operation ignore and return
            if (this.IsBusy())
            {
                return;
            }

            this.sourceFilename = inputURL;
            this.targetFilename = outputURL;

            this.audioOutput = audioOutput;
            this.videoOutput = videoOutput;

            this.startTime = startPosition;
            this.endTime = endPosition;

            // Start the background worker with the arguments
            this.encodeWorker.RunWorkerAsync();
        }
        
        private void Encode_DoWork(object sender, System.ComponentModel.DoWorkEventArgs e)
        {
            try
            {
                // Create Source Reader and Sink Writer
                CreateReaderAndWriter();

                // Retrive the streams information
                RetrieveStreamsInformation();

                // Configure the streams
                ConfigureStreams();

                // Process the samples
                if (ProcessSamples() == false)
                {
                    e.Cancel = true;
                }

                ClearAll();
            }
            catch (Exception ex)
            {
                // Fire the EncodeError event
                if (this.EncodeError != null)
                {
                    this.EncodeError(new Exception(ex.Message, ex));
                }
            }
        }

        private void Encode_ProgressChanged(object sender, System.ComponentModel.ProgressChangedEventArgs e)
        {
            // The BackgroundWorker reports the progress using an integer, but we are using a double in WPF (from 0.0 to 100.0) so
            // to have a smooth transition this class will calculate the progress from 0 to 1000000000 and divide it by 10000000 
            // before reporting it back
            if (this.EncodeProgress != null)
            {
                this.EncodeProgress((double)e.ProgressPercentage / 10000000.0);
            }
        }

        private void Encode_RunWorkerCompleted(object sender, System.ComponentModel.RunWorkerCompletedEventArgs e)
        {
            if (this.EncodeCompleted != null)
            {
                this.EncodeCompleted(this, null);
            }
        }

        private void CreateReaderAndWriter()
        {
            object sourceReaderObject = null;
            object sinkWriterObject = null;
            IMFAttributes attributes = null;

            // Create the class factory
            IMFReadWriteClassFactory factory = (IMFReadWriteClassFactory)(new MFReadWriteClassFactory());

            // Create the attributes
            MFHelper.MFCreateAttributes(out attributes, 1);
            attributes.SetUINT32(new Guid(Consts.MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS), 1);

            // Use the factory to create the Source Reader
            factory.CreateInstanceFromURL(new Guid(CLSID.MFSourceReader), this.sourceFilename, attributes, new Guid(IID.IMFSourceReader), out sourceReaderObject);
            this.sourceReader = (IMFSourceReader)sourceReaderObject;
            
            // Use the factory to create the Sink Writer
            factory.CreateInstanceFromURL(new Guid(CLSID.MFSinkWriter), this.targetFilename, attributes, new Guid(IID.IMFSinkWriter), out sinkWriterObject);
            this.sinkWriter = (IMFSinkWriter)sinkWriterObject;
        }

        private void RetrieveStreamsInformation()
        {
            bool isSelected = false;
            IMFMediaType nativeMediaType = null;
            Guid majorType = Guid.Empty;

            this.selectedStreams = 0;
            this.streamsInfo.Clear();

            for (uint streamIndex = 0; true; streamIndex++)
            {
                try
                {
                    // Try to get the native media type
                    this.sourceReader.GetStreamSelection(streamIndex, out isSelected);

                    // Get the stream information and add it to the info object
                    this.streamsInfo.Add(streamIndex, new StreamInfo());

                    this.sourceReader.GetNativeMediaType(streamIndex, 0, out nativeMediaType);
                    nativeMediaType.GetGUID(new Guid(Consts.MF_MT_MAJOR_TYPE), out majorType);

                    if (majorType.Equals(new Guid(Consts.MFMediaType_Audio)))
                    {
                        this.streamsInfo[streamIndex].StreamType = StreamType.AUDIO_STREAM;
                    }
                    else if (majorType.Equals(new Guid(Consts.MFMediaType_Video)))
                    {
                        this.streamsInfo[streamIndex].StreamType = StreamType.VIDEO_STREAM;
                    }
                    else
                    {
                        this.streamsInfo[streamIndex].StreamType = StreamType.NON_AV_STREAM;
                        isSelected = false;
                        this.sourceReader.SetStreamSelection(streamIndex, isSelected);
                    }

                    this.streamsInfo[streamIndex].IsSelected = isSelected;

                    if (isSelected)
                    {
                        this.selectedStreams++;
                    }
                }
                catch (COMException ex)
                {
                    // If this error is thrown there are no more streams (0xC00D36B3 == MF_E_INVALIDSTREAMNUMBER)
                    if ((uint)ex.ErrorCode == 0xC00D36B3)
                    {
                        break;
                    }

                    throw new COMException(ex.Message, ex);
                }
            }
        }

        private void ConfigureStreams()
        {
            // Add all the selected streams
            for (uint streamIndex = 0; streamIndex < this.streamsInfo.Count; streamIndex++)
            {
                if (this.streamsInfo[streamIndex].IsSelected == false)
                {
                    continue;
                }

                AddStreamToSinkWriter(streamIndex);
            }
        }

        private void AddStreamToSinkWriter(uint streamIndex)
        {
            // Uncompressed formats that will be tried for sending samples from Source Reader to Sink Writer
            Guid[] possibleVideoFormats = { new Guid(Consts.MFVideoFormat_NV12), new Guid(Consts.MFVideoFormat_YV12), new Guid(Consts.MFVideoFormat_YUY2), new Guid(Consts.MFVideoFormat_RGB32) };
            Guid[] possibleAudioFormats = { new Guid(Consts.MFAudioFormat_PCM), new Guid(Consts.MFAudioFormat_Float) };

            if (this.streamsInfo[streamIndex].StreamType == StreamType.VIDEO_STREAM)
            {
                // Create the target video media type and add the stream to the Sink Writer
                this.sinkWriter.AddStream(CreateTargetVideoMediaType(), out streamsInfo[streamIndex].OutputStreamIndex);

                // Bind the Source Reader and Sink Writer streams
                NegotiateStream(streamIndex, new Guid(Consts.MFMediaType_Video), possibleVideoFormats);
            }
            else if (this.streamsInfo[streamIndex].StreamType == StreamType.AUDIO_STREAM)
            {
                // Create the target audio media type and add the stream to the Sink Writer
                this.sinkWriter.AddStream(CreateTargetAudioMediaType(), out streamsInfo[streamIndex].OutputStreamIndex);

                // Bind the Source Reader and Sink Writer streams
                NegotiateStream(streamIndex, new Guid(Consts.MFMediaType_Audio), possibleAudioFormats);
            }
        }

        private IMFMediaType CreateTargetVideoMediaType()
        {
            IMFMediaType mediaType = null;
            MFHelper.MFCreateMediaType(out mediaType);

            mediaType.SetGUID(new Guid(Consts.MF_MT_MAJOR_TYPE), new Guid(Consts.MFMediaType_Video));

            mediaType.SetGUID(new Guid(Consts.MF_MT_SUBTYPE), this.videoOutput.Subtype);
            mediaType.SetUINT64(new Guid(Consts.MF_MT_FRAME_SIZE), this.videoOutput.FrameSize.Packed);
            mediaType.SetUINT64(new Guid(Consts.MF_MT_FRAME_RATE), this.videoOutput.FrameRate.Packed);
            mediaType.SetUINT64(new Guid(Consts.MF_MT_PIXEL_ASPECT_RATIO), this.videoOutput.PixelAspectRatio.Packed);
            mediaType.SetUINT32(new Guid(Consts.MF_MT_AVG_BITRATE), this.videoOutput.AvgBitRate);
            mediaType.SetUINT32(new Guid(Consts.MF_MT_INTERLACE_MODE), this.videoOutput.InterlaceMode);

            return mediaType;
        }

        private IMFMediaType CreateTargetAudioMediaType()
        {
            IMFMediaType mediaType = null;

            if (audioOutput.Subtype.Equals(new Guid(Consts.MFAudioFormat_AAC)))
            {
                // Create the AAC media type
                MFHelper.MFCreateMediaType(out mediaType);

                mediaType.SetGUID(new Guid(Consts.MF_MT_MAJOR_TYPE), new Guid(Consts.MFMediaType_Audio));

                mediaType.SetGUID(new Guid(Consts.MF_MT_SUBTYPE), this.audioOutput.Subtype);
                mediaType.SetUINT32(new Guid(Consts.MF_MT_AUDIO_AVG_BYTES_PER_SECOND), this.audioOutput.AvgBytePerSecond);
                mediaType.SetUINT32(new Guid(Consts.MF_MT_AUDIO_NUM_CHANNELS), this.audioOutput.NumOfChannels);
                mediaType.SetUINT32(new Guid(Consts.MF_MT_AUDIO_SAMPLES_PER_SECOND), this.audioOutput.SamplesPerSecond);
                mediaType.SetUINT32(new Guid(Consts.MF_MT_AUDIO_BLOCK_ALIGNMENT), this.audioOutput.BlockAlignment);
                mediaType.SetUINT32(new Guid(Consts.MF_MT_AUDIO_BITS_PER_SAMPLE), this.audioOutput.BitsPerSample);
            }
            else
            {
                // Create the WMA media type
                uint codecConfig = 0;
                uint elementsNumber = 0;
                uint selectedType = 0;
                int avgBitrateDiff = int.MaxValue;
                uint avgBytePerSecond = uint.MaxValue;
                object supportedAttributes = null;

                // Get the available audio ouput types for the required sub type
                IMFCollection availableTypes = null;
                MFHelper.MFTranscodeGetAudioOutputAvailableTypes(audioOutput.Subtype, (uint)Enums.MFT_ENUM_FLAG.MFT_ENUM_FLAG_ALL, codecConfig, out availableTypes);

                // Get the number of types
                availableTypes.GetElementCount(out elementsNumber);

                for (uint elementIndex = 0; elementIndex < elementsNumber; elementIndex++)
                {
                    // Get the next element
                    availableTypes.GetElement(elementIndex, out supportedAttributes);
                    mediaType = (IMFMediaType)supportedAttributes;

                    // Get the byte per second
                    mediaType.GetUINT32(new Guid(Consts.MF_MT_AUDIO_AVG_BYTES_PER_SECOND), out avgBytePerSecond);

                    // If this is better than the last one found remember the index
                    if (Math.Abs((int)avgBytePerSecond - (int)audioOutput.AvgBytePerSecond) < avgBitrateDiff)
                    {
                        selectedType = elementIndex;
                        avgBitrateDiff = Math.Abs((int)avgBytePerSecond - (int)audioOutput.AvgBytePerSecond);
                    }

                    mediaType = null;
                }

                // Get the best audio type found
                availableTypes.GetElement(selectedType, out supportedAttributes);
                mediaType = (IMFMediaType)supportedAttributes;
            }

            return mediaType;
        }

        private void NegotiateStream(uint streamIndex, Guid majorType, Guid[] possibleSubtypes)
        {
            IMFMediaType outputMediaType = null;
            Exception lastException = null;
            bool foundValidType = false;

            foreach (Guid subType in possibleSubtypes)
            {
                try
                {
                    // Create a partial media type
                    MFHelper.MFCreateMediaType(out outputMediaType);
                    outputMediaType.SetGUID(new Guid(Consts.MF_MT_MAJOR_TYPE), majorType);
                    outputMediaType.SetGUID(new Guid(Consts.MF_MT_SUBTYPE), subType);

                    // Set it as the current media type in source reader
                    this.sourceReader.SetCurrentMediaType(streamIndex, IntPtr.Zero, outputMediaType);

                    // Get the full media type from Source Reader
                    this.sourceReader.GetCurrentMediaType(streamIndex, out outputMediaType);

                    // Set it as the input media type in Sink Writer
                    this.sinkWriter.SetInputMediaType(this.streamsInfo[streamIndex].OutputStreamIndex, outputMediaType, null);

                    // If no exceptions were thrown a suitable media type was found
                    foundValidType = true;

                    // Break from the loop so no more media types are tried
                    break;
                }
                catch (Exception ex)
                {
                    // Something went wrong, the loop will continue and the next type will be tried
                    lastException = ex;
                }
            }

            // If FormatFound is false no suitable type was found and the operation can't continue
            if (foundValidType == false)
            {
                if (lastException != null)
                {
                    throw new Exception(lastException.Message, lastException);
                }
                else
                {
                    throw new Exception("Unknown error occurred");
                }
            }
        }

        private bool ProcessSamples()
        {
            IMFMediaType mediaType = null;
            EncapsulatedSample sample = null;

            uint actualStreamIndex = 0;
            uint flags = 0;
            uint endOfStream = 0;
            ulong timestamp = 0;
            ulong sampleDuration = 0;

            // Set the start position
            object varPosition = (long)this.startTime;
            this.sourceReader.SetCurrentPosition(Guid.Empty, ref varPosition);
            this.sourceReader.Flush(Consts.MF_SOURCE_READER_ALL_STREAMS);

            // SourceReader's SetCurrentPosition don't guarantee the decoder will seek to
            // the exact requested position, so more samples are requested until the desired
            // position is reached
            do
            {
                // Dispose the unmanaged memory if needed
                if (sample != null)
                {
                    sample.Dispose();
                }

                sample = EncapsulatedSample.ReadSample(
                    this.sourceReader,
                    Consts.MF_SOURCE_READER_ANY_STREAM,
                    0,
                    out actualStreamIndex,
                    out flags,
                    out timestamp);

                // Throw an exception if the source reader reported an error
                if ((flags & (uint)Enums.MF_SOURCE_READER_FLAG.ERROR) != 0)
                {
                    throw new ApplicationException("IMFSourceReader::ReadSample reported an error (MF_SOURCE_READERF_ERROR)");  
                }

                // Get the sample duration
                if (sample.MfSample != null)
                {
                    sample.MfSample.GetSampleDuration(out sampleDuration);
                }
                else
                {
                    sampleDuration = 0;
                }
            }
            while (timestamp + sampleDuration < this.startTime);

            // Prepare for writing
            this.sinkWriter.BeginWriting();

            int progress = 0;
            this.lastProgressUpdate = DateTime.Now.Ticks;

            // Continue the operation until all streams are at the EOS
            while (endOfStream < this.selectedStreams)
            {
                // Cancel the operation if requested by the Background Worker
                if (this.encodeWorker.CancellationPending)
                {
                    return false;
                }

                // Read a new sample
                if (sample == null)
                {
                    sample = EncapsulatedSample.ReadSample(
                        this.sourceReader,
                        Consts.MF_SOURCE_READER_ANY_STREAM,
                        0,
                        out actualStreamIndex,
                        out flags,
                        out timestamp);
                }

                // Throw an exception if the source reader reported an error
                if ((flags & (uint)Enums.MF_SOURCE_READER_FLAG.ERROR) != 0)
                {
                    throw new ApplicationException("IMFSourceReader::ReadSample reported an error (MF_SOURCE_READERF_ERROR)");
                }

                // Report the progress if the interval is reached
                if (TimeSpan.FromTicks(DateTime.Now.Ticks - lastProgressUpdate).TotalMilliseconds > UPDATE_PROGRESS_INTERVAL_MS)
                {
                    // The BackgroundWorker reports the progress using an integer, but we are using a double in WPF (from 0.0 to 100.0) so
                    // to have a smooth transition this class will calculate the progress from 0 to 1000000000 and divide it by 10000000 
                    // before reporting it back
                    progress = (int)(((double)(timestamp - this.startTime) / (double)(this.endTime - this.startTime)) * 1000000000);
                    this.encodeWorker.ReportProgress(((progress > 1000000000) ? 1000000000 : progress));
                    lastProgressUpdate = DateTime.Now.Ticks;
                }

                // If the media type changed propagate it to the Sink Writer
                if ((flags & (uint)Enums.MF_SOURCE_READER_FLAG.CURRENTMEDIATYPECHANGED) != 0)
                {
                    this.sourceReader.GetCurrentMediaType(actualStreamIndex, out mediaType);
                    this.sinkWriter.SetInputMediaType(this.streamsInfo[actualStreamIndex].OutputStreamIndex, mediaType, null);
                }

                // Propagate streams tick
                if ((flags & (uint)Enums.MF_SOURCE_READER_FLAG.STREAMTICK) != 0)
                {
                    this.sinkWriter.SendStreamTick(this.streamsInfo[actualStreamIndex].OutputStreamIndex, timestamp - this.startTime);
                }

                if (sample.MfSample != null)
                {
                    // Fix the sample's timestamp
                    sample.MfSample.SetSampleTime(timestamp - this.startTime);

                    // Write the sample 
                    this.sinkWriter.WriteSample(this.streamsInfo[actualStreamIndex].OutputStreamIndex, sample.MfSample);
                }

                // If this was the last sample from the stream or we reached the ending position 
                if ((flags & (uint)Enums.MF_SOURCE_READER_FLAG.ENDOFSTREAM) != 0 || (this.endTime <= timestamp))
                {
                    // Send an end of segment to Sink Writer
                    this.sinkWriter.NotifyEndOfSegment(this.streamsInfo[actualStreamIndex].OutputStreamIndex);

                    // Disable the stream
                    this.sourceReader.SetStreamSelection(actualStreamIndex, false);

                    // One less stream left
                    endOfStream++;
                }

                // If this was the last stream then finalize the operation
                if (endOfStream == this.selectedStreams)
                {
                    this.encodeWorker.ReportProgress(1000000000);
                    this.sinkWriter.DoFinalize();
                }

                // Dispose the sample
                sample.Dispose();
                sample = null;
            }

            return true;
        }

        private void ClearAll()
        {
            this.streamsInfo.Clear();
            this.sourceFilename = string.Empty;
            this.targetFilename = string.Empty;
            this.sourceReader = null;
            this.sinkWriter = null;
            this.selectedStreams = 0;
        }

        // Stream information
        private class StreamInfo
        {
            public StreamType StreamType;
            public bool IsSelected;
            public uint OutputStreamIndex;
        }
    }
}
