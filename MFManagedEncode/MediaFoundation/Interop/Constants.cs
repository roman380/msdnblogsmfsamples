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

    internal static class CLSID
    {
        static CLSID()
        {
        }

        internal const string MFReadWriteClassFactory = "48e2ed0f-98c2-4a37-bed5-166312ddd83f";
        internal const string MFSinkWriter = "a3bbfb17-8273-4e52-9e0e-9739dc887990";
        internal const string MFSourceReader = "1777133c-0881-411b-a577-ad545f0714c4";
        internal const string MFSourceResolver = "90eab60f-e43a-4188-bcc4-e47fdf04868c";
    }

    internal static class IID
    {
        static IID()
        {
        }

        internal const string IMFAttributes = "2cd2d921-c447-44a7-a13c-4adabfc247e3";
        internal const string IMFClock = "2EB1E945-18B8-4139-9B1A-D5D584818530";
        internal const string IMFCollection = "5BC8A76B-869A-46A3-9B03-FA218A66AEBE";
        internal const string IMFMediaBuffer = "045FA593-8799-42B8-BC8D-8968C6453507";
        internal const string IMFMediaEvent = "DF598932-F10C-4E39-BBA2-C308F101DAA3";
        internal const string IMFMediaSession = "90377834-21D0-4DEE-8214-BA2E3E6C1127";
        internal const string IMFMediaSource = "279A808D-AEC7-40C8-9C6B-A6B492C78A66";
        internal const string IMFMediaType = "44AE0FA8-EA31-4109-8D2E-4CAE4997C555";
        internal const string IMFPMediaItem = "90EB3E6B-ECBF-45cc-B1DA-C6FE3EA70D57";
        internal const string IMFPMediaPlayer = "A714590A-58AF-430a-85BF-44F5EC838D85";
        internal const string IMFReadWriteClassFactory = "E7FE2E12-661C-40DA-92F9-4F002AB67627";
        internal const string IMFSample = "C40A00F2-B93A-4D80-AE8C-5A1C634F58E4";
        internal const string IMFSinkWriter = "3137f1cd-fe5e-4805-a5d8-fb477448cb3d";
        internal const string IMFSourceReader = "70ae66f2-c809-4e4f-8915-bdcb406b7993";
        internal const string IMFSourceResolver = "FBE5A32D-A497-4B61-BB85-97B1A848A6E3";
        internal const string IMFTopology = "83CF873A-F6DA-4BC8-823F-BACFD55DC433";
        internal const string IMFTopologyNode = "83CF873A-F6DA-4bc8-823F-BACFD55DC430";
        internal const string IMFTranscodeProfile = "4ADFDBA3-7AB0-4953-A62B-461E7FF3DA1E";
        internal const string IMFPMediaPlayerCallback = "766C8FFB-5FDB-4fea-A28D-B912996F51BD";
        internal const string IMFAsyncCallback = "a27003cf-2354-4f2a-8d6a-ab7cff15437e";
        internal const string IMFAsyncResult = "ac6b7889-0740-4d51-8619-905994a55cc6";
        internal const string IMFPresentationClock = "868CE85C-8EA9-4f55-AB82-B009A910A805";
        internal const string IMFPresentationDescriptor = "03CB2711-24D7-4DB6-A17F-F3A7A479A536";
        internal const string IMFStreamDescriptor = "56C03D9C-9DBB-45F5-AB4B-D80F47C05938";
        internal const string IMFMediaTypeHandler = "E93DCF6C-4B07-4E1E-8123-AA16ED6EADF5";
        internal const string IPropertyStore = "886D8EEB-8CF2-4446-8D02-CDBA1DBDCF99";
    }

    internal static class Consts
    {
        static Consts()
        {
        }

        internal const string MFMediaType_Audio = "73647561-0000-0010-8000-00aa00389b71";
        internal const string MFMediaType_Video = "73646976-0000-0010-8000-00aa00389b71";
        internal const string MFMediaType_Protected = "7b4b6fe6-9d04-4494-be14-7e0bd076c8e4";
        internal const string MFMediaType_SAMI = "e69669a0-3dcd-40cb-9e2e-3708387c0616";
        internal const string MFMediaType_Script = "72178c22-e45b-11d5-bc2a-00b0d0f3f4ab";
        internal const string MFMediaType_Image = "72178c23-e45b-11d5-bc2a-00b0d0f3f4ab";
        internal const string MFMediaType_HTML = "72178c24-e45b-11d5-bc2a-00b0d0f3f4ab";
        internal const string MFMediaType_Binary = "72178c25-e45b-11d5-bc2a-00b0d0f3f4ab";
        internal const string MFMediaType_FileTransfer = "72178c26-e45b-11d5-bc2a-00b0d0f3f4ab";

        internal const string MFAudioFormat_AAC = "00001610-0000-0010-8000-00aa00389b71";
        internal const string MFAudioFormat_ADTS = "00001600-0000-0010-8000-00aa00389b71";
        internal const string MFAudioFormat_Dolby_AC3_SPDIF = "00000092-0000-0010-8000-00aa00389b71";
        internal const string MFAudioFormat_DRM = "00000009-0000-0010-8000-00aa00389b71";
        internal const string MFAudioFormat_DTS = "00000008-0000-0010-8000-00aa00389b71";
        internal const string MFAudioFormat_Float = "00000003-0000-0010-8000-00aa00389b71";
        internal const string MFAudioFormat_MP3 = "00000055-0000-0010-8000-00aa00389b71";
        internal const string MFAudioFormat_MPEG = "00000050-0000-0010-8000-00aa00389b71";
        internal const string MFAudioFormat_MSP1 = "0000000a-0000-0010-8000-00aa00389b71";
        internal const string MFAudioFormat_PCM = "00000001-0000-0010-8000-00aa00389b71";
        internal const string MFAudioFormat_WMASPDIF = "00000164-0000-0010-8000-00aa00389b71";
        internal const string MFAudioFormat_WMAudio_Lossless = "00000163-0000-0010-8000-00aa00389b71";
        internal const string MFAudioFormat_WMAudioV8 = "00000161-0000-0010-8000-00aa00389b71";
        internal const string MFAudioFormat_WMAudioV9 = "00000162-0000-0010-8000-00aa00389b71";

        internal const string MFTranscodeContainerType_ASF = "430f6f6e-b6bf-4fc1-a0bd-9ee46eee2afb";
        internal const string MFTranscodeContainerType_MPEG4 = "dc6cd05d-b9d0-40ef-bd35-fa622c1ab28a";
        internal const string MFTranscodeContainerType_MP3 = "e438b912-83f1-4de6-9e3a-9ffbc6dd24d1";
        internal const string MFTranscodeContainerType_3GP = "34c50167-4472-4f34-9ea0-c49fbacf037d";

        internal const string MFVideoFormat_ARGB32 = "00000021-0000-0010-8000-00aa00389b71";
        internal const string MFVideoFormat_RGB24 = "00000020-0000-0010-8000-00aa00389b71";
        internal const string MFVideoFormat_RGB32 = "00000022-0000-0010-8000-00aa00389b71";
        internal const string MFVideoFormat_RGB555 = "00000024-0000-0010-8000-00aa00389b71";
        internal const string MFVideoFormat_RGB565 = "00000023-0000-0010-8000-00aa00389b71";
        internal const string MFVideoFormat_AYUV = "56555941-0000-0010-8000-00aa00389b71";
        internal const string MFVideoFormat_NV11 = "3131564e-0000-0010-8000-00aa00389b71";
        internal const string MFVideoFormat_NV12 = "3231564e-0000-0010-8000-00aa00389b71";
        internal const string MFVideoFormat_UYVY = "59565955-0000-0010-8000-00aa00389b71";
        internal const string MFVideoFormat_YUY2 = "32595559-0000-0010-8000-00aa00389b71";
        internal const string MFVideoFormat_YV12 = "32315659-0000-0010-8000-00aa00389b71";
        internal const string MFVideoFormat_H264 = "34363248-0000-0010-8000-00aa00389b71";
        internal const string MFVideoFormat_WMV1 = "31564d57-0000-0010-8000-00aa00389b71";
        internal const string MFVideoFormat_WMV2 = "32564d57-0000-0010-8000-00aa00389b71";
        internal const string MFVideoFormat_WMV3 = "33564d57-0000-0010-8000-00aa00389b71";
        internal const string MFVideoFormat_WVC1 = "31435657-0000-0010-8000-00aa00389b71";

        internal const string MF_TOPONODE_MEDIASTART = "835c58ea-e075-4bc7-bcba-4de000df9ae6";
        internal const string MF_TOPONODE_MEDIASTOP = "835c58eb-e075-4bc7-bcba-4de000df9ae6";
        internal const string MF_TOPONODE_MARKIN_HERE = "494bbd00-b031-4e38-97c4-d5422dd618dc";
        internal const string MF_TOPONODE_MARKOUT_HERE = "494bbd01-b031-4e38-97c4-d5422dd618dc";

        internal const string MF_TOPOLOGY_PROJECTSTART = "7ed3f802-86bb-4b3f-b7e4-7cb43afd4b80";
        internal const string MF_TOPOLOGY_PROJECTSTOP = "7ed3f803-86bb-4b3f-b7e4-7cb43afd4b80";
        internal const string MF_TOPOLOGY_NO_MARKIN_MARKOUT = "7ed3f804-86bb-4b3f-b7e4-7cb43afd4b80";
        internal const string MF_TOPOLOGY_DXVA_MODE = "1e8d34f6-f5ab-4e23-bb88-874aa3a1a74d";
        internal const string MF_TOPOLOGY_STATIC_PLAYBACK_OPTIMIZATIONS = "b86cac42-41a6-4b79-897a-1ab0e52b4a1b";
        internal const string MF_TOPOLOGY_PLAYBACK_MAX_DIMS = "5715cf19-5768-44aa-ad6e-8721f1b0f9bb";
        internal const string MF_TOPOLOGY_HARDWARE_MODE = "d2d362fd-4e4f-4191-a579-c618b6676af";
        internal const string MF_TOPOLOGY_PLAYBACK_FRAMERATE = "c164737a-c2b1-4553-83bb-5a526072448f";
        internal const string MF_TOPOLOGY_DYNAMIC_CHANGE_NOT_ALLOWED = "d529950b-d484-4527-a9cd-b1909532b5b0";
        internal const string MF_TOPOLOGY_ENUMERATE_SOURCE_TYPES = "6248c36d-5d0b-4f40-a0bb-b0b305f77698";
        internal const string MF_TOPOLOGY_START_TIME_ON_PRESENTATION_SWITCH = "c8cc113f-7951-4548-aad6-9ed6202e62b3";

        internal const string MF_PD_PMPHOST_CONTEXT = "6c990d31-bb8e-477a-8598-0d5d96fcd88a";
        internal const string MF_PD_APP_CONTEXT = "6c990d32-bb8e-477a-8598-0d5d96fcd88a";
        internal const string MF_PD_DURATION = "6c990d33-bb8e-477a-8598-0d5d96fcd88a";
        internal const string MF_PD_TOTAL_FILE_SIZE = "6c990d34-bb8e-477a-8598-0d5d96fcd88a";
        internal const string MF_PD_AUDIO_ENCODING_BITRATE = "6c990d35-bb8e-477a-8598-0d5d96fcd88a";
        internal const string MF_PD_VIDEO_ENCODING_BITRATE = "6c990d36-bb8e-477a-8598-0d5d96fcd88a";
        internal const string MF_PD_MIME_TYPE = "6c990d37-bb8e-477a-8598-0d5d96fcd88a";
        internal const string MF_PD_LAST_MODIFIED_TIME = "6c990d38-bb8e-477a-8598-0d5d96fcd88a";
        internal const string MF_PD_PLAYBACK_ELEMENT_ID = "6c990d39-bb8e-477a-8598-0d5d96fcd88a";
        internal const string MF_PD_PREFERRED_LANGUAGE = "6c990d3a-bb8e-477a-8598-0d5d96fcd88a";
        internal const string MF_PD_PLAYBACK_BOUNDARY_TIME = "6c990d3b-bb8e-477a-8598-0d5d96fcd88a";
        internal const string MF_PD_AUDIO_ISVARIABLEBITRATE = "33026ee0-e387-4582-ae0a-34a2ad3baa18";

        internal const string MF_TRANSCODE_ENCODINGPROFILE = "6947787c-f508-4ea9-b1e9-a1fe3a49fbc9";
        internal const string MF_TRANSCODE_QUALITYVSSPEED = "98332df8-03cd-476b-89fa-3f9e442dec9f";
        internal const string MF_TRANSCODE_CONTAINERTYPE = "150ff23f-4abc-478b-ac4f-e1916fba1cca";
        internal const string MF_TRANSCODE_TOPOLOGYMODE = "3E3DF610-394A-40B2-9DEA-3BAB650BEBF2";

        internal const string MF_MT_MAJOR_TYPE = "48eba18e-f8c9-4687-bf11-0a74c9f96a8f";
        internal const string MF_MT_SUBTYPE = "f7e34c9a-42e8-4714-b74b-cb29d72c35e5";
        internal const string MF_MT_ALL_SAMPLES_INDEPENDENT = "c9173739-5e56-461c-b713-46fb995cb95f";
        internal const string MF_MT_FIXED_SIZE_SAMPLES = "b8ebefaf-b718-4e04-b0a9-116775e3321b";
        internal const string MF_MT_COMPRESSED = "3afd0cee-18f2-4ba5-a110-8bea502e1f92";
        internal const string MF_MT_SAMPLE_SIZE = "dad3ab78-1990-408b-bce2-eba673dacc10";
        internal const string MF_MT_WRAPPED_TYPE = "4d3f7b23-d02f-4e6c-9bee-e4bf2c6c695d";
        internal const string MF_MT_USER_DATA = "b6bc765f-4c3b-40a4-bd51-2535b66fe09d";

        internal const string MF_MT_AUDIO_NUM_CHANNELS = "37e48bf5-645e-4c5b-89de-ada9e29b696a";
        internal const string MF_MT_AUDIO_SAMPLES_PER_SECOND = "5faeeae7-0290-4c31-9e8a-c534f68d9dba";
        internal const string MF_MT_AUDIO_FLOAT_SAMPLES_PER_SECOND = "fb3b724a-cfb5-4319-aefe-6e42b2406132";
        internal const string MF_MT_AUDIO_AVG_BYTES_PER_SECOND = "1aab75c8-cfef-451c-ab95-ac034b8e1731";
        internal const string MF_MT_AUDIO_BLOCK_ALIGNMENT = "322de230-9eeb-43bd-ab7a-ff412251541d";
        internal const string MF_MT_AUDIO_BITS_PER_SAMPLE = "f2deb57f-40fa-4764-aa33-ed4f2d1ff669";
        internal const string MF_MT_AUDIO_VALID_BITS_PER_SAMPLE = "d9bf8d6a-9530-4b7c-9ddf-ff6fd58bbd06";
        internal const string MF_MT_AUDIO_SAMPLES_PER_BLOCK = "aab15aac-e13a-4995-9222-501ea15c6877";
        internal const string MF_MT_AUDIO_CHANNEL_MASK = "55fb5765-644a-4caf-8479-938983bb1588";
        internal const string MF_MT_AUDIO_FOLDDOWN_MATRIX = "9d62927c-36be-4cf2-b5c4-a3926e3e8711";
        internal const string MF_MT_AUDIO_WMADRC_PEAKREF = "9d62927d-36be-4cf2-b5c4-a3926e3e8711";
        internal const string MF_MT_AUDIO_WMADRC_PEAKTARGET = "9d62927e-36be-4cf2-b5c4-a3926e3e8711";
        internal const string MF_MT_AUDIO_WMADRC_AVGREF = "9d62927f-36be-4cf2-b5c4-a3926e3e8711";
        internal const string MF_MT_AUDIO_WMADRC_AVGTARGET = "9d629280-36be-4cf2-b5c4-a3926e3e8711";
        internal const string MF_MT_AUDIO_PREFER_WAVEFORMATEX = "a901aaba-e037-458a-bdf6-545be2074042";
        internal const string MF_MT_AAC_PAYLOAD_TYPE = "bfbabe79-7434-4d1c-94f0-72a3b9e17188";
        internal const string MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION = "7632f0e6-9538-4d61-acda-ea29c8c14456";

        internal const string MF_MT_FRAME_SIZE = "1652c33d-d6b2-4012-b834-72030849a37d";
        internal const string MF_MT_FRAME_RATE = "c459a2e8-3d2c-4e44-b132-fee5156c7bb0";
        internal const string MF_MT_PIXEL_ASPECT_RATIO = "c6376a1e-8d0a-4027-be45-6d9a0ad39bb6";
        internal const string MF_MT_DRM_FLAGS = "8772f323-355a-4cc7-bb78-6d61a048ae82";
        internal const string MF_MT_PAD_CONTROL_FLAGS = "4d0e73e5-80ea-4354-a9d0-1176ceb028ea";
        internal const string MF_MT_SOURCE_CONTENT_HINT = "68aca3cc-22d0-44e6-85f8-28167197fa38";
        internal const string MF_MT_INTERLACE_MODE = "e2724bb8-e676-4806-b4b2-a8d6efb44ccd";
        internal const string MF_MT_TRANSFER_FUNCTION = "5fb0fce9-be5c-4935-a811-ec838f8eed93";
        internal const string MF_MT_CUSTOM_VIDEO_PRIMARIES = "47537213-8cfb-4722-aa34-fbc9e24d77b8";
        internal const string MF_MT_YUV_MATRIX = "3e23d450-2c75-4d25-a00e-b91670d12327";
        internal const string MF_MT_GEOMETRIC_APERTURE = "66758743-7e5f-400d-980a-aa8596c85696";
        internal const string MF_MT_MINIMUM_DISPLAY_APERTURE = "d7388766-18fe-48c6-a177-ee894867c8c4";
        internal const string MF_MT_PAN_SCAN_APERTURE = "79614dde-9187-48fb-b8c7-4d52689de649";
        internal const string MF_MT_PAN_SCAN_ENABLED = "4b7f6bc3-8b13-40b2-a993-abf630b8204e";
        internal const string MF_MT_AVG_BITRATE = "20332624-fb0d-4d9e-bd0d-cbf6786c102e";
        internal const string MF_MT_AVG_BIT_ERROR_RATE = "799cabd6-3508-4db4-a3c7-569cd533deb1";
        internal const string MF_MT_MAX_KEYFRAME_SPACING = "c16eb52b-73a1-476f-8d62-839d6a020652";
        internal const string MF_MT_MPEG4_SAMPLE_DESCRIPTION = "261e9d83-9529-4b8f-a111-8b9c950a81a9";
        internal const string MF_MT_MPEG4_CURRENT_SAMPLE_ENTRY = "9aa7e155-b64a-4c1d-a500-455d600b6560";
        internal const string MF_MT_VIDEO_CHROMA_SITING = "65df2370-c773-4c33-aa64-843e068efb0c";
        internal const string MF_MT_VIDEO_PRIMARIES = "dbfbe4d7-0740-4ee0-8192-850ab0e21935";
        internal const string MF_MT_VIDEO_LIGHTING = "53a0529c-890b-4216-8bf9-599367ad6d20";
        internal const string MF_MT_VIDEO_NOMINAL_RANGE = "c21b8ee5-b956-4071-8daf-325edf5cab11";

        internal const string MF_SOURCE_READER_ASYNC_CALLBACK = "1e3dbeac-bb43-4c35-b507-cd644464c965";
        internal const string MF_SOURCE_READER_D3D_MANAGER = "ec822da2-e1e9-4b29-a0d8-563c719f5269";
        internal const string MF_SOURCE_READER_DISABLE_DXVA = "aa456cfd-3943-4a1e-a77d-1838c0ea2e35";
        internal const string MF_SOURCE_READER_MEDIASOURCE_CONFIG = "9085abeb-0354-48f9-abb5-200df838c68e";
        internal const string MF_SOURCE_READER_MEDIASOURCE_CHARACTERISTICS = "6d23f5c8-c5d7-4a9b-9971-5d11f8bca880";
        internal const string MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING = "fb394f3d-ccf1-42ee-bbb3-f9b845d5681d";
        internal const string MF_SOURCE_READER_DISCONNECT_MEDIASOURCE_ON_SHUTDOWN = "56b67165-219e-456d-a22e-2d3004c7fe56";

        internal const string MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS = "a634a91c-822b-41b9-a494-4de4643612b0";

        internal const string MF_SESSION_TOPOLOADER = "1e83d482-1f1c-4571-8405-88f4b2181f71";
        internal const string MF_SESSION_GLOBAL_TIME = "1e83d482-1f1c-4571-8405-88f4b2181f72";
        internal const string MF_SESSION_QUALITY_MANAGER = "1e83d482-1f1c-4571-8405-88f4b2181f73";

        internal const uint MF_SOURCE_READER_MEDIASOURCE = 0xFFFFFFFF;
        internal const uint MF_SOURCE_READER_ANY_STREAM = 0xFFFFFFFE;
        internal const uint MF_SOURCE_READER_FIRST_AUDIO_STREAM = 0xFFFFFFFD;
        internal const uint MF_SOURCE_READER_FIRST_VIDEO_STREAM = 0xFFFFFFFC;
        internal const uint MF_SOURCE_READER_ALL_STREAMS = 0xFFFFFFFE;

        internal const uint MF_RESOLUTION_MEDIASOURCE = 0x00000001;
        internal const uint MF_RESOLUTION_BYTESTREAM = 0x00000002; 
        internal const uint MF_RESOLUTION_CONTENT_DOES_NOT_HAVE_TO_MATCH_EXTENSION_OR_MIME_TYPE  = 0x00000010; 
        internal const uint MF_RESOLUTION_KEEP_BYTE_STREAM_ALIVE_ON_FAIL = 0x00000020; 
        internal const uint MF_RESOLUTION_READ = 0x00010000;
        internal const uint MF_RESOLUTION_WRITE = 0x00020000; 

        internal const uint MEError = 1;
        internal const uint MEExtendedType = 2;
        internal const uint MENonFatalError = 3;
        internal const uint MESessionUnknown = 100;
        internal const uint MESessionTopologySet = 101;
        internal const uint MESessionTopologiesCleared = 102;
        internal const uint MESessionStarted = 103;
        internal const uint MESessionPaused = 104;
        internal const uint MESessionStopped = 105;
        internal const uint MESessionClosed = 106;
        internal const uint MESessionEnded = 107;
    }
}
