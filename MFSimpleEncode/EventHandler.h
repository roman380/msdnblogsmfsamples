// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once


class CEncodeEngine;
class CEventHandler
{
public:
    CEventHandler();
    ~CEventHandler();
    LONG AddRef();
    LONG Release();

    HRESULT Init();
    HRESULT AddProvider( __in IMFMediaEventGenerator* pMEG );
    HRESULT AddNotifier( __in CEncodeEngine* pEncoder );
    void RemoveNotifier();
    HRESULT Reset();

    HRESULT SetWaitingEvent( MediaEventType meType );
    HRESULT Wait( __inout HRESULT* phrStatus, DWORD dwMilliseconds );

private:
    HRESULT OnInvoke( __in IMFAsyncResult *pAsyncResult);
    HRESULT HandleEvent( MediaEventType meType, HRESULT hrStatus );
    HRESULT DoneWithWait();

private:
    IMFMediaEventGenerator* m_pMEG;
    CAsyncCallback<CEventHandler> m_CB;
    MediaEventType m_meType;
    HRESULT m_hrStatus;
    HANDLE m_hWaitEvent;
    LONG m_cRef;
    CEncodeEngine* m_pNotifier;
};
