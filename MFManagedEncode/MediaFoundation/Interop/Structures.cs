// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

namespace MFManagedEncode.MediaFoundation.Com.Structures
{
    using System;
    using System.Runtime.InteropServices;
    using MFManagedEncode.MediaFoundation.Com.Interfaces;

    [StructLayout(LayoutKind.Explicit)]
    internal class MediaSessionStartPosition
    {
        public MediaSessionStartPosition(long startPosition)
        {
            this.internalUse = 20; // VT_18
            this.startPosition = startPosition;
        }

        [FieldOffset(0)]
        public short internalUse;
        [FieldOffset(8)]
        public long startPosition;
    }
}

