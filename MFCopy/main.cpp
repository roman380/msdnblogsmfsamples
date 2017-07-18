// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "stdafx.h"
#include "MFCopy.h"

HRESULT ParseCommandLine( __in int argc,
                          __in_ecount( argc ) WCHAR *argv[],
                          __in CMFCopy *pMFCopy,
                          __out LPCWSTR *pwszSourceFilename,
                          __out LPCWSTR *pwszTargetFilename,
                          __out BOOL *pfCopyMetadata,
                          __out BOOL *pfQuietMode );

void DisplayUsage();

void DisplayErrorDetails( __in CMFCopy *pMFCopy );

void DisplayStatistics( __in CMFCopy *pMFCopy );

void ProgressCallback( __in DWORD dwPercentComplete,
                       __in_opt LPVOID pvContext );

HRESULT CopyMetadata( __in LPCWSTR wszSourceFilename,
                      __in LPCWSTR wszTargetFilename );

volatile BOOL g_fProgressBannerDisplayed = FALSE;

///////////////////////////////////////////////////////////////////////////////
int __cdecl wmain(
    __in int argc,
    __in_ecount( argc ) WCHAR *argv[] )
{
    HRESULT hr = S_OK;
    BOOL fCoInitCalled = FALSE;
    BOOL fMFStartupCalled = FALSE;

    LPCWSTR wszSourceFilename = NULL;
    LPCWSTR wszTargetFilename = NULL;
    BOOL fCopyMetadata = TRUE;
    BOOL fQuietMode = FALSE;

    CMFCopy *pMFCopy = NULL;

    HeapSetInformation( NULL, HeapEnableTerminationOnCorruption, NULL, 0 );

    CHECK_HR( hr = CoInitializeEx( NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE ) );
    fCoInitCalled = TRUE;

    CHECK_HR( hr = MFStartup( MF_VERSION ) );
    fMFStartupCalled = TRUE;

    CHECK_HR( hr = CMFCopy::CreateInstance( &pMFCopy ) );

    hr = ParseCommandLine( argc,
                           argv,
                           pMFCopy,
                           &wszSourceFilename,
                           &wszTargetFilename,
                           &fCopyMetadata,
                           &fQuietMode );
    if( S_OK != hr )
    {
        DisplayUsage();
        goto done;
    }

    if( !fQuietMode )
    {
        pMFCopy->SetProgressCallback( ProgressCallback, NULL );
    }

    hr = pMFCopy->Copy( wszSourceFilename, wszTargetFilename );

    if( !fQuietMode && g_fProgressBannerDisplayed )
    {
        printf( "\n" );
    }

    if( S_OK != hr )
    {
        if( !fQuietMode )
        {
            DisplayErrorDetails( pMFCopy );
        }
        goto done;
    }

    if( !fQuietMode )
    {
        DisplayStatistics( pMFCopy );
    }

    if( fCopyMetadata )
    {
        //
        // release the copier to ensure that all file handles
        // to the source/dest files have been closed
        //
        SAFE_RELEASE( pMFCopy );

        hr = CopyMetadata( wszSourceFilename, wszTargetFilename );
        if( S_OK != hr )
        {
            if( !fQuietMode )
            {
                fprintf( stderr, "WARNING: failed to copy metadata - hr=0x%08x\n", hr );
            }
            hr = S_OK;
        }
    }

done:

    SAFE_RELEASE( pMFCopy );

    if( fMFStartupCalled )
    {
        MFShutdown();
    }

    if( fCoInitCalled )
    {
        CoUninitialize();
    }

    return( ( S_OK == hr ) ? 0 : 1 );
}

///////////////////////////////////////////////////////////////////////////////
void DisplayUsage()
{
    printf( "Usage:\n\n"
            "    mfcopy.exe [-?]\n"
            "               [-a audio format] [-nc channels] [-sr rate]\n"
            "               [-v video format] [-fs WxH] [-fr N:D] [-im mode] [-br rate]\n"
            "               [-q] [-s ms] [-d ms] [-hw]\n"
            "               [-xa] [-xv] [-xo] [-xm]\n"
            "               <input filename> <output filename>\n\n"
            "     Audio transcode options:\n"
            "        -a   :  set the audio format (defaults to native format)\n"
            "                  eg. AAC, WMAudioV8, WMAudioV9, WMAudio_Lossless\n"
            "        -nc  :  set number of channels\n"
            "        -sr  :  resample to the specified rate\n\n"
            "     Video transcode options:\n"
            "        -v   :  set the video format (defaults to native format)\n"
            "                  eg. H264, WMV2, WMV3, WVC1\n"
            "        -fs  :  resize to WxH\n"
            "        -fr  :  convert frame rate to N:D\n"
            "        -im  :  set interlace mode (one of the MFVideoInterlace settings)\n"
            "        -br  :  set average bitrate\n\n"
            "    Other options:\n"
            "        -?   :  display usage information\n"
            "        -q   :  quiet mode\n"
            "        -s   :  specify a start position in milliseconds\n"
            "        -d   :  specify a duration in milliseconds\n"
            "        -hw  :  enable hardware transforms\n"
            "        -xa  :  exclude audio streams\n"
            "        -xv  :  exclude video streams\n"
            "        -xo  :  exclude non audio/video streams\n"
            "        -xm  :  exclude metadata\n\n"
            "    Examples:\n\n"
            "        Remux 20 seconds of a WMV file:\n"
            "            mfcopy.exe -d 20000 input.wmv output.wmv\n\n"
            "        Transcode WMV to MP4 and resize to 320x240:\n"
            "            mfcopy.exe -a AAC -v H264 -fs 320x240 input.wmv output.mp4\n\n" );
}

///////////////////////////////////////////////////////////////////////////////
void DisplayErrorDetails( __in CMFCopy *pMFCopy )
{
    HRESULT hr;
    LPCWSTR wszError;

    pMFCopy->GetErrorDetails( &hr, &wszError );

    fprintf( stderr, "ERROR: %S - hr=0x%08x\n", wszError, hr );
}

///////////////////////////////////////////////////////////////////////////////
struct FormatInfo
{
    WCHAR wszName[20];
    GUID guidSubtype;
};

///////////////////////////////////////////////////////////////////////////////
HRESULT GetSubtype(
    __in LPCWSTR wszName,
    __in const DWORD cFormats,
    __in_ecount( cFormats ) const FormatInfo *prgFormats,
    __out GUID *pguidSubtype )
{
    HRESULT hr = E_INVALIDARG;

    for( DWORD ii = 0; ( ii < cFormats ) && ( S_OK != hr ); ii++ )
    {
        if( 0 == _wcsicmp( wszName, prgFormats[ii].wszName ) )
        {
            *pguidSubtype = prgFormats[ii].guidSubtype;
            hr = S_OK;
            break;
        }
    }

    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT GetAudioSubtype(
    __in LPCWSTR wszName,
    __out GUID *pguidAudioSubtype )
{
    HRESULT hr = S_OK;

    static const FormatInfo aAudioFormats[] =
    {
        { L"AAC",              MFAudioFormat_AAC },
        { L"WMAudioV8",        MFAudioFormat_WMAudioV8 },
        { L"WMAudioV9",        MFAudioFormat_WMAudioV9 },
        { L"WMAudio_Lossless", MFAudioFormat_WMAudio_Lossless },
    };

    CHECK_HR( hr = GetSubtype( wszName,
                               ARRAYSIZE( aAudioFormats ),
                               aAudioFormats,
                               pguidAudioSubtype ) );

done:
    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT GetVideoSubtype(
    __in LPCWSTR wszName,
    __out GUID *pguidVideoSubtype )
{
    HRESULT hr = S_OK;

    static const FormatInfo aVideoFormats[] =
    {
        { L"H264",             MFVideoFormat_H264 },
        { L"WMV1",             MFVideoFormat_WMV1 },
        { L"WMV2",             MFVideoFormat_WMV2 },
        { L"WMV3",             MFVideoFormat_WMV3 },
        { L"WVC1",             MFVideoFormat_WVC1 },
    };

    CHECK_HR( hr = GetSubtype( wszName,
                               ARRAYSIZE( aVideoFormats ),
                               aVideoFormats,
                               pguidVideoSubtype ) );

done:
    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT GetUINT32Pair(
    __in LPCWSTR wszValue,
    __in const WCHAR wcSeparator,
    __out UINT32 *punValue1,
    __out UINT32 *punValue2 )
{
    HRESULT hr = S_OK;

    WCHAR wszValue1[30] = {0};
    WCHAR wszValue2[30] = {0};

    LPCWSTR pwszSeparator = wcschr( wszValue, wcSeparator );
    if( ( NULL == pwszSeparator ) ||
        ( wszValue == pwszSeparator ) )
    {
        hr = E_INVALIDARG;
        goto done;
    }

    CHECK_HR( hr = StringCchCopyN( wszValue1, ARRAYSIZE( wszValue1 ), wszValue, pwszSeparator - wszValue ) );
    CHECK_HR( hr = StringCchCopy( wszValue2, ARRAYSIZE( wszValue2 ), pwszSeparator + 1) );

    *punValue1 = _wtoi( wszValue1 );
    *punValue2 = _wtoi( wszValue2 );

done:

    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT GetInterlaceMode(
    __in LPCWSTR wszInterlaceMode,
    __out UINT32 *punInterlaceMode )
{
    HRESULT hr = E_INVALIDARG;

    struct InterlaceModeInfo
    {
        WCHAR wszInterlaceMode[32];
        MFVideoInterlaceMode eInterlaceMode;
    };

    static const InterlaceModeInfo rgInterlaceModes[] =
    {
        { L"Progressive",                 MFVideoInterlace_Progressive },
        { L"FieldInterleavedUpperFirst",  MFVideoInterlace_FieldInterleavedUpperFirst },
        { L"FieldInterleavedLowerFirst",  MFVideoInterlace_FieldInterleavedLowerFirst },
        { L"FieldSingleUpper",            MFVideoInterlace_FieldSingleUpper },
        { L"FieldSingleLower",            MFVideoInterlace_FieldSingleLower },
        { L"MixedInterlaceOrProgressive", MFVideoInterlace_MixedInterlaceOrProgressive },
    };

    *punInterlaceMode = MFVideoInterlace_Unknown;

    for( DWORD ii = 0; ( ii < ARRAYSIZE( rgInterlaceModes ) ) && ( S_OK != hr ); ii++ )
    {
        if( 0 == _wcsicmp( wszInterlaceMode, rgInterlaceModes[ii].wszInterlaceMode ) )
        {
            *punInterlaceMode = rgInterlaceModes[ii].eInterlaceMode;
            hr = S_OK;
            break;
        }
    }

    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT ParseCommandLine(
    __in int argc,
    __in_ecount( argc ) WCHAR *argv[],
    __in CMFCopy *pMFCopy,
    __out LPCWSTR *pwszSourceFilename,
    __out LPCWSTR *pwszTargetFilename,
    __out BOOL *pfCopyMetadata,
    __out BOOL *pfQuietMode )
{
    HRESULT hr = S_OK;

    DWORD dwIndex = 1;

    BOOL fTranscodeAudio = FALSE;
    BOOL fTranscodeVideo = FALSE;
    BOOL fExcludeAudio = FALSE;
    BOOL fExcludeVideo = FALSE;

    *pwszSourceFilename = NULL;
    *pwszTargetFilename = NULL;
    *pfCopyMetadata = TRUE;
    *pfQuietMode = FALSE;

    //
    // Process command line parameters
    //
    while( dwIndex < (DWORD)argc )
    {
        LPCWSTR pwszParameter = NULL;

        if( ( L'-' != *argv[dwIndex] ) &&
            ( L'/' != *argv[dwIndex] ) )
        {
            break;
        }

        pwszParameter = argv[dwIndex] + 1;

        if( ( 0 == wcscmp( pwszParameter, L"?" ) ) ||
            ( 0 == wcscmp( pwszParameter, L"h" ) ) )
        {
            hr = S_FALSE;
            goto done;
        }
        else if( 0 == wcscmp( pwszParameter, L"q" ) )
        {
            *pfQuietMode = TRUE;
            dwIndex++;
        }
        else if( 0 == wcscmp( pwszParameter, L"a" ) )
        {
            GUID guidTargetAudioSubtype;

            if( (DWORD)argc < ( dwIndex + 2 ) )
            {
                hr = E_INVALIDARG;
                goto done;
            }

            if( FAILED( GetAudioSubtype( argv[ dwIndex+1 ], &guidTargetAudioSubtype ) ) )
            {
                hr = E_INVALIDARG;
                fprintf( stderr, "ERROR: Unrecognized audio format.\n" );
                goto done;
            }

            CHECK_HR( hr = pMFCopy->SetTargetAudioFormat( guidTargetAudioSubtype ) );

            fTranscodeAudio = TRUE;

            dwIndex += 2;
        }
        else if( 0 == wcscmp( pwszParameter, L"nc" ) )
        {
            UINT32 unNumChannels;

            if( (DWORD)argc < ( dwIndex + 2 ) )
            {
                hr = E_INVALIDARG;
                goto done;
            }

            unNumChannels = _wtoi( argv[dwIndex+1] );

            CHECK_HR( hr = pMFCopy->SetAudioNumChannels( unNumChannels ) );

            fTranscodeAudio = TRUE;

            dwIndex += 2;
        }
        else if( 0 == wcscmp( pwszParameter, L"sr" ) )
        {
            UINT32 unSampleRate;

            if( (DWORD)argc < ( dwIndex + 2 ) )
            {
                hr = E_INVALIDARG;
                goto done;
            }

            unSampleRate = _wtoi( argv[dwIndex+1] );

            CHECK_HR( hr = pMFCopy->SetAudioSampleRate( unSampleRate ) );

            fTranscodeAudio = TRUE;

            dwIndex += 2;
        }
        else if( 0 == wcscmp( pwszParameter, L"v" ) )
        {
            GUID guidTargetVideoSubtype;

            if( (DWORD)argc < ( dwIndex + 2 ) )
            {
                hr = E_INVALIDARG;
                goto done;
            }

            if( FAILED( GetVideoSubtype( argv[ dwIndex+1 ], &guidTargetVideoSubtype ) ) )
            {
                hr = E_INVALIDARG;
                fprintf( stderr, "ERROR: Unrecognized video format.\n" );
                goto done;
            }

            CHECK_HR( hr = pMFCopy->SetTargetVideoFormat( guidTargetVideoSubtype ) );

            fTranscodeVideo = TRUE;

            dwIndex += 2;
        }
        else if( 0 == wcscmp( pwszParameter, L"fs" ) )
        {
            UINT32 unWidth;
            UINT32 unHeight;

            if( (DWORD)argc < ( dwIndex + 2 ) )
            {
                hr = E_INVALIDARG;
                goto done;
            }

            if( FAILED( GetUINT32Pair( argv[dwIndex+1], L'x', &unWidth, &unHeight ) ) )
            {
                hr = E_INVALIDARG;
                fprintf( stderr, "ERROR: Invalid frame size.\n" );
                goto done;
            }

            CHECK_HR( hr = pMFCopy->SetVideoFrameSize( unWidth, unHeight ) );

            fTranscodeVideo = TRUE;

            dwIndex += 2;
        }
        else if( 0 == wcscmp( pwszParameter, L"fr" ) )
        {
            UINT32 unFrameRateNumerator;
            UINT32 unFrameRateDenominator;

            if( (DWORD)argc < ( dwIndex + 2 ) )
            {
                hr = E_INVALIDARG;
                goto done;
            }

            if( FAILED( GetUINT32Pair( argv[dwIndex+1], L':', &unFrameRateNumerator, &unFrameRateDenominator ) ) )
            {
                hr = E_INVALIDARG;
                fprintf( stderr, "ERROR: Invalid frame rate.\n" );
                goto done;
            }

            CHECK_HR( hr = pMFCopy->SetVideoFrameRate( unFrameRateNumerator, unFrameRateDenominator ) );

            fTranscodeVideo = TRUE;

            dwIndex += 2;
        }
        else if( 0 == wcscmp( pwszParameter, L"im" ) )
        {
            UINT32 unInterlaceMode;

            if( (DWORD)argc < ( dwIndex + 2 ) )
            {
                hr = E_INVALIDARG;
                goto done;
            }

            hr = GetInterlaceMode( argv[dwIndex+1], &unInterlaceMode );
            if( S_OK != hr )
            {
                fprintf( stderr, "ERROR: Unrecognized interlace mode.\n" );
                goto done;
            }

            CHECK_HR( hr = pMFCopy->SetVideoInterlaceMode( unInterlaceMode ) );

            fTranscodeVideo = TRUE;

            dwIndex += 2;
        }
        else if( 0 == wcscmp( pwszParameter, L"br" ) )
        {
            UINT32 unBitrate;

            if( (DWORD)argc < ( dwIndex + 2 ) )
            {
                hr = E_INVALIDARG;
                goto done;
            }

            unBitrate = _wtoi( argv[dwIndex+1] );

            CHECK_HR( hr = pMFCopy->SetVideoAverageBitrate( unBitrate ) );

            fTranscodeVideo = TRUE;

            dwIndex += 2;
        }
        else if( 0 == wcscmp( pwszParameter, L"s" ) )
        {
            LONGLONG llStartPosition;

            if( (DWORD)argc < ( dwIndex + 2 ) )
            {
                hr = E_INVALIDARG;
                goto done;
            }

            // convert from ms to hns units
            llStartPosition = _wtoi64( argv[dwIndex+1] ) * 10000;

            CHECK_HR( hr = pMFCopy->SetStartPosition( llStartPosition ) );

            dwIndex += 2;
        }
        else if( 0 == wcscmp( pwszParameter, L"d" ) )
        {
            ULONGLONG ullDurationToCopy;

            if( (DWORD)argc < ( dwIndex + 2 ) )
            {
                hr = E_INVALIDARG;
                goto done;
            }

            // convert from ms to hns units
            ullDurationToCopy = _wtoi64( argv[dwIndex+1] ) * 10000;

            CHECK_HR( hr = pMFCopy->SetDuration( ullDurationToCopy ) );

            dwIndex += 2;
        }
        else if( 0 == wcscmp( pwszParameter, L"hw" ) )
        {
            CHECK_HR( hr = pMFCopy->EnableHardwareTransforms( TRUE ) );
            dwIndex++;
        }
        else if( 0 == wcscmp( pwszParameter, L"xa" ) )
        {
            CHECK_HR( hr = pMFCopy->ExcludeAudioStreams( TRUE ) );
            fExcludeAudio = TRUE;
            dwIndex++;
        }
        else if( 0 == wcscmp( pwszParameter, L"xv" ) )
        {
            CHECK_HR( hr = pMFCopy->ExcludeVideoStreams( TRUE ) );
            fExcludeVideo = TRUE;
            dwIndex++;
        }
        else if( 0 == wcscmp( pwszParameter, L"xo" ) )
        {
            CHECK_HR( hr = pMFCopy->ExcludeNonAVStreams( TRUE ) );
            dwIndex++;
        }
        else if( 0 == wcscmp( pwszParameter, L"xm" ) )
        {
            *pfCopyMetadata = FALSE;
            dwIndex++;
        }
        else
        {
            break;
        }
    }

    // check for incompatible command line arguments
    if( ( fTranscodeAudio && fExcludeAudio ) ||
        ( fTranscodeVideo && fExcludeVideo ) )
    {
        hr = E_INVALIDARG;
        fprintf( stderr, "ERROR: Mutually exclusive command line arguments specified.\n" );
        goto done;
    }

    // expect the last two parameters to be the source and target filenames
    if( (DWORD)argc != ( dwIndex + 2 ) )
    {
        hr = E_INVALIDARG;
        goto done;
    }

    *pwszSourceFilename = argv[dwIndex];
    *pwszTargetFilename = argv[dwIndex + 1];

done:
    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
void DisplayStatistics(
    __in const MF_SINK_WRITER_STATISTICS *pStats )
{
    printf( "   Num Samples:                %I64u/%I64u/%I64u    (received/encoded/processed)\n"
            "   Bytes Processed:            %I64u\n"
            "   Bytes Still in Queue:       %u\n"
            "   Avg Sample Rate Received:   %u\n"
            "   Avg Sample Rate Encoded:    %u\n"
            "   Avg Sample Rate Processed:  %u\n",
            pStats->qwNumSamplesReceived, pStats->qwNumSamplesEncoded, pStats->qwNumSamplesProcessed,
            pStats->qwByteCountProcessed,
            pStats->dwByteCountQueued,
            pStats->dwAverageSampleRateReceived,
            pStats->dwAverageSampleRateEncoded,
            pStats->dwAverageSampleRateProcessed );
}

///////////////////////////////////////////////////////////////////////////////
void DisplayStatistics(
    __in CMFCopy *pMFCopy )
{
    MF_SINK_WRITER_STATISTICS stats;

    const DWORD cStreams = pMFCopy->GetStreamCount();
    DWORD cSelectedStreams = 0;

    //
    // display the individual stream statistics
    //

    for( DWORD ii = 0; ii < cStreams; ii++ )
    {
        BOOL fSelected;

        if( FAILED( pMFCopy->GetStreamSelection( ii, &fSelected ) ) ||
            ! fSelected )
        {
            continue;
        }

        cSelectedStreams++;

        ZeroMemory( &stats, sizeof( stats ) );
        stats.cb = sizeof( MF_SINK_WRITER_STATISTICS );

        if( SUCCEEDED( pMFCopy->GetStatistics( ii, &stats ) ) )
        {
            printf( "\nStatistics for stream %2u:\n", ii );
            DisplayStatistics( &stats );
        }
    }

    //
    // when multiple streams are present, also display the overall stats
    //

    if( cSelectedStreams > 1 )
    {
        ZeroMemory( &stats, sizeof( stats ) );
        stats.cb = sizeof( MF_SINK_WRITER_STATISTICS );

        if( SUCCEEDED( pMFCopy->GetStatistics( (DWORD)MF_SINK_WRITER_ALL_STREAMS, &stats ) ) )
        {
            printf( "\nOverall statistics:\n" );
            DisplayStatistics( &stats );
        }
    }

    printf( "\n" );

    //
    // display rough timing information
    //
    printf( "\nFinalization Time: %.4fs\n", pMFCopy->GetFinalizationTime() );
    printf( "Overall Processing Time: %.4fs\n", pMFCopy->GetOverallProcessingTime() );
}

///////////////////////////////////////////////////////////////////////////////
void ProgressCallback(
    __in DWORD dwPercentComplete,
    __in_opt LPVOID /* pvContext */ )
{
    static DWORD dwProgress = 0;
    static const DWORD cProgressBarTicks = 52;

    const DWORD dwCurrentProgress = ( min( dwPercentComplete, 100 ) * cProgressBarTicks / 100 );

    if( !g_fProgressBannerDisplayed )
    {
        g_fProgressBannerDisplayed = TRUE;

        printf( "            0%%-------20%%-------40%%-------60%%-------80%%-------100%%\n" );
        printf( "Writing:    " );
    }

    while( dwProgress <= dwCurrentProgress )
    {
        dwProgress++;
        printf( "*" );
    }
}

///////////////////////////////////////////////////////////////////////////////
HRESULT GetPropertyStore(
    __in LPCWSTR wszFilename,
    __in GETPROPERTYSTOREFLAGS gpsFlags,
    __out IPropertyStore **ppPropertyStore )
{
    HRESULT hr = S_OK;

    WCHAR wszFullPath[MAX_PATH];

    *ppPropertyStore = NULL;

    if( NULL == _wfullpath( wszFullPath, wszFilename, ARRAYSIZE( wszFullPath ) ) )
    {
        hr = E_FAIL;
        goto done;
    }

    CHECK_HR( hr = SHGetPropertyStoreFromParsingName( wszFullPath, NULL, gpsFlags, IID_PPV_ARGS( ppPropertyStore ) ) );

done:
    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CopyMetadata(
    __in LPCWSTR wszSourceFilename,
    __in LPCWSTR wszTargetFilename )
{
    HRESULT hr = S_OK;

    IPropertyStore *pSource = NULL;
    IPropertyStore *pDest = NULL;

    DWORD cProps;
    PROPVARIANT var;
    PropVariantInit( &var );

    //
    // Use the shell property system to transfer metadata
    //

    CHECK_HR( hr = GetPropertyStore( wszSourceFilename, GPS_DEFAULT, &pSource ) );
    CHECK_HR( hr = GetPropertyStore( wszTargetFilename, GPS_READWRITE, &pDest ) );

    CHECK_HR( hr = pSource->GetCount( &cProps ) );

    for( DWORD ii = 0; ii < cProps; ii++ )
    {
        PROPERTYKEY pkey;

        if( S_OK != pSource->GetAt( ii, &pkey ) )
        {
            continue;
        }

        if( S_OK != pSource->GetValue( pkey, &var ) )
        {
            continue;
        }

        (void) pDest->SetValue( pkey, var );

        PropVariantClear( &var );
    }

    CHECK_HR( hr = pDest->Commit() );

done:

    PropVariantClear( &var );

    SAFE_RELEASE( pSource );
    SAFE_RELEASE( pDest );

    return( hr );
}

