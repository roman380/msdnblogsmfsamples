#pragma once

#include "wmcontainer.h"

class CTopologyBuilder;

class CMediaEventHandler
{
public:
    virtual void HandleFinished() = 0;
    virtual void HandleError(HRESULT hr) = 0;
};

class CPlaybackHandler
{
public:
    CPlaybackHandler();
    ~CPlaybackHandler();
    
    HRESULT InitSession(CTopologyBuilder* pTopologyBuilder, HWND hVideoOutputWindow);
    HRESULT Start(MFTIME hnsStartPos);
    HRESULT Stop();
    HRESULT Shutdown();
    
    HRESULT GetPresentationTime(MFTIME* phnsTime);
    HRESULT GetPresentationDuration(MFTIME* phnsDuration);
    
    void SetEventHandler(CMediaEventHandler* pEventHandler) { m_pEventHandler = pEventHandler; }

    // for callbacks
    LONG AddRef(); 
    LONG Release();
protected:
    HRESULT OnSessionEvent(IMFAsyncResult* pResult);   
    HRESULT HandleEvent(IMFMediaEvent* pEvent);
    HRESULT HandleNotifyPresentationTime(IMFMediaEvent* pEvent);
    
private:
    CComPtr<IMFMediaSource> m_spSource;
    CComPtr<IMFMediaSession> m_spSession;
    CTopologyBuilder* m_pTopologyBuilder;
    CMediaEventHandler* m_pEventHandler;
    bool m_fTopologySet;
    bool m_fStartOnTopologySet;
    MFTIME m_hnsStartTime;
    MFTIME m_hnsStartPresTime;
    MFTIME m_hnsOffsetTime;
    MFTIME m_hnsStartTimeAtOutput;
    bool m_fReceivedPresentationTime;
    LONG m_cRef;

    CAsyncCallback<CPlaybackHandler> m_cb;
};