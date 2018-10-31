// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

#include <mfidl.h>
#include <wtypes.h>
#include "Common.h"
#include "sourcemanager.h"
#include "profilebuilder.h"
#include "eventhandler.h"

class CEncodeEngine
{
public:
    CEncodeEngine();
    ~CEncodeEngine();
    LONG AddRef();
    LONG Release();

    HRESULT Init();
    HRESULT Load( __in CEncodeConfig* pConfig );
    HRESULT Start();
    HRESULT Stop();
    HRESULT Close();
    HRESULT Shutdown();

    HRESULT GetDuration( __out UINT64* phnsDuration );
    HRESULT GetTime( __out UINT64* phnsTime );
    HRESULT GetErrorStatus( __out HRESULT* phrError );

    void OnError( __in HRESULT hrError );

private:
    HRESULT ValidateConfig( __in CEncodeConfig* pConfig );
    HRESULT Apply();
    HRESULT Reset();
    HRESULT ConfigTopology( __in IMFMediaSource* pSrc, __in IMFTranscodeProfile* pProfile );

private:
    CSourceManager m_SrcManager;
    CAtlStringW m_OutputFilePath;
    CProfileBuilder m_ProfileBuilder;
    IMFMediaSession* m_pSession;
    CEventHandler* m_pEventHandler;
    HRESULT m_hrError;
    LONG m_cRef;
};
