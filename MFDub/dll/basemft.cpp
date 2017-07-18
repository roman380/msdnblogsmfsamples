// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE
//
// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "stdafx.h"

#include "basemft.h"
#include "mferror.h"

//////////////////////////////////////////////////////////////////
// C1in1outTypeHandler
// This class is a base 'type handler' for 1 input, 1 output MFTs.
// Type handling is similar for a lot of MFTs (especially MFDub MFTs,
// which must be 1 input, 1 output with RGB for video or PCM/Float
// for audio) so it can be beneficial to abstract type handling
// out when implementing a number of MFTs.  This base type handler
// uses a static set of available types supplied by a derived class.
C1in1outTypeHandler::C1in1outTypeHandler()
    : m_cbInputSize(0)
    , m_cbOutputSize(0)
    , m_dwInputStreamFlags(0)
    , m_dwOutputStreamFlags(0)
    , m_cInputAvTypes(0)
    , m_cOutputAvTypes(0)
{
}

C1in1outTypeHandler::~C1in1outTypeHandler()
{
    for(DWORD i = 0; i < m_cInputAvTypes; i++)
    {
        m_pInputAvTypes[i]->Release();
    }
    delete[] m_pInputAvTypes;
    
    for(DWORD i = 0; i < m_cOutputAvTypes; i++)
    {
        m_pOutAvTypes[i]->Release();
    }
    delete[] m_pOutAvTypes;
}

HRESULT C1in1outTypeHandler::GetStreamCount(DWORD* pcInputStreams, DWORD* pcOutputStreams)
{
    if(NULL == pcInputStreams || NULL == pcOutputStreams)
    {
        return E_POINTER;
    }

    *pcInputStreams = 1;
    *pcOutputStreams = 1;

    return S_OK;
}

HRESULT C1in1outTypeHandler::GetStreamLimits(DWORD* pdwInputMinimum, DWORD* pdwInputMaximum, DWORD* pdwOutputMinimum, DWORD* pdwOutputMaximum)
{
    HRESULT hr = S_OK;

    if (pdwInputMinimum)
    {
        *pdwInputMinimum = 1;
    }
    if (pdwInputMaximum)
    {
        *pdwInputMaximum = 1;
    }
    if (pdwOutputMinimum)
    {
        *pdwOutputMinimum = 1;
    }
    if (pdwOutputMaximum)
    {
        *pdwOutputMaximum = 1;
    }

    return S_OK;
}

HRESULT C1in1outTypeHandler::GetInputStreamInfo(DWORD dwInputStreamID, MFT_INPUT_STREAM_INFO* pStreamInfo)
{
    if(dwInputStreamID > 0)
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    if(pStreamInfo == NULL)
    {
        return E_POINTER;
    }

    pStreamInfo->hnsMaxLatency = 0;
    pStreamInfo->dwFlags = m_dwInputStreamFlags;
    pStreamInfo->cbSize = m_cbInputSize;
    pStreamInfo->cbMaxLookahead = 0;
    pStreamInfo->cbAlignment = 0;

    return S_OK;
}

HRESULT C1in1outTypeHandler::GetOutputStreamInfo(DWORD dwOutputStreamID, MFT_OUTPUT_STREAM_INFO* pStreamInfo)
{
    if(dwOutputStreamID > 0)
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    if(pStreamInfo == NULL)
    {
        return E_POINTER;
    }

    pStreamInfo->dwFlags = m_dwOutputStreamFlags;
    pStreamInfo->cbSize = m_cbOutputSize;
    pStreamInfo->cbAlignment = 0;

    return S_OK;
}

HRESULT C1in1outTypeHandler::GetInputAvailableType(DWORD dwInputStreamID, DWORD dwTypeIndex, IMFMediaType** ppType)
{
    HRESULT hr = S_OK;
    CComPtr<IMFMediaType> pInType;

    if(NULL == ppType)
    {
        return E_POINTER;
    }

    if( dwInputStreamID > 0 )
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    if(dwTypeIndex >= m_cInputAvTypes)
    {
        return MF_E_NO_MORE_TYPES;
    }

    hr = MFCreateMediaType(ppType);
    if(SUCCEEDED(hr))
    {
        hr = m_pInputAvTypes[dwTypeIndex]->CopyAllItems(*ppType);
    }

    return hr;
}

HRESULT C1in1outTypeHandler::GetOutputAvailableType(DWORD dwOutputStreamID, DWORD dwTypeIndex, IMFMediaType** ppType)
{
    HRESULT hr = S_OK;

    if(NULL == ppType)
    {
        return E_POINTER;
    }

    if( dwOutputStreamID > 0 )
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }
 
    if( dwTypeIndex >= m_cOutputAvTypes )
    {
        return MF_E_NO_MORE_TYPES;
    }

    hr = MFCreateMediaType(ppType);
    if(SUCCEEDED(hr))
    {
        hr = m_pOutAvTypes[dwTypeIndex]->CopyAllItems(*ppType);
    }

    return hr;
}

HRESULT C1in1outTypeHandler::SetInputType(DWORD dwInputStreamID, IMFMediaType* pType, DWORD dwFlags)
{
    HRESULT hr = S_OK;

    if(dwInputStreamID > 0)
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    DWORD dwEqFlags = 0;
    
    BOOL fValid = FALSE;
    
    for(DWORD i = 0; i < m_cInputAvTypes; i++)
    {
        hr = pType->IsEqual(m_pInputAvTypes[i], &dwEqFlags);
    
        if(hr == S_OK || (dwEqFlags & MF_MEDIATYPE_EQUAL_MAJOR_TYPES && dwEqFlags & MF_MEDIATYPE_EQUAL_FORMAT_TYPES && dwEqFlags & MF_MEDIATYPE_EQUAL_FORMAT_DATA))
        {
            fValid = TRUE;
        }
    }
    
    if(!fValid)
    {
        return MF_E_INVALIDMEDIATYPE;
    }
       
    if(dwFlags & MFT_SET_TYPE_TEST_ONLY)
    {
        return S_OK;
    }

    m_spInputType = pType;

    OnInputTypeChanged();

    return hr;
}

HRESULT C1in1outTypeHandler::SetOutputType(DWORD dwOutputStreamID, IMFMediaType* pType, DWORD dwFlags)
{
    if(dwOutputStreamID > 0)
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    DWORD dwEqFlags = 0;
    HRESULT hr;
    BOOL fValid = FALSE;
    for(DWORD i = 0; i < m_cOutputAvTypes; i++)
    {
        hr = pType->IsEqual(m_pOutAvTypes[i], &dwEqFlags);
    
        if(hr == S_OK || (dwEqFlags & MF_MEDIATYPE_EQUAL_MAJOR_TYPES && dwEqFlags & MF_MEDIATYPE_EQUAL_FORMAT_TYPES && dwEqFlags & MF_MEDIATYPE_EQUAL_FORMAT_DATA))
        {
            fValid = TRUE;
        }
    }

    if(dwFlags & MFT_SET_TYPE_TEST_ONLY)
    {
        return S_OK;
    }
    
    m_spOutputType = pType;

    OnOutputTypeChanged();

    return S_OK;
}

HRESULT C1in1outTypeHandler::GetInputCurrentType(DWORD dwInputStreamID, IMFMediaType** ppType)
{
    HRESULT hr = S_OK;
    
    if( dwInputStreamID > 0 )
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    if(NULL == m_spInputType.p)
    {
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    }

    hr = MFCreateMediaType(ppType);
    if(SUCCEEDED(hr))
    {
        hr = m_spInputType->CopyAllItems(*ppType);
    }

    return hr;
}

HRESULT C1in1outTypeHandler::GetOutputCurrentType(DWORD dwOutputStreamID, IMFMediaType** ppType)
{
    HRESULT hr = S_OK;

    if( dwOutputStreamID > 0 )
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    if(NULL == m_spOutputType.p)
    {
        return MF_E_TRANSFORM_TYPE_NOT_SET;
    }

    hr = MFCreateMediaType(ppType);
    if(SUCCEEDED(hr))
    {
        hr = m_spOutputType->CopyAllItems(*ppType);
    }

    return hr;
}

HRESULT C1in1outTypeHandler::SetInputAvTypeCount(DWORD cInputAvTypes)
{
    m_pInputAvTypes = new IMFMediaType*[cInputAvTypes];
    
    if(!m_pInputAvTypes)
    {
        return E_OUTOFMEMORY;
    }
    
    m_cInputAvTypes = cInputAvTypes;
    
    return S_OK;
}

void C1in1outTypeHandler::SetInputAvType(DWORD dwIndex, IMFMediaType* pType)
{
    m_pInputAvTypes[dwIndex] = pType;
    pType->AddRef();
}

HRESULT C1in1outTypeHandler::SetOutputAvTypeCount(DWORD cOutputAvTypes)
{
    m_pOutAvTypes = new IMFMediaType*[cOutputAvTypes];
    
    if(!m_pOutAvTypes)
    {
        return E_OUTOFMEMORY;
    }
    
    m_cOutputAvTypes = cOutputAvTypes;
    
    return S_OK;
}

void C1in1outTypeHandler::SetOutputAvType(DWORD dwIndex, IMFMediaType* pType)
{
    m_pOutAvTypes[dwIndex] = pType;
    pType->AddRef();
}

//////////////////////////////////////////////////////////////////
// AMFTransform

void AMFTransform::Init(AMFTTypeHandler* pTypeHandler)
{
    m_pTypeHandler = pTypeHandler;
}

HRESULT AMFTransform::GetStreamCount(DWORD* pcInputStreams, DWORD* pcOutputStreams)
{
    return m_pTypeHandler->GetStreamCount(pcInputStreams, pcOutputStreams);
}

HRESULT AMFTransform::GetStreamLimits(DWORD* pdwInputMinimum, DWORD* pdwInputMaximum, DWORD* pdwOutputMinimum, DWORD* pdwOutputMaximum)
{
    return m_pTypeHandler->GetStreamLimits(pdwInputMinimum, pdwInputMaximum, pdwOutputMinimum, pdwOutputMaximum);
}

HRESULT AMFTransform::GetInputStreamInfo(DWORD dwInputStreamIndex, MFT_INPUT_STREAM_INFO* pStreamInfo)
{
    return m_pTypeHandler->GetInputStreamInfo(dwInputStreamIndex, pStreamInfo);
}

HRESULT AMFTransform::GetOutputStreamInfo(DWORD dwOutputStreamIndex, MFT_OUTPUT_STREAM_INFO* pStreamInfo)
{
    return m_pTypeHandler->GetOutputStreamInfo(dwOutputStreamIndex, pStreamInfo);
}

HRESULT AMFTransform::GetInputAvailableType(DWORD dwInputStreamIndex, DWORD dwTypeIndex, IMFMediaType** ppType)
{
    return m_pTypeHandler->GetInputAvailableType(dwInputStreamIndex, dwTypeIndex, ppType);
}

HRESULT AMFTransform::GetOutputAvailableType(DWORD dwOutputStreamIndex, DWORD dwTypeIndex, IMFMediaType** ppType)
{
    return m_pTypeHandler->GetOutputAvailableType(dwOutputStreamIndex, dwTypeIndex, ppType);
}

HRESULT AMFTransform::SetInputType(DWORD dwInputStreamIndex, IMFMediaType* pType, DWORD dwFlags)
{
    return m_pTypeHandler->SetInputType(dwInputStreamIndex, pType, dwFlags);
}

HRESULT AMFTransform::SetOutputType(DWORD dwOutputStreamIndex, IMFMediaType* pType, DWORD dwFlags)
{
    return m_pTypeHandler->SetOutputType(dwOutputStreamIndex, pType, dwFlags);
}

HRESULT AMFTransform::GetInputCurrentType(DWORD dwInputStreamIndex, IMFMediaType** ppType)
{
    return m_pTypeHandler->GetInputCurrentType(dwInputStreamIndex, ppType);
}

HRESULT AMFTransform::GetOutputCurrentType(DWORD dwOutputStreamIndex, IMFMediaType** ppType)
{
    return m_pTypeHandler->GetOutputCurrentType(dwOutputStreamIndex, ppType);
}