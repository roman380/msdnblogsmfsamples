// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

namespace MFManagedEncode.MediaFoundation.Com
{
    using System;
    using System.Runtime.InteropServices;
    using MFManagedEncode.MediaFoundation.Com.Interfaces;

    internal static class Enums
    {
        static Enums()
        {
        }

        [Flags]
        internal enum MF_SOURCE_READER_FLAG : uint
        {
            None = 0x00000000,
            ERROR = 0x00000001,
            ENDOFSTREAM = 0x00000002,
            NEWSTREAM = 0x00000004,
            NATIVEMEDIATYPECHANGED = 0x00000010,
            CURRENTMEDIATYPECHANGED = 0x00000020,
            STREAMTICK = 0x00000100
        }

        [Flags]
        internal enum MFSESSION_SETTOPOLOGY_FLAGS : uint
        {
            None                                  = 0x0,
            MFSESSION_SETTOPOLOGY_IMMEDIATE       = 0x1,
            MFSESSION_SETTOPOLOGY_NORESOLUTION    = 0x2,
            MFSESSION_SETTOPOLOGY_CLEAR_CURRENT   = 0x4 
        }

        [Flags]
        internal enum MFT_ENUM_FLAG : uint
        {
            None = 0,
            MFT_ENUM_FLAG_SYNCMFT = 0x00000001,
            MFT_ENUM_FLAG_ASYNCMFT = 0x00000002,
            MFT_ENUM_FLAG_HARDWARE = 0x00000004,
            MFT_ENUM_FLAG_FIELDOFUSE = 0x00000008,
            MFT_ENUM_FLAG_LOCALMFT = 0x00000010,
            MFT_ENUM_FLAG_TRANSCODE_ONLY = 0x00000020,
            MFT_ENUM_FLAG_SORTANDFILTER = 0x00000040,
            MFT_ENUM_FLAG_ALL = 0x0000003F
        }

        [Flags]
        internal enum MFSESSION_GETFULLTOPOLOGY_FLAGS : uint
        {
            None = 0,
            MF_SESSION_GETFULLTOPOLOGY_CURRENT = 1
        } 

    }
}
