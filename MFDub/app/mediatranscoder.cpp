// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE
//
// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "stdafx.h"

#include "mediatranscoder.h"
#include "topologybuilder.h"
#include "wmcodecdsp.h"
#include "mferror.h"
#include "assert.h"

////////////////////////////////////////////////

// This container map was taken from the sink writer's extension map.  This
// map would need to be updated for any changes in the sink writer's extension
// map, or could be removed if the sink writer exposes the container type in the
// future.
CMediaTranscoder::CONTAINER_MAP CMediaTranscoder::m_ContainerMap[] =
{
    { 4, {L"asf", L"wma", L"wmv", L"wm"}, MFAudioFormat_WMAudioV8, MFVideoFormat_WMV3 },
    { 1, {L"mp3"}, MFAudioFormat_MP3, GUID_NULL },
    { 5, {L"mp4", L"m4a", L"m4v", L"mp4v", L"mov"}, MFAudioFormat_AAC, MFVideoFormat_H264 },
    { 4, {L"3gp", L"3g2", L"3gpp", L"3gp2"}, MFAudioFormat_AAC, MFVideoFormat_H264 }
};

CMediaTranscoder::CMediaTranscoder(LPCWSTR szInputURL, CVideoTransformApplier* pTransformApplier, CAudioTransformApplier* pAudioTransformApplier,
    UINT32 unFrameRateN, UINT32 unFrameRateD, LPCWSTR szOutputURL, int iEncodeQuality, CMediaTranscodeEventHandler* pEventHandler, 
    HRESULT& hr)
    : m_cRef(1)
    , m_pTransformApplier(pTransformApplier)
    , m_pAudioTransformApplier(pAudioTransformApplier)
    , m_dwAudioStreamIndex(0)
    , m_dwVideoStreamIndex(0)
    , m_iEncodeQuality(iEncodeQuality)
    , m_pEventHandler(pEventHandler)
{
    const GUID guidDesiredAudioSubtype = MFAudioFormat_PCM;
    // Right now MFDub transforms only support RGB32.  In the future,
    // a more efficient color type like YV12 would be better for
    // transcode.
    const GUID guidDesiredVideoSubtype = MFVideoFormat_RGB32;

    GUID guidTargetAudioSubtype;
    GUID guidTargetVideoSubtype;
    CComPtr<IMFAttributes> spAttributes;

    CHECK_HR( hr = MFCreateAttributes(&spAttributes, 1) );
    CHECK_HR( hr = spAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, 1) );

    CHECK_HR( hr = MFCreateSourceReaderFromURL(szInputURL, spAttributes, &m_spSourceReader) );
    CHECK_HR( hr = ConfigureSourceReader(guidDesiredAudioSubtype, guidDesiredVideoSubtype) );

    CHECK_HR( hr = MFCreateSinkWriterFromURL(szOutputURL, NULL, NULL, &m_spSinkWriter) );
    CHECK_HR( hr = GetContainerTypesForOutputFile(szOutputURL, guidTargetAudioSubtype, guidTargetVideoSubtype) );
    CHECK_HR( hr = ConfigureSinkWriter(guidDesiredAudioSubtype, guidDesiredVideoSubtype, guidTargetAudioSubtype, guidTargetVideoSubtype, unFrameRateN, unFrameRateD) );

    // Offload transcode to another thread so that it does not block the UI.
    m_spTranscodeThread.Attach(new CTranscodeThread(m_spSourceReader, m_spSinkWriter, m_dwAudioStreamIndex, 
        m_dwVideoStreamIndex, m_pTransformApplier, m_pAudioTransformApplier, m_pEventHandler, hr));
    if(NULL == m_spTranscodeThread.m_p) 
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    CHECK_HR( hr = hr );

done:
    ;
}

CMediaTranscoder::~CMediaTranscoder()
{
}

HRESULT CMediaTranscoder::BeginTranscode()
{
    m_spTranscodeThread->StartProcessing();

    return S_OK;
}

LONG CMediaTranscoder::AddRef()
{
    LONG cRef = ::InterlockedIncrement(&m_cRef);
    
    return cRef;
}

LONG CMediaTranscoder::Release()
{
    LONG cRef = ::InterlockedDecrement(&m_cRef);
    
    if(cRef == 0)
    {
        delete this;
    }
    
    return cRef;
}

// GetPresentationTime
// Return transcode progress as a last timestamp processed in *phnsTime
HRESULT CMediaTranscoder::GetPresentationTime(MFTIME* phnsTime)
{
    HRESULT hr = S_OK;
    MF_SINK_WRITER_STATISTICS AudioStats;
    MF_SINK_WRITER_STATISTICS VideoStats;

    AudioStats.cb = sizeof(AudioStats);
    VideoStats.cb = sizeof(VideoStats);

    CHECK_HR( hr = m_spSinkWriter->GetStatistics(m_dwAudioStreamIndex, &AudioStats) );
    CHECK_HR( hr = m_spSinkWriter->GetStatistics(m_dwVideoStreamIndex, &VideoStats) );

    // Return the lesser timestamp between the audio and video streams, in case one
    // stream is processing ahead of the other.
    if(AudioStats.llLastTimestampProcessed < VideoStats.llLastTimestampProcessed)
    {
        *phnsTime = AudioStats.llLastTimestampProcessed;
    }
    else
    {
        *phnsTime = VideoStats.llLastTimestampProcessed;
    }

done:
    return hr;
}

// GetPresentationDuration
// Return the total duration of the output file in *phnsDuration.
HRESULT CMediaTranscoder::GetPresentationDuration(MFTIME* phnsDuration)
{
    HRESULT hr = S_OK;
    PROPVARIANT var;

    PropVariantInit(&var);

    CHECK_HR( hr = m_spSourceReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &var) );

    if(VT_UI8 != var.vt)
    {
        CHECK_HR( hr = MF_E_ATTRIBUTENOTFOUND );
    }

    *phnsDuration = var.uhVal.QuadPart;

done:
    PropVariantClear(&var);

    return hr;
}

// GetContainerTypesForOutputFile
// Based upon the output file name, determine the proper compressed audio and video subtype for the respective output streams.
HRESULT CMediaTranscoder::GetContainerTypesForOutputFile(LPCWSTR szOutputFile, GUID& guidAudioSubtype, GUID& guidVideoSubtype)
{
    HRESULT hr = MF_E_INVALIDMEDIATYPE;
    LPCWSTR szExtension = NULL;

    szExtension = wcsrchr(szOutputFile, L'.');
    if(NULL == szExtension)
    {
        CHECK_HR( hr = E_INVALIDARG );
    }
    szExtension++;

    for(DWORD i = 0; i < ARRAYSIZE(m_ContainerMap); i++)
    {
        const CONTAINER_MAP& ContainerMap = m_ContainerMap[i];

        for(DWORD j = 0; j < ContainerMap.cExtensions; j++)
        {
            if(0 == _wcsicmp(szExtension, ContainerMap.szExtensions[j]))
            {
                guidAudioSubtype = ContainerMap.guidAudioSubtype;
                guidVideoSubtype = ContainerMap.guidVideoSubtype;
                hr = S_OK;
                goto done;
            }
        }
    }

done:
    return hr;
}

// ConfigureSourceReader
// Set stream selection and media types for the source reader.
HRESULT CMediaTranscoder::ConfigureSourceReader(const GUID& guidDesiredAudioSubtype, const GUID& guidDesiredVideoSubtype)
{
    assert(m_spSourceReader);

    HRESULT hr = S_OK;

    for(DWORD i = 0; ; i++)
    {
        BOOL fSelected;
        CComPtr<IMFMediaType> spNativeType;
        CComPtr<IMFMediaType> spDesiredType;
        GUID guidMajorType;

        // Keep iterating through stream numbers until MF_E_INVALIDSTREAMNUMBER is returned
        hr = m_spSourceReader->GetStreamSelection(i, &fSelected);
        if(MF_E_INVALIDSTREAMNUMBER == hr)
        {
            hr = S_OK;
            break;
        }
        CHECK_HR( hr = hr );

        if(!fSelected)
        {
            continue;
        }

        CHECK_HR( hr = m_spSourceReader->GetNativeMediaType(i, 0, &spNativeType) );
        CHECK_HR( hr = spNativeType->GetMajorType(&guidMajorType) );

        CHECK_HR( hr = MFCreateMediaType(&spDesiredType) );
        CHECK_HR( hr = spDesiredType->SetGUID(MF_MT_MAJOR_TYPE, guidMajorType) );

        // Ignore streams that are not audio or video.  MFDub does not handle
        // other kinds of streams at this time.
        // Form an output media type by replacing the native subtype with the
        // desired subtype.  The source reader will insert a decoder if necessary.
        if(MFMediaType_Audio == guidMajorType)
        {
            CHECK_HR( hr = spDesiredType->SetGUID(MF_MT_SUBTYPE, guidDesiredAudioSubtype) );
            CHECK_HR( hr = m_spSourceReader->SetCurrentMediaType(i, NULL, spDesiredType) );
        }
        else if(MFMediaType_Video == guidMajorType)
        {
            CHECK_HR( hr = spDesiredType->SetGUID(MF_MT_SUBTYPE, guidDesiredVideoSubtype) );
            CHECK_HR( hr = m_spSourceReader->SetCurrentMediaType(i, NULL, spDesiredType) );
        }
        else
        {
            CHECK_HR( hr = m_spSourceReader->SetStreamSelection(i, FALSE) );
        }
    }

done:
    return hr;
}

// ConfigureSinkWriter
// Create output streams and set their media types.
HRESULT CMediaTranscoder::ConfigureSinkWriter(const GUID& guidInputAudioSubtype, const GUID& guidInputVideoSubtype, const GUID& guidTargetAudioSubtype, const GUID& guidTargetVideoSubtype, UINT32 unFrameRateN, UINT32 unFrameRateD)
{
    assert(m_spSourceReader);
    assert(m_spSinkWriter);

    HRESULT hr = S_OK;

    for(DWORD i = 0; ; i++)
    {
        BOOL fSelected;
        CComPtr<IMFMediaType> spCurrentType;
        CComPtr<IMFMediaType> spTargetType;
        GUID guidMajorType;

        // Keep iterating through stream numbers until MF_E_INVALIDSTREAMNUMBER is returned
        hr = m_spSourceReader->GetStreamSelection(i, &fSelected);
        if(MF_E_INVALIDSTREAMNUMBER == hr)
        {
            hr = S_OK;
            break;
        }
        CHECK_HR( hr = hr );

        if(!fSelected)
        {
            continue;
        }

        CHECK_HR( hr = m_spSourceReader->GetCurrentMediaType(i, &spCurrentType) );
        CHECK_HR( hr = spCurrentType->GetMajorType(&guidMajorType) );

        if(MFMediaType_Audio == guidMajorType)
        {
            CHECK_HR( hr = MakeTargetAudioType(spCurrentType, guidTargetAudioSubtype, &spTargetType) );
            CHECK_HR( hr = m_spSinkWriter->AddStream(spTargetType, &m_dwAudioStreamIndex) );
            CHECK_HR( hr = m_spSinkWriter->SetInputMediaType(m_dwAudioStreamIndex, spCurrentType, NULL) );
        }
        else if(MFMediaType_Video == guidMajorType)
        {
            CComPtr<IMFMediaType> spWriterInputType;

            CHECK_HR( hr = MakeTargetVideoType(spCurrentType, guidTargetVideoSubtype, &spTargetType) );
            CHECK_HR( hr = m_spSinkWriter->AddStream(spTargetType, &m_dwVideoStreamIndex) );

            CHECK_HR( hr = MFCreateMediaType(&spWriterInputType) );
            CHECK_HR( hr = spCurrentType->CopyAllItems(spWriterInputType) );

            // Transforms may have modified the video frame size, so update this to the proper size in the output media type
            CHECK_HR( hr = MFSetAttributeRatio(spWriterInputType, MF_MT_FRAME_SIZE, m_pTransformApplier->GetOutputWidth(), m_pTransformApplier->GetOutputHeight()) );
            CHECK_HR( hr = MFSetAttributeRatio(spWriterInputType, MF_MT_FRAME_RATE, unFrameRateN, unFrameRateD) );

            CHECK_HR( hr = m_spSinkWriter->SetInputMediaType(m_dwVideoStreamIndex, spWriterInputType, NULL) );
        }
    }

done:
    return hr;
}

// MakeTargetAudioType
// For an input type and a desired output subtype, determine the 'best' supported audio type to encode to.
HRESULT CMediaTranscoder::MakeTargetAudioType(IMFMediaType* pInputType, const GUID& guidTargetAudioSubtype, IMFMediaType** ppTargetType)
{
    HRESULT hr;
    CComPtr<IMFCollection> spTypeCollection;
    CComPtr<IMFMediaType> spBestMatchType;
    DWORD cTypes;

    CHECK_HR( hr = MFTranscodeGetAudioOutputAvailableTypes(guidTargetAudioSubtype, MFT_ENUM_FLAG_ALL, NULL, &spTypeCollection) );
    CHECK_HR( hr = spTypeCollection->GetElementCount(&cTypes) );

    for(DWORD i = 0; i < cTypes; i++)
    {
        CComPtr<IUnknown> spTypeUnk;
        CComPtr<IMFMediaType> spType;

        CHECK_HR( hr = spTypeCollection->GetElement(i, &spTypeUnk) );
        CHECK_HR( hr = spTypeUnk->QueryInterface( IID_PPV_ARGS(&spType)) );

        if(NULL == spBestMatchType || IsBetterAudioTypeMatch(pInputType, spType, spBestMatchType))
        {
            spBestMatchType = spType;
        }
    }

    if(NULL == spBestMatchType.p)
    {
        CHECK_HR( hr = MF_E_INVALIDMEDIATYPE );
    }

    *ppTargetType = spBestMatchType.Detach();

done:
    return hr;
}

// IsBetterAudioTypeMatch
// Determine whether pPossibleType is better than pCurrentType for the input format pInputType.
bool CMediaTranscoder::IsBetterAudioTypeMatch(IMFMediaType* pInputType, IMFMediaType* pPossibleType, IMFMediaType* pCurrentType)
{
    // The ideal output type matches the input type exactly.  The transcode cannot produce information that is
    // not in the input stream, so overshooting only needlessly increases the file size.  Undershooting decreases
    // the playback quality.
    // Type restrictions are applied in order.
    //   1) Channel count - Losing channels (for example, going stereo to mono) causes an obvious drop in quality
    //   2) Sample rate - Lowering sample rate loses information, but not particularly noticable as long as the
    //        rate drop is not large.  Increasing the sample rate just causes redundant information.
    //   3) Bitrate - Similar to sample rate.  However, a good encoder could potentially produce the same quality 
    //        output file at a lower bitrate

    // Prefer equivalent channel count.  If equivalent channel count cannot be found, prefer the
    // higher channel count that is not over the input channel count.  Transcode should not increase
    // the number of channels.
    UINT32 nInputChannels = MFGetAttributeUINT32(pInputType, MF_MT_AUDIO_NUM_CHANNELS, 0);
    if(0 != nInputChannels)
    {
        UINT32 nPossibleChannels = MFGetAttributeUINT32(pPossibleType, MF_MT_AUDIO_NUM_CHANNELS, 0);
        UINT32 nCurrentChannels = MFGetAttributeUINT32(pCurrentType, MF_MT_AUDIO_NUM_CHANNELS, 0);

        if( 0 == nPossibleChannels 
            || (nPossibleChannels != nInputChannels && nCurrentChannels == nInputChannels)
            || (nPossibleChannels != nInputChannels && nCurrentChannels != nInputChannels && nPossibleChannels < nCurrentChannels)
            )
        {
            return false;
        }
    }

    // Prefer the closest sample rate to the input format.  Output sample rate can either be above or below
    // the target, as long as it is close.
    UINT32 nInputSampleRate = MFGetAttributeUINT32(pInputType, MF_MT_AUDIO_SAMPLES_PER_SECOND, 0);
    if(0 != nInputSampleRate)
    {
        UINT32 nPossibleSampleRate = MFGetAttributeUINT32(pPossibleType, MF_MT_AUDIO_SAMPLES_PER_SECOND, 0);
        UINT32 nCurrentSampleRate = MFGetAttributeUINT32(pCurrentType, MF_MT_AUDIO_SAMPLES_PER_SECOND, 0);

        INT32 nDiffPoss = INT32(nInputSampleRate) - nPossibleSampleRate;
        INT32 nDiffCurr = INT32(nInputSampleRate) - nCurrentSampleRate;
        if( abs(nDiffPoss) > abs(nDiffCurr) )
        {
            return false;
        }
    }

    // Prefer the closest bitrate to the input format, but prefer overshooting rather than undershooting.
    UINT32 nInputByteRate = MFGetAttributeUINT32(pInputType, MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 0);
    if(0 != nInputByteRate)
    {
        UINT32 nPossibleByteRate = MFGetAttributeUINT32(pPossibleType, MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 0);
        UINT32 nCurrentByteRate = MFGetAttributeUINT32(pCurrentType, MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 0);

        INT32 nDiffPoss = INT32(nInputByteRate) - nPossibleByteRate;
        INT32 nDiffCurr = INT32(nInputByteRate) - nCurrentByteRate;
        if(nDiffPoss > 0 && nDiffCurr < 0 || abs(nDiffPoss) > abs(nDiffCurr) )
        {
            return false;
        }
    }

    return true;
}

// MakeTargetVideoType
// Form a complete video type based upon the input type and the transform chain
HRESULT CMediaTranscoder::MakeTargetVideoType(IMFMediaType* pInputType, const GUID& guidTargetVideoSubtype, IMFMediaType** ppTargetType)
{
    HRESULT hr = S_OK;
    CComPtr<IMFMediaType> spTargetType;
    UINT32 unFrameRateN;
    UINT32 unFrameRateD;
    UINT32 unPARN;
    UINT32 unPARD;
    UINT32 unInterlaceMode;

    CHECK_HR( hr = MFCreateMediaType(&spTargetType) );
    CHECK_HR( hr = spTargetType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video) );
    CHECK_HR( hr = spTargetType->SetGUID(MF_MT_SUBTYPE, guidTargetVideoSubtype) );
    CHECK_HR( hr = MFSetAttributeSize(spTargetType, MF_MT_FRAME_SIZE, m_pTransformApplier->GetOutputWidth(), m_pTransformApplier->GetOutputHeight()) );

    CHECK_HR( hr = MFGetAttributeRatio(pInputType, MF_MT_FRAME_RATE, &unFrameRateN, &unFrameRateD) );
    CHECK_HR( hr = MFSetAttributeRatio(spTargetType, MF_MT_FRAME_RATE, unFrameRateN, unFrameRateD) );

    // If pixel aspect ratio and interlace mode are unknown, it is better to leave them unset.
    if( SUCCEEDED(MFGetAttributeRatio(pInputType, MF_MT_PIXEL_ASPECT_RATIO, &unPARN, &unPARD)) )
    {
        CHECK_HR( hr = MFSetAttributeRatio(spTargetType, MF_MT_PIXEL_ASPECT_RATIO, unPARN, unPARD) );
    }

    if( SUCCEEDED(pInputType->GetUINT32(MF_MT_INTERLACE_MODE, &unInterlaceMode)) )
    {
        CHECK_HR( hr = spTargetType->SetUINT32(MF_MT_INTERLACE_MODE, unInterlaceMode) );
    }

    CHECK_HR( hr = spTargetType->SetUINT32(MF_MT_AVG_BITRATE, m_iEncodeQuality) );
    
    *ppTargetType = spTargetType.Detach();

done:
    return hr;
}

///////////////////////////////////////////////////////////////

CTranscodeThread::CTranscodeThread(IMFSourceReader* pSourceReader, IMFSinkWriter* pSinkWriter, 
    DWORD dwAudioStreamIndex, DWORD dwVideoStreamIndex, CVideoTransformApplier* pApplier, CAudioTransformApplier* pAudioApplier,
    CMediaTranscodeEventHandler* pEventHandler, HRESULT& hr)
    : m_spSourceReader(pSourceReader)
    , m_spSinkWriter(pSinkWriter)
    , m_dwAudioStreamIndex(dwAudioStreamIndex)
    , m_dwVideoStreamIndex(dwVideoStreamIndex)
    , m_pTransformApplier(pApplier)
    , m_pAudioTransformApplier(pAudioApplier)
    , m_pEventHandler(pEventHandler)
    , m_hThread(NULL)
    , m_hStartEvent(NULL)
{
    m_hStartEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    if(NULL == m_hStartEvent)
    {
        CHECK_HR( hr = E_FAIL );
    }

    m_hThread = ::CreateThread(NULL, 0, &TranscodeThreadProc, this, 0, NULL);
    if(NULL == m_hThread)
    {
        hr = E_FAIL;
    }

done:
    ;
}

CTranscodeThread::~CTranscodeThread()
{
    if(m_hStartEvent)
    {
        ::CloseHandle(m_hStartEvent);
    }

    if(NULL != m_hThread)
    {
        ::CloseHandle(m_hThread);
    }
}

// StartProcessing
// Start transcoding data.  Set the start event to let the transcode thread know it is
// OK to continue.
void CTranscodeThread::StartProcessing()
{
    ::SetEvent(m_hStartEvent);
}

// TranscodeThreadProc
// Thread function for the transcode thread.  Reads samples in a loop, processes them
// through the transform chain, and writes them to the sink writer.
DWORD WINAPI CTranscodeThread::TranscodeThreadProc(LPVOID lpParam)
{
    HRESULT hr = S_OK;
    CTranscodeThread* pThis = reinterpret_cast<CTranscodeThread*>(lpParam);
    bool fAudioStreamEnded = false;
    bool fVideoStreamEnded = false;

    ::WaitForSingleObject(pThis->m_hStartEvent, INFINITE);

    CHECK_HR( hr = pThis->m_spSinkWriter->BeginWriting() );

    while(!fAudioStreamEnded || !fVideoStreamEnded)
    {
        CComPtr<IMFSample> spSample;
        DWORD dwStreamIndex = 0;
        DWORD dwFlags = 0;
        MFTIME hnsSample = 0;

        // Keep reading until a sample, end of stream, or error.  Ignore stream ticks and such
        // since nothing is done with them.  The ReadSample call may block, which is why this
        // task is running on an alternate thread.  Alternatively, the source reader could be
        // used in asynchronous mode.  Then the ReadSample call would not block and the sample
        // would be supplied in a callback function.
        while(!(dwFlags & MF_SOURCE_READERF_ENDOFSTREAM) && !spSample.p)
        {
            CHECK_HR( hr = pThis->m_spSourceReader->ReadSample(MF_SOURCE_READER_ANY_STREAM, 0, &dwStreamIndex, &dwFlags, &hnsSample, &spSample) );
        }

        if(pThis->m_dwAudioStreamIndex == dwStreamIndex)
        {
            if(dwFlags & MF_SOURCE_READERF_ENDOFSTREAM)
            {
                fAudioStreamEnded = true;
                continue;
            }

            CComPtr<IMFSample> spOutSample;
            CHECK_HR( hr = pThis->m_pAudioTransformApplier->ProcessSample(spSample, &spOutSample) );
            spSample = spOutSample;
        }
        else if(pThis->m_dwVideoStreamIndex == dwStreamIndex)
        {
            if(dwFlags & MF_SOURCE_READERF_ENDOFSTREAM)
            {
                fVideoStreamEnded = true;
                continue;
            }

            CComPtr<IMFSample> spOutSample;
            CHECK_HR( hr = pThis->m_pTransformApplier->ProcessSample(spSample, &spOutSample) );
            spSample = spOutSample;
        }

        // The WriteSample call will block if too much data is queued up for writing.  This is
        // a good thing for this thread, as otherwise the transcode would eat up lots of memory
        // by queueing up lots of uncompressed samples.  If a non-blocking WriteSample call is
        // desirable, MFDub could set the MF_SINK_WRITER_DISABLE_THROTTLING attribute on the 
        // attribute store passed to MFCreateSinkWriterFromURL.
        CHECK_HR( hr = pThis->m_spSinkWriter->WriteSample(dwStreamIndex, spSample) );
    }

    CHECK_HR( hr = pThis->m_spSinkWriter->Finalize() );
    
    pThis->m_pEventHandler->HandleFinished();

done:
    if(FAILED(hr))
    {
        pThis->m_pEventHandler->HandleError(hr);
    }

    return 0;
}
