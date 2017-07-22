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
    using MFManagedEncode.MediaFoundation.Com.Interfaces;

    /// <summary>
    ///     Keeps track of the unmanaged memory used by samples
    /// </summary>
    internal class EncapsulatedSample
        : IDisposable
    {
        private IMFSample sample;

        private uint bufferSize;
        private bool disposed;
        private bool read;

        public EncapsulatedSample()
        {
            this.sample = null;
            this.bufferSize = 0;
            this.disposed = false;
        }

        ~EncapsulatedSample()
        {
            this.Dispose(false);
        }

        /// <summary>
        ///     Gets the real sample interface
        /// </summary>
        public IMFSample MfSample
        {
            get
            {
                return this.sample;
            }
        }

        /// <summary>
        ///     Use Source Reader to allocate a sample
        /// </summary>
        /// <param name="sourceReader">The Source Reader</param>
        /// <param name="dwStreamIndex">The stream to pull data from</param>
        /// <param name="dwControlFlags">A bitwise OR of zero or more flags from the MF_SOURCE_READER_CONTROL_FLAG enumeration</param>
        /// <param name="pdwActualStreamIndex">Receives the zero-based index of the stream</param>
        /// <param name="pdwStreamFlags">Receives a bitwise OR of zero or more flags from the MF_SOURCE_READER_FLAG enumeration</param>
        /// <param name="pllTimestamp">Receives the time stamp of the sample, or the time of the stream event indicated in pdwStreamFlags. The time is given in 100-nanosecond units</param>
        /// <returns>An encapsulated sample</returns>
        public static EncapsulatedSample ReadSample(
            IMFSourceReader sourceReader,
            uint dwStreamIndex,
            uint dwControlFlags,
            out uint pdwActualStreamIndex,
            out uint pdwStreamFlags,
            out ulong pllTimestamp)
        {
            EncapsulatedSample sample = new EncapsulatedSample();
            sample.ReadSampleFrom(
                sourceReader,
                dwStreamIndex,
                dwControlFlags,
                out pdwActualStreamIndex,
                out pdwStreamFlags,
                out pllTimestamp);

            return sample;
        }

        /// <summary>
        ///     Use Source Reader to allocate a sample
        /// </summary>
        /// <param name="sourceReader">The Source Reader</param>
        /// <param name="dwStreamIndex">The stream to pull data from</param>
        /// <param name="dwControlFlags">A bitwise OR of zero or more flags from the MF_SOURCE_READER_CONTROL_FLAG enumeration</param>
        /// <param name="pdwActualStreamIndex">Receives the zero-based index of the stream</param>
        /// <param name="pdwStreamFlags">Receives a bitwise OR of zero or more flags from the MF_SOURCE_READER_FLAG enumeration</param>
        /// <param name="pllTimestamp">Receives the time stamp of the sample, or the time of the stream event indicated in pdwStreamFlags. The time is given in 100-nanosecond units</param>
        public void ReadSampleFrom(
            IMFSourceReader sourceReader,
            uint dwStreamIndex,
            uint dwControlFlags,
            out uint pdwActualStreamIndex,
            out uint pdwStreamFlags,
            out ulong pllTimestamp)
        {
            // Once a sample is read avoid reading more samples
            if (this.read)
            {
                throw new InvalidOperationException("Already holding a sample");
            }

            sourceReader.ReadSample(dwStreamIndex, dwControlFlags, out pdwActualStreamIndex, out pdwStreamFlags, out pllTimestamp, out this.sample);

            if (this.sample != null)
            {
                // Get the size of the media sample in bytes
                IMFMediaBuffer buffer = null;
                this.sample.GetBufferByIndex(0, out buffer);
                buffer.GetCurrentLength(out this.bufferSize);

                // Add the memory pressure of unmanaged memory to improve the garbage collector performance
                GC.AddMemoryPressure(this.bufferSize);
            }

            this.read = true;
        }

        public void Dispose()
        {
            this.Dispose(true);
			GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            // If the object hasn't been disposed
            if (this.disposed == false)
            {
                if (this.bufferSize != 0)
                {
                    // Informs the Garbage Collector that the unmanaged memory has been released 
                    Marshal.FinalReleaseComObject(sample);
                    GC.RemoveMemoryPressure(this.bufferSize);
                }

                this.disposed = true;
            }
        }
    }
}
