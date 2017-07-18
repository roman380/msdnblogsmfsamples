// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE
//
// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "stdafx.h"

#include "playbackhandler.h"
#include "topologybuilder.h"
#include "mferror.h"

CPlaybackHandler::CPlaybackHandler()
    : m_pTopologyBuilder(NULL)
    , m_pEventHandler(NULL)
    , m_fTopologySet(false)
    , m_fStartOnTopologySet(false)
    , m_hnsStartTime(0)
    , m_fReceivedPresentationTime(false)
    , m_cRef(0)
    , m_cb(this, &CPlaybackHandler::OnSessionEvent)
{
}

CPlaybackHandler::~CPlaybackHandler()
{
    delete m_pTopologyBuilder;
}

// InitSession
// Create a media session, build a topology, and set it on the session.
HRESULT CPlaybackHandler::InitSession(CTopologyBuilder* pTopologyBuilder, HWND hVideoOutWnd)
{
    HRESULT hr = S_OK;
    CComPtr<IMFTopology> spTopology;

    m_pTopologyBuilder = pTopologyBuilder;
    
    CHECK_HR( hr = MFCreateMediaSession(NULL, &m_spSession) );
    CHECK_HR( hr = m_spSession->BeginGetEvent(&m_cb, NULL) );
    
    CHECK_HR( hr = m_pTopologyBuilder->GetSource(&m_spSource) );

    if(m_pTopologyBuilder->HasAudioStream())
    {
        CComPtr<IMFActivate> spSARActivate;
        CHECK_HR( hr = MFCreateAudioRendererActivate(&spSARActivate) );
        CHECK_HR( hr = m_pTopologyBuilder->SetAudioStreamSink(spSARActivate, 0) );
    }
    
    if(m_pTopologyBuilder->HasVideoStream())
    {
        CComPtr<IMFActivate> spEVRActivate;
        CHECK_HR( hr = MFCreateVideoRendererActivate(hVideoOutWnd, &spEVRActivate) );
        CHECK_HR( hr = m_pTopologyBuilder->SetVideoStreamSink(spEVRActivate, 0) );
    }
    
    CHECK_HR( hr = m_pTopologyBuilder->GetTopology(&spTopology) );
    CHECK_HR( hr = m_spSession->SetTopology(0, spTopology) );
    
done:
    return hr;
}

// Start
// Start playback at hnsStartTime.
HRESULT CPlaybackHandler::Start(MFTIME hnsStartTime)
{
    HRESULT hr = S_OK;
    
    if(m_fTopologySet)
    {
        PROPVARIANT var;
        var.vt = VT_I8;
        var.hVal.QuadPart = hnsStartTime;
        CHECK_HR( hr = m_spSession->Start(NULL, &var) );
    }
    else
    {
        // The session is still processing the topology.  Once
        // the topology set event comes back, the start can
        // actually execute.  Starting a session without a topology
        // results in an MF_E_INVALIDREQUEST error.
        m_fStartOnTopologySet = true;
        m_hnsStartTime = hnsStartTime;
    }
    
done:
    return hr;
}

// Stop
// Stop the session.
HRESULT CPlaybackHandler::Stop()
{
    return m_spSession->Stop();
}

// Shutdown
// Shutdown both the session and the source to remove circular references.
HRESULT CPlaybackHandler::Shutdown()
{
    m_spSession->Shutdown();
    m_spSource->Shutdown();
    
    return S_OK;
}

// GetPresentationTime
// Get the current time from the presentation clock.
HRESULT CPlaybackHandler::GetPresentationTime(MFTIME* phnsTime)
{
    HRESULT hr = S_OK;
    CComPtr<IMFClock> spClock;
    CComPtr<IMFPresentationClock> spPresentationClock;
    
    CHECK_HR( hr = m_spSession->GetClock(&spClock) );
    CHECK_HR( hr = spClock->QueryInterface(IID_IMFPresentationClock, (void**) &spPresentationClock) );
    CHECK_HR( hr = spPresentationClock->GetTime(phnsTime) );

    if (m_fReceivedPresentationTime)
    {
        *phnsTime -= m_hnsOffsetTime;
    }
    
done:
    return( hr );
}

// GetPresentationDuration
// Get the duration of the media file from the presentation descriptor.
HRESULT CPlaybackHandler::GetPresentationDuration(MFTIME* phnsTime)
{
    HRESULT hr = S_OK;
    IMFPresentationDescriptor *pPD = NULL;

    CHECK_HR( hr = m_spSource->CreatePresentationDescriptor( &pPD ) );
    CHECK_HR( hr = pPD->GetUINT64( MF_PD_DURATION, (UINT64*) phnsTime ) );

done:
    if(pPD) pPD->Release();
    
    return( hr );
}
    
// OnSessionEvent
// Async callback function for session events.
HRESULT CPlaybackHandler::OnSessionEvent(IMFAsyncResult* pResult)
{
    HRESULT hr;
    CComPtr<IMFMediaEvent> spEvent;

    CHECK_HR( hr = m_spSession->EndGetEvent(pResult, &spEvent) );
    CHECK_HR( hr = HandleEvent(spEvent) );
    
done:
    if(FAILED(hr) && hr != MF_E_SHUTDOWN)
    {
        if(m_pEventHandler) m_pEventHandler->HandleError(hr);
    }

    return hr;
}

// HandleEvent
// Process relevant session events.
HRESULT CPlaybackHandler::HandleEvent(IMFMediaEvent* pEvent)
{
    HRESULT hr;
    HRESULT hrEvent;
    MediaEventType met;
    PROPVARIANT var;

    PropVariantInit(&var);
    
    CHECK_HR( hr = pEvent->GetType(&met) );
    CHECK_HR( hr = pEvent->GetStatus(&hrEvent) );
    CHECK_HR( hr = pEvent->GetValue(&var) );

    CHECK_HR( hr = hrEvent );
    
    BOOL fGetMoreEvents = TRUE;
    switch(met)
    {
        case MESessionTopologySet:
            m_fTopologySet = true;
            if(m_fStartOnTopologySet)
            {
                // This is a delayed Start from a Start call that came in
                // before the topology was ready.
                Start(m_hnsStartTime);
            }
            
            break;
        case MESessionEnded:
            if(m_pEventHandler) m_pEventHandler->HandleFinished();
            fGetMoreEvents = FALSE;
            break;
        case MESessionNotifyPresentationTime:
            HandleNotifyPresentationTime(pEvent);
            break;
    }
    
    if(fGetMoreEvents)
    {
        CHECK_HR( hr = m_spSession->BeginGetEvent(&m_cb, NULL) );
    }
    
done:
    PropVariantClear(&var);

    return hr;
}

// HandleNotifyPresentationTime
// Process the MESessionNotifyPresentationTime event.  Save the offsets to be used
// in calculating the real media time from the clock time.  Only TIME_OFFSET is
// important to this simple player.  START_PRESENTATION_TIME is important for
// playlist playback, and TIME_AT_OUTPUT is useful for calculating buffer amounts
// when an MFT is buffering data in the topology.
HRESULT CPlaybackHandler::HandleNotifyPresentationTime(IMFMediaEvent* pEvent)
{
    HRESULT hr = S_OK;

    CHECK_HR( hr = pEvent->GetUINT64(MF_EVENT_START_PRESENTATION_TIME, (UINT64*) &m_hnsStartPresTime) );
    CHECK_HR( hr = pEvent->GetUINT64(MF_EVENT_PRESENTATION_TIME_OFFSET, (UINT64*) &m_hnsOffsetTime) );
    CHECK_HR( hr = pEvent->GetUINT64(MF_EVENT_START_PRESENTATION_TIME_AT_OUTPUT, (UINT64*) &m_hnsStartTimeAtOutput) );

    m_fReceivedPresentationTime = true;
done:
    return hr;
}

HRESULT CPlaybackHandler::AddRef()
{
    LONG cRef = ::InterlockedIncrement(&m_cRef);
    
    return cRef;
}

HRESULT CPlaybackHandler::Release()
{
    LONG cRef = ::InterlockedDecrement(&m_cRef);
    
    if(cRef == 0)
    {
        delete this;
    }
    
    return m_cRef;
}