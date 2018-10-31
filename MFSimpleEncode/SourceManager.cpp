// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "Common.h"
#include "SourceManager.h"

///////////////////////////////////////////////////////////////////////////////
CSourceManager::CSourceManager()
{
    m_pSrc = NULL;
    m_pPD = NULL;
    m_pMFSrcResolver = NULL;
    m_hnsDuration = 0;
}

///////////////////////////////////////////////////////////////////////////////
CSourceManager::~CSourceManager()
{
    Reset();
    SAFE_RELEASE( m_pMFSrcResolver );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CSourceManager::Init()
{
    HRESULT hr = S_OK;

    if( NULL == m_pMFSrcResolver )
    {
        CHECK_HR( hr = MFCreateSourceResolver( &m_pMFSrcResolver ) );
    }

done:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CSourceManager::Load( __in LPCWSTR pszFilePath )
{
    HRESULT hr = S_OK;
    MF_OBJECT_TYPE objType;
    IMFPresentationDescriptor* pPD = NULL;
    IMFMediaSource* pSrc = NULL;

    SAFE_RELEASE( m_pSrc );
    SAFE_RELEASE( m_pPD );
    m_hnsDuration = 0;

    // Get the source out of the url.
    CHECK_HR( hr = m_pMFSrcResolver->CreateObjectFromURL(  pszFilePath,
                        MF_RESOLUTION_MEDIASOURCE | MF_RESOLUTION_READ | MF_RESOLUTION_CONTENT_DOES_NOT_HAVE_TO_MATCH_EXTENSION_OR_MIME_TYPE,
                        NULL,
                        &objType,
                        (IUnknown**)&pSrc ) );

    CHECK_HR( hr = pSrc->CreatePresentationDescriptor( &pPD ) );
    hr = pPD->GetUINT64( MF_PD_DURATION, &m_hnsDuration );
    if( FAILED( hr ) )
    {
        hr = S_OK;
        m_hnsDuration = 0;
    }

    m_pSrc = pSrc;
    pSrc = NULL;
    m_pPD = pPD;
    pPD = NULL;

done:
    SAFE_RELEASE( pPD );
    SAFE_RELEASE( pSrc );
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CSourceManager::GetSource( __out IMFMediaSource** ppSrc )
{
    HRESULT hr = S_OK;

    if( NULL == ppSrc )
    {
        hr = E_INVALIDARG;
        goto done;
    }

    *ppSrc = m_pSrc;

    SAFE_ADDREF( *ppSrc );

done:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
void CSourceManager::GetDuration( __out UINT64* phnsDuration )
{
    assert( phnsDuration );
    *phnsDuration = m_hnsDuration;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CSourceManager::GetPresentationDescriptor( __out IMFPresentationDescriptor** ppPD )
{
    HRESULT hr = S_OK;

    if( NULL == ppPD )
    {
        hr = E_INVALIDARG;
        goto done;
    }

    *ppPD = m_pPD;
    SAFE_ADDREF( *ppPD );

done:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
void CSourceManager::ShutdownSource()
{
    if( m_pSrc )
    {
        m_pSrc->Shutdown();
        m_pSrc->Release();
        m_pSrc = NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////
void CSourceManager::Reset()
{
    ShutdownSource();
    m_hnsDuration = 0;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CSourceManager::GetCharacteristics( __out DWORD* pdwCharacteristics )
{
    HRESULT hr = S_OK;

    if( NULL  == m_pSrc )
    {
        hr = E_POINTER;
        goto done;
    }

    CHECK_HR( hr = m_pSrc->GetCharacteristics( pdwCharacteristics ) );

done:
    return hr;
}
