// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "stdafx.h"
#include "MFCopy.h"

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::CreateInstance(
    __out CMFCopy **ppMFCopy )
{
    HRESULT hr = S_OK;

    CMFCopy *pMFCopy = NULL;

    *ppMFCopy = NULL;

    pMFCopy = new CMFCopy();
    if( NULL == pMFCopy )
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    CHECK_HR( hr = pMFCopy->_Initialize() );

    *ppMFCopy = pMFCopy;
    pMFCopy = NULL;

done:

    SAFE_RELEASE( pMFCopy );

    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
CMFCopy::CMFCopy()
    : m_cRef( 1 )
    , m_wszSourceFilename( NULL )
    , m_wszTargetFilename( NULL )
    , m_pSourceReader( NULL )
    , m_pSinkWriter( NULL )
    , m_pAttributes( NULL )
    , m_pAudioOverrides( NULL )
    , m_pVideoOverrides( NULL )
    , m_paStreamInfo( NULL )
    , m_cStreams( 0 )
    , m_cSelectedStreams( 0 )
    , m_llTimestampAdjustment( 0 )
    , m_fRemuxing( FALSE )
    , m_pfnProgressCallback( NULL )
    , m_pvContext( NULL )
    , m_hrError( S_OK )
    , m_wszError( NULL )
{
    ZeroMemory( &m_Timing, sizeof( m_Timing ) );
    ZeroMemory( &m_Options, sizeof( m_Options ) );
}

///////////////////////////////////////////////////////////////////////////////
CMFCopy::~CMFCopy()
{
    SAFE_RELEASE( m_pSourceReader );
    SAFE_RELEASE( m_pSinkWriter );
    SAFE_RELEASE( m_pAttributes );
    SAFE_RELEASE( m_pAudioOverrides );
    SAFE_RELEASE( m_pVideoOverrides );
    SAFE_ARRAY_DELETE( m_paStreamInfo );
}

///////////////////////////////////////////////////////////////////////////////
ULONG CMFCopy::AddRef()
{
    LONG cRef = InterlockedIncrement( &m_cRef );

    return( cRef );
}

///////////////////////////////////////////////////////////////////////////////
ULONG CMFCopy::Release()
{
    LONG cRef = InterlockedDecrement( &m_cRef );

    if( 0 == cRef )
    {
        m_cRef++;
        delete this;
    }

    return( cRef );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::Copy(
    __in LPCWSTR wszSourceFilename,
    __in LPCWSTR wszTargetFilename )
{
    HRESULT hr = S_OK;

    if( ( NULL == wszSourceFilename ) ||
        ( NULL == wszTargetFilename ) )
    {
        hr = E_INVALIDARG;
        _SetErrorDetails( hr, L"Source and target filenames are required" );
        goto done;
    }

    m_wszSourceFilename = wszSourceFilename;
    m_wszTargetFilename = wszTargetFilename;

    //
    // reset internal state
    //
    ZeroMemory( &m_Timing, sizeof( m_Timing ) );

    m_fRemuxing = FALSE;
    m_llTimestampAdjustment = 0;

    m_hrError = S_OK;
    m_wszError = NULL;

    CHECK_HR( hr = _CreateReaderAndWriter() );

    CHECK_HR( hr = _DetermineDuration() );

    CHECK_HR( hr = _ConfigureStreams() );

    if( 0 != m_Options.llStartPosition )
    {
        CHECK_HR( hr = _ConfigureStartPosition() );
    }

    CHECK_HR( hr = _ProcessSamples() );

done:

    if( S_OK != hr )
    {
        _SetErrorDetails( hr, L"Failed to copy" );
    }

    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
void CMFCopy::SetProgressCallback(
    __in_opt PFN_MFCOPY_PROGRESS_CALLBACK pfnCallback,
    __in_opt LPVOID pvContext )
{
    m_pfnProgressCallback = pfnCallback;
    m_pvContext = pvContext;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::SetStartPosition(
    __in LONGLONG llTimestamp )
{
    m_Options.llStartPosition = llTimestamp;

    return( S_OK );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::SetDuration(
    __in ULONGLONG ullDuration )
{
    HRESULT hr = S_OK;

    if( 0 == ullDuration )
    {
        hr = E_INVALIDARG;
        goto done;
    }

    m_Options.fDurationSpecified = TRUE;
    m_Options.ullDuration = ullDuration;

done:
    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::SetTargetAudioFormat(
    __in REFGUID guidSubtype )
{
    HRESULT hr = S_OK;

    if( GUID_NULL == guidSubtype )
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if( m_Options.fExcludeAudio )
    {
        CHECK_HR( hr = MF_E_INVALIDREQUEST );
    }

    CHECK_HR( hr = m_pAudioOverrides->SetGUID( MF_MT_SUBTYPE, guidSubtype ) );

    m_Options.fTranscodeAudio = TRUE;

done:
    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::SetAudioNumChannels(
    __in UINT32 unNumChannels )
{
    HRESULT hr = S_OK;

    if( 0 == unNumChannels )
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if( m_Options.fExcludeAudio )
    {
        hr = MF_E_INVALIDREQUEST;
        goto done;
    }

    CHECK_HR( hr = m_pAudioOverrides->SetUINT32( MF_MT_AUDIO_NUM_CHANNELS, unNumChannels ) );

    m_Options.fTranscodeAudio = TRUE;

done:
    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::SetAudioSampleRate(
    __in UINT32 unSampleRate )
{
    HRESULT hr = S_OK;

    if( 0 == unSampleRate )
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if( m_Options.fExcludeAudio )
    {
        hr = MF_E_INVALIDREQUEST;
        goto done;
    }

    CHECK_HR( hr = m_pAudioOverrides->SetUINT32( MF_MT_AUDIO_SAMPLES_PER_SECOND, unSampleRate ) );

    m_Options.fTranscodeAudio = TRUE;

done:
    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::SetTargetVideoFormat(
    __in REFGUID guidSubtype )
{
    HRESULT hr = S_OK;

    if( GUID_NULL == guidSubtype )
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if( m_Options.fExcludeVideo )
    {
        hr = MF_E_INVALIDREQUEST;
        goto done;
    }

    CHECK_HR( hr = m_pVideoOverrides->SetGUID( MF_MT_SUBTYPE, guidSubtype ) );

    m_Options.fTranscodeVideo = TRUE;

done:
    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::SetVideoFrameSize(
    __in UINT32 unWidth,
    __in UINT32 unHeight )
{
    HRESULT hr = S_OK;

    if( ( 0 == unWidth ) || ( 0 == unHeight ) )
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if( m_Options.fExcludeVideo )
    {
        CHECK_HR( hr = MF_E_INVALIDREQUEST );
    }

    CHECK_HR( hr = MFSetAttributeSize( m_pVideoOverrides, MF_MT_FRAME_SIZE, unWidth, unHeight ) );

    m_Options.fTranscodeVideo = TRUE;

done:
    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::SetVideoFrameRate(
    __in UINT32 unNumerator,
    __in UINT32 unDenominator )
{
    HRESULT hr = S_OK;

    if( ( 0 == unNumerator ) || ( 0 == unDenominator ) )
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if( m_Options.fExcludeVideo )
    {
        hr = MF_E_INVALIDREQUEST;
        goto done;
    }

    CHECK_HR( hr = MFSetAttributeRatio( m_pVideoOverrides, MF_MT_FRAME_RATE, unNumerator, unDenominator ) );

    m_Options.fTranscodeVideo = TRUE;

done:
    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::SetVideoInterlaceMode(
    __in UINT32 unInterlaceMode )
{
    HRESULT hr = S_OK;

    if( MFVideoInterlace_Unknown == unInterlaceMode )
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if( m_Options.fExcludeVideo )
    {
        hr = MF_E_INVALIDREQUEST;
        goto done;
    }

    CHECK_HR( hr = m_pVideoOverrides->SetUINT32( MF_MT_INTERLACE_MODE, unInterlaceMode ) );

    m_Options.fTranscodeVideo = TRUE;

done:
    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::SetVideoAverageBitrate(
    __in UINT32 unBitrate )
{
    HRESULT hr = S_OK;

    if( 0 == unBitrate )
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if( m_Options.fExcludeVideo )
    {
        hr = MF_E_INVALIDREQUEST;
        goto done;
    }

    CHECK_HR( hr = m_pVideoOverrides->SetUINT32( MF_MT_AVG_BITRATE, unBitrate ) );

    m_Options.fTranscodeVideo = TRUE;

done:
    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::ExcludeAudioStreams(
    __in BOOL fExclude )
{
    HRESULT hr = S_OK;

    if( fExclude &&
        ( m_Options.fTranscodeAudio ||
          ( m_Options.fExcludeVideo && m_Options.fExcludeNonAVStreams ) ) )
    {
        CHECK_HR( hr = MF_E_INVALIDREQUEST );
    }

    m_Options.fExcludeAudio = fExclude;

done:
    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::ExcludeVideoStreams(
    __in BOOL fExclude )
{
    HRESULT hr = S_OK;

    if( fExclude &&
        ( m_Options.fTranscodeVideo ||
          ( m_Options.fExcludeAudio && m_Options.fExcludeNonAVStreams ) ) )
    {
        CHECK_HR( hr = MF_E_INVALIDREQUEST );
    }

    m_Options.fExcludeVideo = fExclude;

done:
    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::ExcludeNonAVStreams(
    __in BOOL fExclude )
{
    HRESULT hr = S_OK;

    if( fExclude && m_Options.fExcludeAudio && m_Options.fExcludeVideo )
    {
        CHECK_HR( hr = MF_E_INVALIDREQUEST );
    }

    m_Options.fExcludeNonAVStreams = fExclude;

done:
    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::EnableHardwareTransforms(
    __in BOOL fEnable )
{
    m_Options.fEnableHardwareTransforms = fEnable;

    return( S_OK );
}

///////////////////////////////////////////////////////////////////////////////
void CMFCopy::Reset()
{
    //
    // Reset all configuration options to default settings
    //
    ZeroMemory( &m_Options, sizeof( m_Options ) );

    (void) m_pAudioOverrides->DeleteAllItems();
    (void) m_pVideoOverrides->DeleteAllItems();
}

///////////////////////////////////////////////////////////////////////////////
DWORD CMFCopy::GetStreamCount()
{
    return( m_cStreams );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::GetStreamSelection(
    __in DWORD dwStreamIndex,
    __out BOOL *pfSelected )
{
    HRESULT hr = S_OK;

    if( dwStreamIndex >= m_cStreams )
    {
        *pfSelected = FALSE;
        CHECK_HR( hr = MF_E_INVALIDSTREAMNUMBER );
    }

    *pfSelected = m_paStreamInfo[dwStreamIndex].fSelected;

done:
    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::GetStatistics(
    __in DWORD dwStreamIndex,
    __out MF_SINK_WRITER_STATISTICS *pStats )
{
    HRESULT hr = S_OK;

    DWORD dwOutputStreamIndex = 0;

    if( dwStreamIndex < m_cStreams )
    {
        const StreamInfo& streamInfo = m_paStreamInfo[dwStreamIndex];

        if( !streamInfo.fSelected )
        {
            hr = MF_E_INVALIDREQUEST;
            goto done;
        }

        dwOutputStreamIndex = streamInfo.dwOutputStreamIndex;
    }
    else if( MF_SINK_WRITER_ALL_STREAMS == dwStreamIndex )
    {
        dwOutputStreamIndex = (DWORD)MF_SINK_WRITER_ALL_STREAMS;
    }
    else
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if( NULL == m_pSinkWriter )
    {
        hr = MF_E_INVALIDREQUEST;
        goto done;
    }

    CHECK_HR( hr = m_pSinkWriter->GetStatistics( dwOutputStreamIndex, pStats ) );

done:
    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
double CMFCopy::GetFinalizationTime()
{
    LARGE_INTEGER li;
    QueryPerformanceFrequency( &li );

    return( (double)( m_Timing.llStopTime - m_Timing.llFinalizeStartTime ) / (double)li.QuadPart );
}

///////////////////////////////////////////////////////////////////////////////
double CMFCopy::GetOverallProcessingTime()
{
    LARGE_INTEGER li;
    QueryPerformanceFrequency( &li );

    return( (double)( m_Timing.llStopTime - m_Timing.llStartTime ) / (double)li.QuadPart );
}

///////////////////////////////////////////////////////////////////////////////
void CMFCopy::GetErrorDetails(
    __out HRESULT *phr,
    __out LPCWSTR *pwszError )
{
    *phr = m_hrError;
    *pwszError = m_wszError;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::_Initialize()
{
    HRESULT hr = S_OK;

    IMFAttributes *pAttributes = NULL;
    IMFAttributes *pAudioOverrides = NULL;
    IMFAttributes *pVideoOverrides = NULL;

    //
    // Allocate resources
    //

    CHECK_HR( hr = MFCreateAttributes( &pAttributes, 1 ) );
    CHECK_HR( hr = MFCreateAttributes( &pAudioOverrides, 1 ) );
    CHECK_HR( hr = MFCreateAttributes( &pVideoOverrides, 1 ) );

    //
    // Transfer references
    //

    m_pAttributes = pAttributes;
    pAttributes = NULL;

    m_pAudioOverrides = pAudioOverrides;
    pAudioOverrides = NULL;

    m_pVideoOverrides = pVideoOverrides;
    pVideoOverrides = NULL;

done:

    SAFE_RELEASE( pAttributes );
    SAFE_RELEASE( pAudioOverrides );
    SAFE_RELEASE( pVideoOverrides );

    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::_CreateReaderAndWriter()
{
    HRESULT hr = S_OK;

    IMFReadWriteClassFactory *pClassFactory = NULL;
    IMFSourceReader *pSourceReader = NULL;
    IMFSinkWriter *pSinkWriter = NULL;

    CHECK_HR( hr = m_pAttributes->SetUINT32( MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS,
                                             m_Options.fEnableHardwareTransforms ) );

    CHECK_HR( hr = CoCreateInstance( CLSID_MFReadWriteClassFactory,
                                     NULL,
                                     CLSCTX_INPROC_SERVER,
                                     IID_PPV_ARGS( &pClassFactory ) ) );

    CHECK_HR( hr = pClassFactory->CreateInstanceFromURL( CLSID_MFSourceReader,
                                                         m_wszSourceFilename,
                                                         m_pAttributes,
                                                         IID_PPV_ARGS( &pSourceReader ) ) );

    CHECK_HR( hr = pClassFactory->CreateInstanceFromURL( CLSID_MFSinkWriter,
                                                         m_wszTargetFilename,
                                                         m_pAttributes,
                                                         IID_PPV_ARGS( &pSinkWriter ) ) );

    // transfer the references

    SAFE_RELEASE( m_pSourceReader );
    m_pSourceReader = pSourceReader;
    pSourceReader = NULL;

    SAFE_RELEASE( m_pSinkWriter );
    m_pSinkWriter = pSinkWriter;
    pSinkWriter = NULL;

done:

    SAFE_RELEASE( pSinkWriter );
    SAFE_RELEASE( pSourceReader );
    SAFE_RELEASE( pClassFactory );

    if( S_OK != hr )
    {
        _SetErrorDetails( hr, L"Failed to create source reader and sink writer objects" );
    }

    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::_DetermineDuration()
{
    HRESULT hr = S_OK;

    PROPVARIANT var;
    PropVariantInit( &var );

    ULONGLONG ullDuration;

    // get the duration as an attribute from the media source
    CHECK_HR( hr = m_pSourceReader->GetPresentationAttribute( (DWORD)MF_SOURCE_READER_MEDIASOURCE,
                                                              MF_PD_DURATION,
                                                              &var ) );

    // check if duration is unknown
    if( ( var.vt != VT_UI8 ) || ( 0 == var.uhVal.QuadPart ) )
    {
        ullDuration = 1;
    }
    else
    {
        ullDuration = var.uhVal.QuadPart;
    }

    // check if duration to copy is overriden by the command line
    if( 0 == m_Options.ullDuration )
    {
        m_Options.ullDuration = ullDuration;
    }
    else
    {
        m_Options.ullDuration = min( m_Options.ullDuration, ullDuration );
    }

done:

    PropVariantClear( &var );

    if( S_OK != hr )
    {
        _SetErrorDetails( hr, L"Failed to failed to determine content duration" );
    }

    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::_ConfigureStreams()
{
    HRESULT hr = S_OK;

    // Allocate an array to store information about the streams
    CHECK_HR( hr = _AllocateStreamInfoArray() );

    // setup stream selection based on the command line options
    CHECK_HR( hr = _ConfigureStreamSelection() );

    //
    // Loop through the selected streams to configure the sink writer
    //

    for( DWORD ii = 0; ii < m_cStreams; ii++ )
    {
        if( !m_paStreamInfo[ii].fSelected )
        {
            continue;
        }

        CHECK_HR( hr = _ConfigureStream( ii ) );
    }

done:

    if( S_OK != hr )
    {
        _SetErrorDetails( hr, L"Failed to configure streams" );
    }

    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::_AllocateStreamInfoArray()
{
    HRESULT hr = S_OK;

    StreamInfo *paStreamInfo = NULL;
    DWORD cStreams = 0;

    CHECK_HR( hr = _GetSourceReaderStreamCount( &cStreams ) );

    if( 0 == cStreams )
    {
        hr = E_FAIL;
        _SetErrorDetails( hr, L"No available streams" );
        goto done;
    }

    // ensure that sizeof( StreamInfo ) * cStreams will not overflow
    size_t cbStreamInfo;
    CHECK_HR( hr = SizeTMult( sizeof( StreamInfo ), cStreams, &cbStreamInfo ) );

    paStreamInfo = new StreamInfo[ cStreams ];
    if( NULL == paStreamInfo )
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    ZeroMemory( paStreamInfo, cbStreamInfo );

    SAFE_ARRAY_DELETE( m_paStreamInfo );
    m_paStreamInfo = paStreamInfo;
    paStreamInfo = NULL;

    m_cStreams = cStreams;

done:

    SAFE_ARRAY_DELETE( paStreamInfo );

    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::_GetSourceReaderStreamCount(
    __out DWORD *pcStreams )
{
    HRESULT hr = S_OK;

    *pcStreams = 0;

    for( DWORD ii = 0 ;; ii++ )
    {
        BOOL fSelected;

        hr = m_pSourceReader->GetStreamSelection( ii, &fSelected );
        if( MF_E_INVALIDSTREAMNUMBER == hr )
        {
            hr = S_OK;
            break;
        }
        CHECK_HR( hr );

        (*pcStreams)++;
    }

done:
    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::_ConfigureStreamSelection()
{
    HRESULT hr = S_OK;

    IMFMediaType *pNativeMediaType = NULL;

    BOOL fHasAudio = FALSE;
    BOOL fHasVideo = FALSE;

    m_cSelectedStreams = 0;

    //
    // loop through all available streams and set stream selection
    // based on the command line options
    //
    for( DWORD ii = 0; ii < m_cStreams; ii++ )
    {
        StreamInfo& streamInfo = m_paStreamInfo[ii];
        GUID guidMajorType;

        SAFE_RELEASE( pNativeMediaType );

        CHECK_HR( hr = m_pSourceReader->GetNativeMediaType( ii, 0, &pNativeMediaType ) );
        CHECK_HR( hr = pNativeMediaType->GetGUID( MF_MT_MAJOR_TYPE, &guidMajorType ) );

        const BOOL fIsAudio = ( MFMediaType_Audio == guidMajorType );
        const BOOL fIsVideo = ( MFMediaType_Video == guidMajorType );

        if( fIsAudio )
        {
            streamInfo.eStreamType = AUDIO_STREAM;
            fHasAudio = TRUE;
        }
        else if( fIsVideo )
        {
            streamInfo.eStreamType = VIDEO_STREAM;
            fHasVideo = TRUE;
        }
        else
        {
            streamInfo.eStreamType = NON_AV_STREAM;
        }

        if( ( m_Options.fExcludeAudio && fIsAudio ) ||
            ( m_Options.fExcludeVideo && fIsVideo ) ||
            ( m_Options.fExcludeNonAVStreams && !fIsAudio && !fIsVideo ) )
        {
            streamInfo.fSelected = FALSE;
        }
        else
        {
            streamInfo.fSelected = TRUE;
            m_cSelectedStreams++;
        }

        CHECK_HR( hr = m_pSourceReader->SetStreamSelection( ii, streamInfo.fSelected ) );
    }

    if( 0 == m_cSelectedStreams )
    {
        hr = E_FAIL;
        _SetErrorDetails( hr, L"No streams were selected" );
        goto done;
    }

    if( m_Options.fTranscodeAudio && !fHasAudio )
    {
        hr = E_FAIL;
        _SetErrorDetails( hr, L"Content does not have an audio stream" );
        goto done;
    }

    if( m_Options.fTranscodeVideo && !fHasVideo )
    {
        hr = E_FAIL;
        _SetErrorDetails( hr, L"Content does not have a video stream" );
        goto done;
    }

done:

    SAFE_RELEASE( pNativeMediaType );

    if( S_OK != hr )
    {
        _SetErrorDetails( hr, L"Failed to configure stream selection" );
    }

    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::_ConfigureStream(
    __in DWORD dwStreamIndex )
{
    HRESULT hr = S_OK;

    IMFMediaType *pNativeMediaType = NULL;
    IMFMediaType *pTargetMediaType = NULL;

    if( dwStreamIndex >= m_cStreams )
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    StreamInfo& streamInfo = m_paStreamInfo[dwStreamIndex];

    CHECK_HR( hr = m_pSourceReader->GetNativeMediaType( dwStreamIndex,
                                                        0,
                                                        &pNativeMediaType ) );

    //
    // configure the target format for the stream
    //

    if( m_Options.fTranscodeAudio &&
        ( AUDIO_STREAM == streamInfo.eStreamType ) )
    {
        CHECK_HR( hr = _CreateTargetAudioMediaType( pNativeMediaType,
                                                    &pTargetMediaType ) );
    }
    else if( m_Options.fTranscodeVideo &&
             ( VIDEO_STREAM == streamInfo.eStreamType ) )
    {
        CHECK_HR( hr = _CreateTargetVideoMediaType( pNativeMediaType,
                                                    &pTargetMediaType ) );
    }

    // if we're not transcoding the stream, just use the native media type
    if( NULL == pTargetMediaType )
    {
        m_fRemuxing = TRUE;
        pTargetMediaType = pNativeMediaType;
        pTargetMediaType->AddRef();
    }

    hr = m_pSinkWriter->AddStream( pTargetMediaType,
                                   &streamInfo.dwOutputStreamIndex );
    if( S_OK != hr )
    {
        _SetErrorDetails( hr, L"Failed in call to IMFSinkWriter::AddStream" );
        goto done;
    }

    //
    // when transcoding, negotiate the connection between the source reader and sink writer
    //

    if( m_Options.fTranscodeAudio &&
        ( AUDIO_STREAM == streamInfo.eStreamType ) )
    {
        CHECK_HR( hr = _NegotiateAudioStreamFormat( dwStreamIndex ) );
    }
    else if( m_Options.fTranscodeVideo &&
             ( VIDEO_STREAM == streamInfo.eStreamType ) )
    {
        CHECK_HR( hr = _NegotiateVideoStreamFormat( dwStreamIndex ) );
    }

done:

    SAFE_RELEASE( pTargetMediaType );
    SAFE_RELEASE( pNativeMediaType );

    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::_NegotiateAudioStreamFormat(
    __in DWORD dwStreamIndex )
{
    HRESULT hr = S_OK;

    static const GUID *aAudioFormats[] =
    {
        &MFAudioFormat_Float,
        &MFAudioFormat_PCM,
    };

    CHECK_HR( hr = _NegotiateStreamFormat( dwStreamIndex,
                                           MFMediaType_Audio,
                                           ARRAYSIZE( aAudioFormats ),
                                           aAudioFormats ) );

done:
    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::_NegotiateVideoStreamFormat(
    __in DWORD dwStreamIndex )
{
    HRESULT hr = S_OK;

    static const GUID *aVideoFormats[] =
    {
        &MFVideoFormat_NV12,
        &MFVideoFormat_YV12,
        &MFVideoFormat_YUY2,
        &MFVideoFormat_RGB32,
    };

    CHECK_HR( hr = _NegotiateStreamFormat( dwStreamIndex,
                                           MFMediaType_Video,
                                           ARRAYSIZE( aVideoFormats ),
                                           aVideoFormats ) );

done:
    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::_NegotiateStreamFormat(
    __in DWORD dwStreamIndex,
    __in REFGUID guidMajorType,
    __in DWORD cFormats,
    __in_ecount( cFormats ) const GUID **paFormats )
{
    HRESULT hr = S_OK;

    const StreamInfo& streamInfo = m_paStreamInfo[dwStreamIndex];

    IMFMediaType *pPartialMediaType = NULL;
    IMFMediaType *pFullMediaType = NULL;

    BOOL fConfigured = FALSE;

    CHECK_HR( hr = MFCreateMediaType( &pPartialMediaType ) );

    CHECK_HR( hr = pPartialMediaType->SetGUID( MF_MT_MAJOR_TYPE, guidMajorType ) );

    for( DWORD ii = 0; ii < cFormats; ii++ )
    {
        SAFE_RELEASE( pFullMediaType );

        CHECK_HR( hr = pPartialMediaType->SetGUID( MF_MT_SUBTYPE, *paFormats[ii] ) );

        // try to set the partial media type on the source reader
        hr = m_pSourceReader->SetCurrentMediaType( dwStreamIndex,
                                                   NULL,
                                                   pPartialMediaType );
        if( S_OK != hr )
        {
            // format is not supported by the source reader, try the next on the list
            hr = S_OK;
            continue;
        }

        // get the full media type from the source reader
        CHECK_HR( hr = m_pSourceReader->GetCurrentMediaType( dwStreamIndex,
                                                             &pFullMediaType ) );

        // try to set the input media type on the sink writer
        hr = m_pSinkWriter->SetInputMediaType( streamInfo.dwOutputStreamIndex,
                                               pFullMediaType,
                                               NULL );
        if( S_OK != hr )
        {
            // format is not supported by the sink writer, try the next on the list
            hr = S_OK;
            continue;
        }

        fConfigured = TRUE;
        break;
    }

    if( !fConfigured )
    {
        hr = MF_E_INVALIDMEDIATYPE;
        _SetErrorDetails( hr, L"Failed to negotiate a format between the source reader and sink writer" );
        goto done;
    }

done:

    SAFE_RELEASE( pPartialMediaType );
    SAFE_RELEASE( pFullMediaType );

    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::_ConfigureStartPosition()
{
    HRESULT hr = S_OK;

    PROPVARIANT varMediaSourceCaps;
    PropVariantInit( &varMediaSourceCaps );

    PROPVARIANT varPosition;
    InitPropVariantFromInt64( m_Options.llStartPosition, &varPosition );

    // check if the media source is seekable
    CHECK_HR( hr = m_pSourceReader->GetPresentationAttribute( (DWORD)MF_SOURCE_READER_MEDIASOURCE,
                                                              MF_SOURCE_READER_MEDIASOURCE_CHARACTERISTICS,
                                                              &varMediaSourceCaps ) );

    if( 0 == ( MFMEDIASOURCE_CAN_SEEK & varMediaSourceCaps.ulVal ) )
    {
        hr = MF_E_INVALIDREQUEST;
        _SetErrorDetails( hr, L"Cannot set a start position since the source is not seekable" );
        goto done;
    }

    // Set the start position on the source reader as specified on the command line
    hr = m_pSourceReader->SetCurrentPosition( GUID_NULL, varPosition );
    if( S_OK != hr )
    {
        _SetErrorDetails( hr, L"Failed in call to IMFSourceReader::SetCurrentPosition" );
        goto done;
    }

    //
    // determine the timestamp adjustment to use in order to normalize the sample timestamps
    // coming out of the source reader
    //
    CHECK_HR( hr = _DetermineTimestampAdjustment() );

    //
    // Reset the start position on the source reader as the current position may
    // have changed while determining the timestamp adjustment
    //
    hr = m_pSourceReader->SetCurrentPosition( GUID_NULL, varPosition );
    if( S_OK != hr )
    {
        _SetErrorDetails( hr, L"Failed in call to IMFSourceReader::SetCurrentPosition" );
        goto done;
    }

done:

    PropVariantClear( &varPosition );
    PropVariantClear( &varMediaSourceCaps );

    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::_DetermineTimestampAdjustment()
{
    HRESULT hr = S_OK;

    IMFSample *pSample = NULL;
    DWORD cStreamsToConfigure = m_cSelectedStreams;
    LONGLONG llInitialTimestamp = LONGLONG_MAX;

    struct TimestampInfo
    {
        ULONGLONG ullNumSamples;
        LONGLONG llLastTimestamp;
        BOOL fDone;
    };

    TimestampInfo *paTimestampInfo = new TimestampInfo[ m_cStreams ];
    if( NULL == paTimestampInfo )
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    ZeroMemory( paTimestampInfo, sizeof( TimestampInfo ) * m_cStreams );

    while( cStreamsToConfigure > 0 )
    {
        DWORD dwStreamIndex;
        DWORD dwFlags;
        LONGLONG llTimestamp;
        TimestampInfo *pTimestampInfo = NULL;

        SAFE_RELEASE( pSample );

        hr = m_pSourceReader->ReadSample( (DWORD)MF_SOURCE_READER_ANY_STREAM,
                                          0,
                                          &dwStreamIndex,
                                          &dwFlags,
                                          &llTimestamp,
                                          &pSample );
        if( S_OK != hr )
        {
            _SetErrorDetails( hr, L"Failed in call to IMFSourceReader::ReadSample while detemining timestamp adjustment" );
            goto done;
        }

        if( dwStreamIndex >= m_cStreams )
        {
            hr = E_UNEXPECTED;
            goto done;
        }

        pTimestampInfo = &paTimestampInfo[dwStreamIndex];

        if( pTimestampInfo->fDone )
        {
            continue;
        }

        if( pSample != NULL )
        {
            if( m_fRemuxing )
            {
                //
                // When remuxing any of the streams, we have to start from the position
                // where the source seeked to (which may be earlier than the desired position
                // if we had to seek back to the previous key frame).  So, we'll just record
                // the first timestamp for each stream, and use the lowest timestamp as the
                // actual start position.
                //
                pTimestampInfo->llLastTimestamp = llTimestamp;
                pTimestampInfo->fDone = TRUE;
            }
            else
            {
                //
                // When transcoding all of the audio and video streams, we can be more precise
                // in where we start from.  The source reader will still seek to the previous
                // key frame, so we read samples out until we find the timestamp that is closest
                // to the desired seek position, and record the lowest timestamp across all streams.
                //

                if( 0 == pTimestampInfo->ullNumSamples )
                {
                    pTimestampInfo->llLastTimestamp = llTimestamp;
                }
                else
                {
                    const LONGLONG llPreviousDelta = _abs64( m_Options.llStartPosition - pTimestampInfo->llLastTimestamp );
                    const LONGLONG llCurrentDelta = _abs64( m_Options.llStartPosition - llTimestamp );

                    if( llCurrentDelta < llPreviousDelta )
                    {
                        //
                        // if we're closer to the desired start position,
                        // mark this as a candidate timestamp, and then 
                        // continue to check if the next timestamp is better
                        //
                        pTimestampInfo->llLastTimestamp = llTimestamp;
                    }
                    else
                    {
                        //
                        // if the previous candidate timestamp was closer,
                        // then keep the previous value, and we're done
                        //
                        pTimestampInfo->fDone = TRUE;
                    }

                }
            }

            pTimestampInfo->ullNumSamples++;
        }

        //
        // if the stream is EOS, mark that it is done even if we didn't pull any samples out for it
        //
        if( 0 != ( MF_SOURCE_READERF_ENDOFSTREAM & dwFlags ) )
        {
            pTimestampInfo->fDone = TRUE;
        }

        if( pTimestampInfo->fDone )
        {
            cStreamsToConfigure--;
        }
    }

    //
    // pick the lowest timestamp from all streams to use as the timestamp adjustment value
    //
    for( DWORD ii = 0; ii < m_cStreams; ii++ )
    {
        const TimestampInfo& info = paTimestampInfo[ii];

        if( !info.fDone || ( 0 == info.ullNumSamples ) )
        {
            continue;
        }

        if( info.llLastTimestamp < llInitialTimestamp )
        {
            llInitialTimestamp = info.llLastTimestamp;
        }
    }

    if( LONGLONG_MAX == llInitialTimestamp )
    {
        hr = E_FAIL;
        _SetErrorDetails( hr, L"Failed to determine the timestamp adjustment" );
    }

    m_llTimestampAdjustment = llInitialTimestamp;

done:

    SAFE_ARRAY_DELETE( paTimestampInfo );

    SAFE_RELEASE( pSample );

    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::_ProcessSamples()
{
    HRESULT hr = S_OK;

    DWORD cStreamsAtEOS = 0;

    // record the time we started the copy operation
    m_Timing.llStartTime = _GetQPCTime();

    //
    // main loop to process samples from the source reader
    //

    hr = m_pSinkWriter->BeginWriting();
    if( S_OK != hr )
    {
        _SetErrorDetails( hr, L"Failed in call to IMFSinkWriter::BeginWriting" );
        goto done;
    }

    for( ;; )
    {
        DWORD dwStreamIndex;
        DWORD dwFlags = 0;
        LONGLONG llTimestamp = 0;
        LONGLONG llAdjustedTimestamp = 0;
        IMFSample *pSample = NULL;

        hr = m_pSourceReader->ReadSample( (DWORD)MF_SOURCE_READER_ANY_STREAM,
                                          0,
                                          &dwStreamIndex,
                                          &dwFlags,
                                          &llTimestamp,
                                          &pSample );

        if( S_OK != hr )
        {
            _SetErrorDetails( hr, L"Failed in call to IMFSourceReader::ReadSample" );
            goto done;
        }

        if( dwStreamIndex >= m_cStreams )
        {
            hr = E_UNEXPECTED;
            goto done;
        }

        StreamInfo& streamInfo = m_paStreamInfo[dwStreamIndex];

        // before processing the sample, check if the format has changed
        if( dwFlags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED )
        {
            CHECK_HR( hr = _HandleFormatChange( dwStreamIndex, streamInfo.dwOutputStreamIndex ) );
        }

        // adjust the timestamp if a start position was set
        if( 0 != m_Options.llStartPosition )
        {
            llAdjustedTimestamp = llTimestamp - m_llTimestampAdjustment;
        }
        else
        {
            llAdjustedTimestamp = llTimestamp;
        }

        // the sample may be null if either end of stream or a stream tick is returned
        if( pSample )
        {
            if( m_fRemuxing ||
                ( 0 == m_Options.llStartPosition ) ||
                ( llTimestamp >= m_llTimestampAdjustment ) )
            {
                CHECK_HR( hr = pSample->SetSampleTime( llAdjustedTimestamp ) );

                if( !m_Options.fDurationSpecified ||
                    ( llAdjustedTimestamp <= (LONGLONG)m_Options.ullDuration ) )
                {
                    // ensure discontinuity is set for the first sample in each stream
                    if( 0 == streamInfo.cSamples )
                    {
                        CHECK_HR( hr = pSample->SetUINT32( MFSampleExtension_Discontinuity, TRUE ) );
                    }

                    streamInfo.cSamples++;

                    hr = m_pSinkWriter->WriteSample( streamInfo.dwOutputStreamIndex, pSample );
                    if( S_OK != hr )
                    {
                        _SetErrorDetails( hr, L"Failed in call to IMFSinkWriter::WriteSample" );
                        goto done;
                    }

                    if( m_pfnProgressCallback )
                    {
                        const DWORD dwPercentComplete = (DWORD)( max( llAdjustedTimestamp, 0 ) * 100 / m_Options.ullDuration );
                        m_pfnProgressCallback( dwPercentComplete, m_pvContext );
                    }
                }
                else
                {
                    dwFlags |= MF_SOURCE_READERF_ENDOFSTREAM;
                }
            }

            SAFE_RELEASE( pSample );
        }

        if( dwFlags & MF_SOURCE_READERF_STREAMTICK )
        {
            CHECK_HR( hr = m_pSinkWriter->SendStreamTick( streamInfo.dwOutputStreamIndex, llAdjustedTimestamp ) );
        }

        if( dwFlags & MF_SOURCE_READERF_ENDOFSTREAM )
        {
            if( !streamInfo.fEOS )
            {
                CHECK_HR( hr = m_pSinkWriter->NotifyEndOfSegment( streamInfo.dwOutputStreamIndex ) );

                streamInfo.fEOS = TRUE;
                cStreamsAtEOS++;
            }

            if( cStreamsAtEOS == m_cSelectedStreams )
            {
                break;
            }
        }
    }

    // record the time we started to finalize the generated file
    m_Timing.llFinalizeStartTime = _GetQPCTime();

    hr = m_pSinkWriter->Finalize();
    if( S_OK != hr )
    {
        _SetErrorDetails( hr, L"Failed in call to IMFSinkWriter::Finalize" );
        goto done;
    }

    // record the time that the copy operation completed
    m_Timing.llStopTime = _GetQPCTime();

done:

    if( S_OK != hr )
    {
        _SetErrorDetails( hr, L"Failed while processing samples" );
    }

    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::_HandleFormatChange(
    __in DWORD dwInputStreamIndex,
    __in DWORD dwTargetStreamIndex )
{
    HRESULT hr = S_OK;

    //
    // handle a format change notification from the source reader
    // by retrieving the current media type for the stream
    // and setting it as the new input media type for the sink writer
    //

    IMFMediaType *pMediaType = NULL;

    CHECK_HR( hr = m_pSourceReader->GetCurrentMediaType( dwInputStreamIndex,
                                                         &pMediaType ) );

    CHECK_HR( hr = m_pSinkWriter->SetInputMediaType( dwTargetStreamIndex,
                                                     pMediaType,
                                                     NULL ) );

done:

    SAFE_RELEASE( pMediaType );

    if( S_OK != hr )
    {
        _SetErrorDetails( hr, L"Failed while handling a format change" );
    }

    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::_CreateTargetVideoMediaType(
    __in IMFMediaType *pNativeMediaType,
    __out IMFMediaType **ppTargetMediaType )
{
    HRESULT hr = S_OK;

    IMFMediaType *pTargetMediaType = NULL;

    GUID guidSubtype;

    UINT32 unWidth;
    UINT32 unHeight;
    UINT32 unNumerator;
    UINT32 unDenominator;
    UINT32 unAspectX;
    UINT32 unAspectY;
    UINT32 unInterlaceMode;
    UINT32 unBitrate;

    *ppTargetMediaType = NULL;

    hr = m_pVideoOverrides->GetGUID( MF_MT_SUBTYPE, &guidSubtype );
    if( MF_E_ATTRIBUTENOTFOUND == hr )
    {
        hr = pNativeMediaType->GetGUID( MF_MT_SUBTYPE, &guidSubtype );
    }
    CHECK_HR( hr );

    CHECK_HR( hr = MFCreateMediaType( &pTargetMediaType ) );
    CHECK_HR( hr = pTargetMediaType->SetGUID( MF_MT_MAJOR_TYPE, MFMediaType_Video ) );
    CHECK_HR( hr = pTargetMediaType->SetGUID( MF_MT_SUBTYPE, guidSubtype ) );

    if( FAILED( MFGetAttributeSize( m_pVideoOverrides, MF_MT_FRAME_SIZE, &unWidth, &unHeight ) ) )
    {
        CHECK_HR( hr = MFGetAttributeSize( pNativeMediaType, MF_MT_FRAME_SIZE, &unWidth, &unHeight ) );
    }

    if( FAILED( MFGetAttributeRatio( m_pVideoOverrides, MF_MT_FRAME_RATE, &unNumerator, &unDenominator ) ) )
    {
        CHECK_HR( hr = MFGetAttributeRatio( pNativeMediaType, MF_MT_FRAME_RATE, &unNumerator, &unDenominator ) );
    }

    if( FAILED( MFGetAttributeRatio( m_pVideoOverrides, MF_MT_PIXEL_ASPECT_RATIO, &unAspectX, &unAspectY ) ) )
    {
        if( FAILED( MFGetAttributeRatio( pNativeMediaType, MF_MT_PIXEL_ASPECT_RATIO, &unAspectX, &unAspectY ) ) )
        {
            unAspectX = unAspectY = 1;
        }
    }

    if( FAILED( m_pVideoOverrides->GetUINT32( MF_MT_INTERLACE_MODE, &unInterlaceMode ) ) )
    {
        if( FAILED( pNativeMediaType->GetUINT32( MF_MT_INTERLACE_MODE, &unInterlaceMode ) ) ||
            ( MFVideoInterlace_Unknown == unInterlaceMode ) )
        {
            unInterlaceMode = MFVideoInterlace_Progressive;
        }
    }

    if( FAILED( m_pVideoOverrides->GetUINT32( MF_MT_AVG_BITRATE, &unBitrate ) ) )
    {
        unBitrate = MFGetAttributeUINT32( pNativeMediaType, MF_MT_AVG_BITRATE, 500000 );
    }

    CHECK_HR( hr = MFSetAttributeSize( pTargetMediaType, MF_MT_FRAME_SIZE, unWidth, unHeight ) );
    CHECK_HR( hr = MFSetAttributeRatio( pTargetMediaType, MF_MT_FRAME_RATE, unNumerator, unDenominator ) );
    CHECK_HR( hr = MFSetAttributeRatio( pTargetMediaType, MF_MT_PIXEL_ASPECT_RATIO, unAspectX, unAspectY ) );
    CHECK_HR( hr = pTargetMediaType->SetUINT32( MF_MT_INTERLACE_MODE, unInterlaceMode ) );
    CHECK_HR( hr = pTargetMediaType->SetUINT32( MF_MT_AVG_BITRATE, unBitrate ) );

    *ppTargetMediaType = pTargetMediaType;
    pTargetMediaType = NULL;

done:

    SAFE_RELEASE( pTargetMediaType );

    if( S_OK != hr )
    {
        _SetErrorDetails( hr, L"Failed to create target video media type" );
    }

    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::_CreateTargetAudioMediaType(
    __in IMFMediaType *pNativeMediaType,
    __out IMFMediaType **ppTargetMediaType )
{
    HRESULT hr = S_OK;

    GUID guidSubtype;

    IMFMediaType *pTargetMediaType = NULL;
    IMFCollection *pAvailableTypes = NULL;
    IUnknown *punkObject = NULL;
    IMFMediaType *pMediaType = NULL;

    DWORD dwAvailableTypes = 0;

    UINT32 unNumChannels;
    UINT32 unNumChannelsTarget;
    UINT32 unSampleRate;
    UINT32 unSampleRateTarget;

    BOOL fFoundMediaType = FALSE;

    *ppTargetMediaType = NULL;

    hr = m_pAudioOverrides->GetGUID( MF_MT_SUBTYPE, &guidSubtype );
    if( MF_E_ATTRIBUTENOTFOUND == hr )
    {
        hr = pNativeMediaType->GetGUID( MF_MT_SUBTYPE, &guidSubtype );
    }
    CHECK_HR( hr );

    //
    // special handling for WMAudio_Lossless since
    // MFTranscodeGetAudioOutputAvailableTypes does not enumerate this format
    //
    if( guidSubtype == MFAudioFormat_WMAudio_Lossless )
    {
        CHECK_HR( hr = _CreateWMALosslessMediaType( pNativeMediaType,
                                                    ppTargetMediaType ) );
        goto done;
    }

    if( FAILED( m_pAudioOverrides->GetUINT32( MF_MT_AUDIO_NUM_CHANNELS, &unNumChannelsTarget ) ) )
    {
        CHECK_HR( hr = pNativeMediaType->GetUINT32( MF_MT_AUDIO_NUM_CHANNELS, &unNumChannelsTarget ) );
    }

    if( FAILED( m_pAudioOverrides->GetUINT32( MF_MT_AUDIO_SAMPLES_PER_SECOND, &unSampleRateTarget ) ) )
    {
        CHECK_HR( hr = pNativeMediaType->GetUINT32( MF_MT_AUDIO_SAMPLES_PER_SECOND, &unSampleRateTarget ) );
    }

    // get a list of available media types by subtype
    CHECK_HR( hr = MFTranscodeGetAudioOutputAvailableTypes( guidSubtype, MFT_ENUM_FLAG_ALL, NULL, &pAvailableTypes ) );

    CHECK_HR( hr = pAvailableTypes->GetElementCount( &dwAvailableTypes ) );

    //
    // loop through the available media types looking for one that
    // matches the desired number of channels and sampling rate
    //
    for( DWORD ii = 0; ii < dwAvailableTypes; ii++ )
    {
        SAFE_RELEASE( punkObject );
        SAFE_RELEASE( pMediaType );

        CHECK_HR( hr = pAvailableTypes->GetElement( ii, &punkObject ) );
        CHECK_HR( hr = punkObject->QueryInterface( IID_PPV_ARGS( &pMediaType ) ) );

        CHECK_HR( hr = pMediaType->GetUINT32( MF_MT_AUDIO_NUM_CHANNELS, &unNumChannels ) );
        CHECK_HR( hr = pMediaType->GetUINT32( MF_MT_AUDIO_SAMPLES_PER_SECOND, &unSampleRate ) );

        if( ( unNumChannels == unNumChannelsTarget ) && 
            ( unSampleRate == unSampleRateTarget ) )
        {
            fFoundMediaType = TRUE;
            break;
        }
    }

    if( ! fFoundMediaType )
    {
        CHECK_HR( hr = MF_E_NO_MORE_TYPES );
    }

    CHECK_HR( hr = MFCreateMediaType( &pTargetMediaType ) );
    CHECK_HR( hr = pMediaType->CopyAllItems( pTargetMediaType ) );

    *ppTargetMediaType = pTargetMediaType;
    pTargetMediaType = NULL;

done:

    SAFE_RELEASE( pTargetMediaType );
    SAFE_RELEASE( pAvailableTypes );
    SAFE_RELEASE( punkObject );
    SAFE_RELEASE( pMediaType );

    if( S_OK != hr )
    {
        _SetErrorDetails( hr, L"Failed to create target audio media type" );
    }

    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CMFCopy::_CreateWMALosslessMediaType(
    __in IMFMediaType *pNativeMediaType,
    __out IMFMediaType **ppTargetMediaType )
{
    HRESULT hr = S_OK;

    IMFMediaType *pTargetMediaType = NULL;

    UINT32 unNumChannelsTarget;
    UINT32 unSampleRateTarget;

    *ppTargetMediaType = NULL;

    if( FAILED( m_pAudioOverrides->GetUINT32( MF_MT_AUDIO_NUM_CHANNELS, &unNumChannelsTarget ) ) )
    {
        CHECK_HR( hr = pNativeMediaType->GetUINT32( MF_MT_AUDIO_NUM_CHANNELS, &unNumChannelsTarget ) );
    }

    if( FAILED( m_pAudioOverrides->GetUINT32( MF_MT_AUDIO_SAMPLES_PER_SECOND, &unSampleRateTarget ) ) )
    {
        CHECK_HR( hr = pNativeMediaType->GetUINT32( MF_MT_AUDIO_SAMPLES_PER_SECOND, &unSampleRateTarget ) );
    }

    CHECK_HR( hr = MFCreateMediaType( &pTargetMediaType ) );

    CHECK_HR( hr = pTargetMediaType->SetGUID( MF_MT_MAJOR_TYPE, MFMediaType_Audio ) );
    CHECK_HR( hr = pTargetMediaType->SetGUID( MF_MT_SUBTYPE, MFAudioFormat_WMAudio_Lossless ) );
    CHECK_HR( hr = pTargetMediaType->SetUINT32( MF_MT_AUDIO_NUM_CHANNELS, unNumChannelsTarget ) );
    CHECK_HR( hr = pTargetMediaType->SetUINT32( MF_MT_AUDIO_SAMPLES_PER_SECOND, unSampleRateTarget ) );
    CHECK_HR( hr = pTargetMediaType->SetUINT32( MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE ) );

    if( 44100 == unSampleRateTarget )
    {
        CHECK_HR( hr = pTargetMediaType->SetUINT32( MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 144004 ) );
        CHECK_HR( hr = pTargetMediaType->SetUINT32( MF_MT_AUDIO_BLOCK_ALIGNMENT, 13375 ) );
        CHECK_HR( hr = pTargetMediaType->SetUINT32( MF_MT_AUDIO_BITS_PER_SAMPLE, 16 ) );
    }
    else if( 48000 == unSampleRateTarget )
    {
        CHECK_HR( hr = pTargetMediaType->SetUINT32( MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 144000 ) );
        CHECK_HR( hr = pTargetMediaType->SetUINT32( MF_MT_AUDIO_BLOCK_ALIGNMENT, 12288 ) );
        CHECK_HR( hr = pTargetMediaType->SetUINT32( MF_MT_AUDIO_BITS_PER_SAMPLE, 24 ) );
    }
    else if( 88200 == unSampleRateTarget )
    {
        CHECK_HR( hr = pTargetMediaType->SetUINT32( MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 144004 ) );
        CHECK_HR( hr = pTargetMediaType->SetUINT32( MF_MT_AUDIO_BLOCK_ALIGNMENT, 13375 ) );
        CHECK_HR( hr = pTargetMediaType->SetUINT32( MF_MT_AUDIO_BITS_PER_SAMPLE, 24 ) );
    }
    else if( 96000 == unSampleRateTarget )
    {
        CHECK_HR( hr = pTargetMediaType->SetUINT32( MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 144000 ) );
        CHECK_HR( hr = pTargetMediaType->SetUINT32( MF_MT_AUDIO_BLOCK_ALIGNMENT, 12288 ) );
        CHECK_HR( hr = pTargetMediaType->SetUINT32( MF_MT_AUDIO_BITS_PER_SAMPLE, 24 ) );
    }
    else
    {
        CHECK_HR( hr = MF_E_NO_MORE_TYPES );
    }

    *ppTargetMediaType = pTargetMediaType;
    pTargetMediaType = NULL;

done:

    SAFE_RELEASE( pTargetMediaType );

    if( S_OK != hr )
    {
        _SetErrorDetails( hr, L"Failed to create target WMAudio_Lossless media type" );
    }

    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
LONGLONG CMFCopy::_GetQPCTime( void )
{
    LARGE_INTEGER li;
    QueryPerformanceCounter( &li );

    return( li.QuadPart );
}

///////////////////////////////////////////////////////////////////////////////
void CMFCopy::_SetErrorDetails(
    __in HRESULT hrError,
    __in LPCWSTR wszError )
{
    // only store the first error that occurs
    if( S_OK == m_hrError )
    {
        m_hrError = hrError;
        m_wszError = wszError;
    }
}

