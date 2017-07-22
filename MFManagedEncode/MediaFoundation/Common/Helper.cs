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
    using MFManagedEncode.MediaFoundation.Com;
    using MFManagedEncode.MediaFoundation.Com.Classes;
    using MFManagedEncode.MediaFoundation.Com.Interfaces;

    internal static class MFHelper
    {
        private static readonly ulong mediaFoundationVersion = 0x0270;

        [DllImport("mfplat.dll", EntryPoint = "MFStartup")]
        private static extern int ExternMFStartup(
            [In] ulong IVersion,
            [In] uint dwFlags);

        [DllImport("mfplat.dll", EntryPoint = "MFShutdown")]
        private static extern int ExternMFShutdown();

        [DllImport("mfplat.dll", EntryPoint = "MFCreateMediaType")]
        private static extern int ExternMFCreateMediaType(
            [Out, MarshalAs(UnmanagedType.Interface)] out IMFMediaType ppMFType);

        [DllImport("mfplat.dll", EntryPoint = "MFCreateSourceResolver")]
        private static extern int ExternMFCreateSourceResolver(
            [Out, MarshalAs(UnmanagedType.Interface)] out IMFSourceResolver ppISourceResolver);

        [DllImport("mfplat.dll", EntryPoint = "MFCreateAttributes")]
        private static extern int ExternMFCreateAttributes(
            [Out, MarshalAs(UnmanagedType.Interface)] out IMFAttributes ppMFAttributes,
            [In] uint cInitialSize);

        [DllImport("Mf.dll", EntryPoint = "MFCreateMediaSession")]
        private static extern int ExternMFCreateMediaSession(
            [In, MarshalAs(UnmanagedType.Interface)] IMFAttributes pConfiguration,
            [Out, MarshalAs(UnmanagedType.Interface)] out IMFMediaSession ppMS);

        [DllImport("Mf.dll", EntryPoint = "MFCreateTranscodeProfile")]
        private static extern int ExternMFCreateTranscodeProfile(
            [Out, MarshalAs(UnmanagedType.Interface)] out IMFTranscodeProfile ppTranscodeProfile);

        [DllImport("Mf.dll", EntryPoint = "MFTranscodeGetAudioOutputAvailableTypes")]
        private static extern int ExternMFTranscodeGetAudioOutputAvailableTypes(
            [In, MarshalAs(UnmanagedType.LPStruct)] Guid guidSubType,
            [In] uint dwMFTFlags,
            [In] uint pCodecConfig,
            [Out, MarshalAs(UnmanagedType.Interface)] out IMFCollection ppAvailableTypes);

        [DllImport("Mf.dll", EntryPoint = "MFCreateTranscodeTopology")]
        private static extern int ExternMFCreateTranscodeTopology(
            [In, MarshalAs(UnmanagedType.Interface)] IMFMediaSource pSrc,
            [In, MarshalAs(UnmanagedType.LPWStr)] string pwszOutputFilePath,
            [In, MarshalAs(UnmanagedType.Interface)] IMFTranscodeProfile pProfile,
            [Out, MarshalAs(UnmanagedType.Interface)] out IMFTopology ppTranscodeTopo);

        /// <summary>
        ///     Starts Media Foundation
        /// </summary>
        /// <remarks>
        ///     Will fail if the OS version is prior Windows 7
        /// </remarks>
        public static void MFStartup()
        {
            int result = ExternMFStartup(mediaFoundationVersion, 0);
            if (result < 0)
            {
                throw new COMException("Exception from HRESULT: 0x" + result.ToString("X", System.Globalization.NumberFormatInfo.InvariantInfo) + "(MFStartup)", result);
            }
        }
        
        public static void MFShutdown()
        {
            int result = ExternMFShutdown();
            if (result < 0)
            {
                throw new COMException("Exception from HRESULT: 0x" + result.ToString("X", System.Globalization.NumberFormatInfo.InvariantInfo) + " (MFShutdown)", result);
            }
        }

        public static void MFCreateMediaType(out IMFMediaType mediaType)
        {
            int result = ExternMFCreateMediaType(out mediaType);
            if (result < 0)
            {
                throw new COMException("Exception from HRESULT: 0x" + result.ToString("X", System.Globalization.NumberFormatInfo.InvariantInfo) + " (MFCreateMediaType)", result);
            }
        }

        public static void MFCreateSourceResolver(out IMFSourceResolver sourceResolver)
        {
            int result = ExternMFCreateSourceResolver(out sourceResolver);
            if (result < 0)
            {
                throw new COMException("Exception from HRESULT: 0x" + result.ToString("X", System.Globalization.NumberFormatInfo.InvariantInfo) + " (MFCreateSourceResolver)", result);
            }
        }

        public static void MFCreateAttributes(out IMFAttributes attributes, uint initialSize)
        {
            int result = ExternMFCreateAttributes(out attributes, initialSize);
            if (result < 0)
            {
                throw new COMException("Exception from HRESULT: 0x" + result.ToString("X", System.Globalization.NumberFormatInfo.InvariantInfo) + " (MFCreateAttributes)", result);
            }
        }

        public static void MFCreateMediaSession(IMFAttributes configuration, out IMFMediaSession mediaSession)
        {
            int result = ExternMFCreateMediaSession(configuration, out mediaSession);
            if (result < 0)
            {
                throw new COMException("Exception from HRESULT: 0x" + result.ToString("X", System.Globalization.NumberFormatInfo.InvariantInfo) + " (MFCreateMediaSession)", result);
            }
        }

        public static void MFCreateTranscodeProfile(out IMFTranscodeProfile transcodeProfile)
        {
            int result = ExternMFCreateTranscodeProfile(out transcodeProfile);
            if (result < 0)
            {
                throw new COMException("Exception from HRESULT: 0x" + result.ToString("X", System.Globalization.NumberFormatInfo.InvariantInfo) + " (MFCreateTranscodeProfile)", result);
            }
        }

        public static void MFTranscodeGetAudioOutputAvailableTypes(
            Guid subType,
            uint flags,
            uint codecConfig,
            out IMFCollection availableTypes)
        {
            int result = ExternMFTranscodeGetAudioOutputAvailableTypes(subType, flags, codecConfig, out availableTypes);
            if (result != 0)
            {
                throw new COMException("Exception from HRESULT: 0x" + result.ToString("X", System.Globalization.NumberFormatInfo.InvariantInfo) + "(MFTranscodeGetAudioOutputAvailableTypes)", result);
            }
        }

        public static void MFCreateTranscodeTopology(
            IMFMediaSource mediaSource,
            string outputFilePath,
            IMFTranscodeProfile transcodeProfile,
            out IMFTopology transcodeTopology)
        {
            int result = ExternMFCreateTranscodeTopology(mediaSource, outputFilePath, transcodeProfile, out transcodeTopology);
            if (result < 0)
            {
                throw new COMException("Exception from HRESULT: 0x" + result.ToString("X", System.Globalization.NumberFormatInfo.InvariantInfo) + " (MFCreateTranscodeTopology failed)", result);
            }
        }
    }
}