// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE
//
// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "stdafx.h"

#include "sampleprovider.h"
#include "mferror.h"
#include <assert.h>

/////////////////////////////////////////////////////
//
CSampleProvider::CSampleProvider()
    : m_tLastSample(0)
    , m_dwCurrentFrameNumber(0)
{
}

CSampleProvider::~CSampleProvider()
{
}

// LoadSource
// Build an index, then create an uncompressed source reader for szSourceURL.
HRESULT CSampleProvider::LoadSource(LPCWSTR szSourceURL)
{
    HRESULT hr = S_OK;
    m_dwVideoStreamId = 0xFFFFFFFF;
    m_dwAudioStreamId = 0xFFFFFFFF;
    CComPtr<IMFAttributes> spAttributes;

     
    CHECK_HR( hr = MFCreateAttributes(&spAttributes, 1) );
    CHECK_HR( hr = spAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, 1) );

    CHECK_HR( hr = MFCreateSourceReaderFromURL(szSourceURL, spAttributes, &m_spSourceReader) );

    for(DWORD i = 0; ; i++)
    {
        CComPtr<IMFMediaType> spNativeType;
        CComPtr<IMFMediaType> spOutputType;

        hr = m_spSourceReader->GetNativeMediaType(i, 0, &spNativeType);
        if(MF_E_INVALIDSTREAMNUMBER == hr)
        {
            hr = S_OK;
            break;
        }
        else
        {
            CHECK_HR( hr = hr );
        }

        // Convert the native media type to an uncompressed media type that
        // MFDub can work with.
        GUID gidMajorType;
        CHECK_HR( hr = spNativeType->GetMajorType(&gidMajorType) );
        if(gidMajorType == MFMediaType_Video)
        {
            m_dwVideoStreamId = i;
            m_spVideoType = spNativeType;

            CHECK_HR( hr = MFGetAttributeSize(spNativeType, MF_MT_FRAME_SIZE, &m_unSampleWidth, &m_unSampleHeight) );

            CHECK_HR( hr = CreateOutputMediaType(spNativeType, &spOutputType) );

            CHECK_HR( hr = m_spSourceReader->SetCurrentMediaType(i, NULL, spOutputType) );
        }
        else if(gidMajorType == MFMediaType_Audio)
        {
            m_dwAudioStreamId = i;
            m_spAudioType = spNativeType;

            CHECK_HR( hr = CreateOutputMediaType(spNativeType, &spOutputType) );

            CHECK_HR( hr = m_spSourceReader->SetCurrentMediaType(i, NULL, spOutputType) );
        }
    }

done:
    return hr;
}

// GetMediaDuration
// Return the duration of the media file in *ptDuration.
HRESULT CSampleProvider::GetMediaDuration(MFTIME* ptDuration)
{
    PROPVARIANT varAttribute;
    HRESULT hr = m_spSourceReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &varAttribute);

    if(SUCCEEDED(hr))
    {
        if(varAttribute.vt == VT_UI8)
        {
            *ptDuration = varAttribute.uhVal.QuadPart;
        }
        else
        {
            hr = MF_E_NOT_FOUND;
        }
    }

    return hr;
}

// GetSampleSize
// Return the width and height of the video frames in *punSampleWidth and *punSampleHeight.
HRESULT CSampleProvider::GetSampleSize(UINT32* punSampleWidth, UINT32* punSampleHeight)
{
    *punSampleWidth = m_unSampleWidth;
    *punSampleHeight = m_unSampleHeight;

    return S_OK;
}

// GetVideoSample
// Get the video frame at time tNext.
HRESULT CSampleProvider::GetVideoSample(MFTIME tNext, IMFSample** ppSample, DWORD* pdwSampleNum, bool* pfIsKey)
{
    HRESULT hr = S_OK;
    
    CHECK_HR( hr = SeekTo(tNext, ppSample) );

    // Even with seeking, the source reader may not be producing the exact
    // frame that MFDub needs.  Keep reading frames until one comes out with
    // a stimestamp greater than or equal to the desired timestamp.
    while(*ppSample == NULL || m_tLastSample < tNext)
    {
        if(*ppSample)
        {
            (*ppSample)->Release();
            *ppSample = NULL;
        }
        
        while(NULL == *ppSample)
        {
            DWORD dwStreamId;
            DWORD dwFlags;
            MFTIME hnsSample;
            CHECK_HR( hr = m_spSourceReader->ReadSample(m_dwVideoStreamId, 0, &dwStreamId, &dwFlags, &hnsSample, ppSample) );
            if(dwFlags & MF_SOURCE_READERF_ENDOFSTREAM)
            {
                CHECK_HR( hr = MF_E_END_OF_STREAM );
            }
        }

        CHECK_HR( hr = (*ppSample)->GetSampleTime(&m_tLastSample) );
    } 
    
    *pdwSampleNum = 0;
    *pfIsKey = FALSE;
    
done:
    return hr;
}

// GetVideoSample
// Get the video frame with frame number dwFrameNumber.
HRESULT CSampleProvider::GetVideoSample(DWORD dwFrameNumber, IMFSample** ppSample, DWORD* pdwSampleNum, bool* pfIsKey)
{
    HRESULT hr = S_OK;
    CComPtr<IMFMediaType> spType;
    UINT32 unFrameRateN, unFrameRateD;
    LONGLONG khnsPerSecond = 10000000;

    CHECK_HR( hr = m_spSourceReader->GetCurrentMediaType(m_dwVideoStreamId, &spType) );
    CHECK_HR( MFGetAttributeRatio(spType, MF_MT_FRAME_RATE, &unFrameRateN, &unFrameRateD) );

    MFTIME tNext = MFllMulDiv(dwFrameNumber, LONGLONG(unFrameRateD) * khnsPerSecond, unFrameRateN, 0);
    
    CHECK_HR( hr = GetVideoSample(tNext, ppSample, pdwSampleNum, pfIsKey) );
    
done:
    return hr;
}

// GetAudioMediaType
// Return the media type of the audio stream.
IMFMediaType* CSampleProvider::GetAudioMediaType()
{
    return m_spAudioType.p;
}

// GetVideoMediaType
// Return the media type of the video stream.
IMFMediaType* CSampleProvider::GetVideoMediaType()
{
    return m_spVideoType.p;
}

// GetSourceReader
// Return the source reader instance.
IMFSourceReader* CSampleProvider::GetSourceReader()
{
    return m_spSourceReader.p;
}

// CreateOutputMediaType
// Given a native media type, generate an uncompressed media type that MFDub can work with.
HRESULT CSampleProvider::CreateOutputMediaType(IMFMediaType* pMediaTypeIn, IMFMediaType** ppMediaTypeOut)
{
    HRESULT hr = S_OK;

    CComPtr<IMFMediaType> spMediaTypeOut;
    CHECK_HR( hr = MFCreateMediaType(&spMediaTypeOut) );

    GUID gidMajorType;
    CHECK_HR( hr = pMediaTypeIn->GetMajorType(&gidMajorType) );

    if(gidMajorType == MFMediaType_Video)
    {
        // Always prefer RGB32 for video.  A YUV format like YV12 would be faster
        // as far as processing efficiency, but would require more development
        // work for transforms.  Perhaps in the future this can be parameterized.
        CHECK_HR( hr = spMediaTypeOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video) );
        CHECK_HR( hr = spMediaTypeOut->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32) );
        
        UINT64 unFrameSize;
        CHECK_HR( hr = pMediaTypeIn->GetUINT64(MF_MT_FRAME_SIZE, &unFrameSize) );
        CHECK_HR( hr = spMediaTypeOut->SetUINT64(MF_MT_FRAME_SIZE, unFrameSize) );
    }
    else if(gidMajorType == MFMediaType_Audio)
    {
        // Either PCM or IEEE Float audio is acceptable for audio.  Both are
        // pretty similar as far as difficulty to work with.
        GUID gidSubtype;
        CHECK_HR( hr = pMediaTypeIn->GetGUID(MF_MT_SUBTYPE, &gidSubtype) );
        
        CHECK_HR( hr = spMediaTypeOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio) );
        
        // Special case for uncompressed PCM audio sources, as there is no way currently for MF
        // to change this to float audio
        if(gidSubtype == MFAudioFormat_PCM)
        {
            CHECK_HR( hr = spMediaTypeOut->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM) );
        }
        else
        {
            CHECK_HR( hr = spMediaTypeOut->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float) );
        }
    }

    *ppMediaTypeOut = spMediaTypeOut;
    (*ppMediaTypeOut)->AddRef();
done:
    return hr;
}

// SeekTo
// Seek the source reader to the time hnsNext
HRESULT CSampleProvider::SeekTo(MFTIME hnsNext, IMFSample** ppSample)
{
    HRESULT hr = S_OK;
    PROPVARIANT varSeekPos;
    DWORD dwStreamId;
    DWORD dwFlags;
    MFTIME hnsSample;

    varSeekPos.vt = VT_I8;
    varSeekPos.hVal.QuadPart = hnsNext;
    CHECK_HR( hr = m_spSourceReader->SetCurrentPosition(GUID_NULL, varSeekPos) );
    CHECK_HR( hr = m_spSourceReader->ReadSample(m_dwVideoStreamId, 0, &dwStreamId, &dwFlags, &hnsSample, ppSample) );
        
    if(dwFlags & MF_SOURCE_READERF_ENDOFSTREAM)
    {
        m_tLastSample = 0x7FFFFFFFFFFFFFFF;
        hr = S_OK;
    }
    else 
    {
        CHECK_HR( hr = (*ppSample)->GetSampleTime(&m_tLastSample) );        
    }
        
done:
    return hr;
}

