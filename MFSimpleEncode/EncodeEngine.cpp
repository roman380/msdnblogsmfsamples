// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "EncodeEngine.h"

#define MAX_EVENT_WAIT_TIME 3000

///////////////////////////////////////////////////////////////////////////////
CEncodeEngine::CEncodeEngine()
{
    m_pEventHandler = NULL;
    m_pSession = NULL;
    m_hrError = S_OK;
    m_cRef = 0;
}

///////////////////////////////////////////////////////////////////////////////
CEncodeEngine::~CEncodeEngine()
{
    Shutdown();
}

///////////////////////////////////////////////////////////////////////////////
LONG CEncodeEngine::AddRef()
{
    LONG cRef = InterlockedIncrement( &m_cRef );
    return cRef;
}

///////////////////////////////////////////////////////////////////////////////
LONG CEncodeEngine::Release()
{
    LONG cRef = InterlockedDecrement( &m_cRef );

    if( cRef == 0 )
    {
        delete this;
    }

    return cRef;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CEncodeEngine::Init()
{
    HRESULT hr = S_OK;

    m_pEventHandler = new CEventHandler();
    if( NULL == m_pEventHandler )
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    m_pEventHandler->AddRef();

    CHECK_HR( hr = m_pEventHandler->Init() );
    CHECK_HR( hr = m_pEventHandler->AddNotifier( this ) );

done:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CEncodeEngine::Load( __in CEncodeConfig* pConfig )
{
    HRESULT hr = S_OK;
    IMFPresentationDescriptor* pPD = NULL;

    CHECK_HR( hr = ValidateConfig( pConfig ) );

    CHECK_HR( hr = Reset() );

    CHECK_HR( hr = m_SrcManager.Init() );
    CHECK_HR( hr = m_SrcManager.Load( (pConfig->strInputFile).GetBuffer() ) );

    //
    // Get the output file path
    //
    m_OutputFilePath.SetString( (pConfig->strOutputFile).GetBuffer() );

    //
    // Create a transcode profile
    //
    if( (pConfig->strProfile).GetLength() != 0 )
    {
        CHECK_HR( hr = m_ProfileBuilder.Load( (pConfig->strProfile).GetBuffer() ) );
    }
    else
    {
        CHECK_HR( hr = m_SrcManager.GetPresentationDescriptor( &pPD ) );
        CHECK_HR( hr = m_ProfileBuilder.CreateProfile( pPD, pConfig->enumTranscodeMode, (pConfig->strContainerName).GetBuffer() ) );
    }

    //
    // Create a partial topology
    //
    CHECK_HR( hr = Apply() );

done:
    SAFE_RELEASE( pPD );
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CEncodeEngine::Start()
{
    HRESULT hr = S_OK;
    HRESULT hrEventStatus = S_OK;
    PROPVARIANT varStart;
    PropVariantInit( &varStart );
    DWORD dwCharacteristics = 0;

    CHECK_HR( hr = m_SrcManager.GetCharacteristics( &dwCharacteristics ) );

    if( dwCharacteristics & MFMEDIASOURCE_CAN_SEEK )
    {
        varStart.vt = VT_I8;
    }
    else
    {
        varStart.vt = VT_EMPTY; //for unseekable files, start from the current position.
    }

    if( NULL == m_pSession )
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    CHECK_HR( hr = m_pEventHandler->SetWaitingEvent( MESessionStarted ) );
    CHECK_HR( hr = m_pSession->Start(NULL, &varStart ) );
    CHECK_HR( hr = m_pEventHandler->Wait( &hrEventStatus, MAX_EVENT_WAIT_TIME ) );
    hr = hrEventStatus;

done:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CEncodeEngine::Stop()
{
    HRESULT hr = S_OK;
    HRESULT hrEventStatus = S_OK;

    if( NULL == m_pSession )
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    CHECK_HR( hr = m_pEventHandler->SetWaitingEvent( MESessionStopped ) );
    CHECK_HR( hr = m_pSession->Stop() );
    CHECK_HR( hr = m_pEventHandler->Wait( &hrEventStatus, MAX_EVENT_WAIT_TIME ) );
    hr = hrEventStatus;

done:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//Close the encoding session. It will generate the encoded file.
///////////////////////////////////////////////////////////////////////////////
HRESULT CEncodeEngine::Close()
{
    HRESULT hr = S_OK;
    HRESULT hrEventStatus = S_OK;

    if( NULL == m_pSession )
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    CHECK_HR( hr = m_pEventHandler->SetWaitingEvent( MESessionClosed ) );
    CHECK_HR( hr = m_pSession->Close() );
    CHECK_HR( hr = m_pEventHandler->Wait( &hrEventStatus, MAX_EVENT_WAIT_TIME ) );
    hr = hrEventStatus;

done:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//Release the encoding session. It shutdown the sink and remove all stream sinks.
///////////////////////////////////////////////////////////////////////////////
HRESULT CEncodeEngine::Shutdown()
{
    if( m_pSession )
    {
        m_pSession->Shutdown();
        m_pSession->Release();
        m_pSession = NULL;
    }

    if( m_pEventHandler )
    {
        m_pEventHandler->RemoveNotifier();
        m_pEventHandler->Release();
        m_pEventHandler = NULL;
    }

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CEncodeEngine::GetDuration( __out UINT64* phnsDuration )
{
    HRESULT hr = S_OK;

    if( NULL == phnsDuration )
    {
        hr = E_INVALIDARG;
        goto done;
    }

    m_SrcManager.GetDuration( phnsDuration );

done:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CEncodeEngine::GetTime( __out UINT64* phnsTime )
{
    HRESULT hr = S_OK;
    IMFClock* pClock = NULL;
    IMFPresentationClock* pPresClock = NULL;
    MFTIME mfTime;

    if( NULL == phnsTime )
    {
        hr = E_INVALIDARG;
        goto done;
    }

    //Get presentation clock
    CHECK_HR( hr = m_pSession->GetClock( &pClock ) );
    CHECK_HR( hr = pClock->QueryInterface( IID_IMFPresentationClock, (void**)&pPresClock ) );
    CHECK_HR( hr = pPresClock->GetTime( &mfTime ) );

    *phnsTime = (UINT64)mfTime;

done:
    SAFE_RELEASE( pClock );
    SAFE_RELEASE( pPresClock );
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CEncodeEngine::GetErrorStatus( __out HRESULT* phrError )
{
    *phrError = m_hrError;
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
void CEncodeEngine::OnError( __in HRESULT hrError )
{
    m_hrError = hrError;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CEncodeEngine::Reset()
{
    HRESULT hr = S_OK;

    m_SrcManager.Reset();
    m_ProfileBuilder.Reset();

    if( m_pEventHandler )
    {
        hr = m_pEventHandler->Reset();
    }

    if( m_pSession )
    {
        m_pSession->Shutdown();
        m_pSession->Release();
        m_pSession = NULL;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CEncodeEngine::Apply()
{
    HRESULT hr = S_OK;
    IMFMediaSource* pSrc = NULL;
    IMFTranscodeProfile* pProfile = NULL;

    CHECK_HR( hr = m_SrcManager.GetSource( &pSrc ) );
    CHECK_HR( hr = m_ProfileBuilder.GetProfile( &pProfile ) );

    if( NULL != m_pSession )
    {
        CHECK_HR( hr = E_UNEXPECTED );
    }

    CHECK_HR( hr = MFCreateMediaSession( NULL, &m_pSession ) );

    CHECK_HR( hr = m_pEventHandler->AddProvider( m_pSession ) );

    //
    // Build a partial topology.
    //
    CHECK_HR( hr = ConfigTopology( pSrc, pProfile ) );

done:
    SAFE_RELEASE( pSrc );
    SAFE_RELEASE( pProfile );

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CEncodeEngine::ConfigTopology( __in IMFMediaSource* pSrc, __in IMFTranscodeProfile* pProfile )
{
    HRESULT hr = S_OK;
    HRESULT hrEventStatus = S_OK;
    IMFTopology* pTopo = NULL;

    CHECK_HR( hr = MFCreateTranscodeTopology( pSrc, m_OutputFilePath.GetBuffer(), pProfile, &pTopo ) );
    CHECK_HR( hr = m_pEventHandler->SetWaitingEvent( MESessionTopologySet ) );
    CHECK_HR( hr = m_pSession->SetTopology( 0, pTopo ) );
    CHECK_HR( hr = m_pEventHandler->Wait( &hrEventStatus, MAX_EVENT_WAIT_TIME ) );
    hr = hrEventStatus;

done:
    SAFE_RELEASE( pTopo );
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CEncodeEngine::ValidateConfig( __in CEncodeConfig* pConfig )
{
    HRESULT hr = S_OK;

    //
    // Must have the input file
    //
    if( 0 == (pConfig->strInputFile).GetLength() )
    {
        CHECK_HR( hr = E_INVALIDARG );
    }

    //
    // Must have the output file
    //
    if( 0 == (pConfig->strOutputFile).GetLength() )
    {
        CHECK_HR( hr = E_INVALIDARG );
    }

    //
    // When transcode profile is specified, cannot specify split audio/video stream mode.
    //
    if( ( (pConfig->strProfile).GetLength() != 0 ) && ( TranscodeMode_Default != pConfig->enumTranscodeMode ) )
    {
        CHECK_HR( hr = E_INVALIDARG );
    }

done:
    return hr;
}
