// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "common.h"
#include "EventHandler.h"
#include "encodeengine.h"

///////////////////////////////////////////////////////////////////////////////
CEventHandler::CEventHandler(): m_CB(this, &CEventHandler::OnInvoke)
{
    m_pMEG = NULL;
    m_hrStatus = S_OK;
    m_cRef = 0;
    m_hWaitEvent = NULL;
    m_pNotifier = NULL;
    m_meType = 0;
}

///////////////////////////////////////////////////////////////////////////////
CEventHandler::~CEventHandler()
{
    Reset();
    RemoveNotifier();

    if( NULL != m_hWaitEvent )
    {
        CloseHandle( m_hWaitEvent );
    }
}

///////////////////////////////////////////////////////////////////////////////
LONG CEventHandler::AddRef()
{
    LONG cRef = InterlockedIncrement( &m_cRef );
    return cRef;
}

///////////////////////////////////////////////////////////////////////////////
LONG CEventHandler::Release()
{
    LONG cRef = InterlockedDecrement( &m_cRef );

    if( cRef == 0 )
    {
        delete this;
    }

    return cRef;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CEventHandler::Init()
{
    HRESULT hr = S_OK;

    m_hWaitEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    if( NULL == m_hWaitEvent )
    {
        hr = E_UNEXPECTED;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CEventHandler::AddProvider( __in IMFMediaEventGenerator* pMEG )
{
    HRESULT hr = S_OK;
    if( NULL == pMEG )
    {
        hr = E_INVALIDARG;
        goto done;
    }

    SAFE_RELEASE( m_pMEG );
    m_pMEG = pMEG;
    m_pMEG->AddRef();

    CHECK_HR( hr = m_pMEG->BeginGetEvent( &m_CB, NULL ) );

done:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CEventHandler::AddNotifier( __in CEncodeEngine* pEncoder )
{
    HRESULT hr = S_OK;

    if( NULL == pEncoder )
    {
        hr = E_INVALIDARG;
        goto done;
    }

    m_pNotifier = pEncoder;
    m_pNotifier->AddRef();

done:
     return hr;
}

///////////////////////////////////////////////////////////////////////////////
void CEventHandler::RemoveNotifier()
{
    SAFE_RELEASE( m_pNotifier );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CEventHandler::OnInvoke( __in IMFAsyncResult *pAsyncResult )
{
    HRESULT hr = S_OK;
    IMFMediaEvent* pEvent = NULL;
    MediaEventType meType = MEUnknown;
    HRESULT hrStatus = S_OK;

    CHECK_HR( hr = m_pMEG->EndGetEvent(pAsyncResult, &pEvent) );

    CHECK_HR( hr = pEvent->GetType(&meType) );

    CHECK_HR( hr = pEvent->GetStatus(&hrStatus) );

    CHECK_HR( hr = HandleEvent( meType, hrStatus ) );

    CHECK_HR( hr = m_pMEG->BeginGetEvent(&m_CB, NULL) );

done:
    SAFE_RELEASE(pEvent);
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CEventHandler::HandleEvent( MediaEventType meType, HRESULT hrStatus )
{
    HRESULT hr = S_OK;

    //
    // If the event encoder is waiting for the event, signal the event.
    //
    if( meType == m_meType )
    {
        m_hrStatus = hrStatus;

        if( NULL == m_hWaitEvent )
        {
            CHECK_HR( hr = E_UNEXPECTED );
        }

        if( 0 == SetEvent( m_hWaitEvent ) )
        {
            CHECK_HR( hr = HRESULT_FROM_WIN32( GetLastError() ) );
        }
    }
    else if( m_pNotifier && ( MEError == meType ) ) //If the event is error event, notify the error to the listener.
    {
        m_pNotifier->OnError( hrStatus );
    }

done:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CEventHandler::SetWaitingEvent( MediaEventType meType )
{
    HRESULT hr = S_OK;

    if( m_meType != 0 )
    {
        CHECK_HR( hr = E_UNEXPECTED );
    }

    m_meType = meType;


done:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CEventHandler::Wait( __inout HRESULT* phrStatus, DWORD dwMilliseconds )
{
    HRESULT hr = S_OK;
    DWORD dwTimeoutStatus = 0;

    dwTimeoutStatus = WaitForSingleObject( m_hWaitEvent, dwMilliseconds );
    if( dwTimeoutStatus != WAIT_OBJECT_0 )
    {
        CHECK_HR( hr = E_FAIL );
    }
    else
    {
        *phrStatus = m_hrStatus;
    }

done:
    HRESULT hrDone = DoneWithWait();
    if( SUCCEEDED( hr ) )
    {
        hr = hrDone;
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CEventHandler::DoneWithWait()
{
    HRESULT hr = S_OK;

    m_meType = 0;
    m_hrStatus = S_OK;

    if( 0 == ResetEvent( m_hWaitEvent ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CEventHandler::Reset()
{
    HRESULT hr = S_OK;

    SAFE_RELEASE( m_pMEG );
    hr = DoneWithWait();

    return hr;
}
