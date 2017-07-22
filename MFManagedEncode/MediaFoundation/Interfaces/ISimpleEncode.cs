// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

namespace MFManagedEncode.MediaFoundation
{
    using System;

    internal delegate void EncodeProgressHandler(double Progress);

    internal delegate void EncodeErrorHandler(Exception e);

    /// <summary>
    ///     Encode content asychronously.
    /// </summary>
    /// <remarks>
    ///     Use the class events to get the operation status.
    /// </remarks>
    internal interface ISimpleEncode
    {
        event EventHandler EncodeCompleted;

        event EncodeErrorHandler EncodeError;

        event EncodeProgressHandler EncodeProgress;

        /// <summary>
        ///     Requests cancellation of a background operation.
        /// </summary>
        void CancelAsync();

        /// <summary>
        ///     Gets a value indicating whether the object is running an asynchronous operation.
        /// </summary>
        /// <returns>Value indicating whether the object is busy</returns>
        bool IsBusy();

        /// <summary>
        ///     Starts the asychronous encode operation 
        /// </summary>
        /// <param name="inputURL">Source filename</param>
        /// <param name="outputURL">Target filename</param>
        /// <param name="audio">Audio format that will be used for audio streams</param>
        /// <param name="video">Video format that will be used for video streams</param>
        /// <param name="startPosition">Starting position of the new contet</param>
        /// <param name="endPosition">Position where the new content will end</param>
        void Encode(string inputURL, string outputURL, AudioFormat audioOutput, VideoFormat videoOutput, ulong startPosition, ulong endPosition);
    }
}
