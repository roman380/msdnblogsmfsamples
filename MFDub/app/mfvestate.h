#pragma once

#include "mediatranscoder.h"

class CMfveState;
class CTimeBarControl;
class CSampleOutputWindow;
class CSampleProvider;
class CPlaybackHandler;
class CMediaTranscoder;
class CTransformApplier;
class CMainToolbar;
class CTransportToolbar;
class CMediaEventHandler;
class CMediaTranscodeEventHandler;

class CMfveState
{
public:
    CMfveState(CMfveState* pOldState, CTimeBarControl* pTimeBar);
    virtual ~CMfveState();
  
    CMfveState* GetOldState() { return m_pOldState; }
    virtual HRESULT HandleSeek(WORD wPos, WORD wPosMax, bool fEnd) = 0;
    virtual HRESULT HandleTimer() = 0;
    virtual HRESULT HandleNextKeyframe() { return E_NOTIMPL; }
    virtual HRESULT HandlePrevKeyframe() { return E_NOTIMPL; }
    virtual HRESULT HandleShowMetadata() { return E_NOTIMPL; }
    virtual HRESULT Activate() { return S_OK; }
  
    virtual bool CanStop() { return false; }
    
    static void SetMainToolbar(CMainToolbar* pToolbar) { ms_pMainToolbar = pToolbar; }
    static void SetTransportToolbar(CTransportToolbar* pToolbar) { ms_pTransportToolbar = pToolbar; }
    
protected:
    CTimeBarControl* PTimeBar() { return m_pTimeBar; }
    static CMainToolbar* PMainToolbar() { return ms_pMainToolbar; }
    static CTransportToolbar* PTransportToolbar() { return ms_pTransportToolbar; }
    
private:
    CTimeBarControl* m_pTimeBar;
    CMfveState* m_pOldState;
    static CMainToolbar* ms_pMainToolbar;
    static CTransportToolbar* ms_pTransportToolbar;
};

class CMfveClosedState
    : public CMfveState
{
public:
    CMfveClosedState(CMfveState* pOldState, CTimeBarControl* pTimeBar);
    
    virtual HRESULT HandleSeek(WORD wPos, WORD wPosMax, bool fEnd);
    virtual HRESULT HandleTimer();
    virtual HRESULT Activate();
};

class CMfveScrubState
    : public CMfveState
{
public:
    CMfveScrubState(CMfveState* pOldState, CTimeBarControl* pTimeBar, CSampleOutputWindow* pOutputWindow, 
        CSampleOutputWindow* pPreviewWindow, CVideoTransformApplier* pTransformApplier, CAudioTransformApplier* pAudioTransformApplier);
    ~CMfveScrubState();
    
    virtual HRESULT HandleSeek(WORD wPos, WORD wPosMax, bool fEnd);
    virtual HRESULT HandleTimer();
    virtual HRESULT HandleNextKeyframe();
    virtual HRESULT HandlePrevKeyframe();
    virtual HRESULT HandleShowMetadata();
    virtual HRESULT Activate();
    virtual HRESULT Init(LPCWSTR szFileName, UINT32* punFrameRateN, UINT32* punFrameRateD);

protected:
    virtual HRESULT PumpSample(MFTIME hnsSeekTime);
    virtual HRESULT PumpSampleNum(DWORD dwSampleNum);
    virtual HRESULT ProcessSample(IMFSample* pSample, DWORD dwSampleNum, bool fIsKey);
    
private:
    CSampleOutputWindow* m_pOutputWindow;
    CSampleOutputWindow* m_pPreviewWindow;
    CSampleProvider* m_pSampleProvider;
    CVideoTransformApplier* m_pTransformApplier;
    CAudioTransformApplier* m_pAudioTransformApplier;
    CAtlString m_strFileName;
    WORD m_wLastPos;
    CComPtr<IMFSample> m_spCurrentDisplaySample;
    bool m_fIsCurrentFrameKey;
    DWORD m_dwCurrentSampleNum;
};

class CMfvePlaybackState
    : public CMfveState
{
public:
    CMfvePlaybackState(CMfveState* pOldState, CTimeBarControl* pTimeBar, CSampleOutputWindow* pOutputWindow, CMediaEventHandler* pEventHandler);
    ~CMfvePlaybackState();
    
    virtual HRESULT HandleSeek(WORD wPos, WORD wPosMax, bool fEnd);
    virtual HRESULT HandleTimer();
    virtual HRESULT Activate();
    virtual HRESULT Init(LPCWSTR szSourceURL, CTransformApplier* pTransformApplier = NULL, CTransformApplier* pAudioTransformApplier = NULL);
    
    virtual bool CanStop() { return true; }
    
private:
    CSampleOutputWindow* m_pOutputWindow;
    CPlaybackHandler* m_pPlaybackHandler;
    CMediaEventHandler* m_pEventHandler;
};

class CMfveTranscodeState
    : public CMfveState
{
public:
    CMfveTranscodeState(CMfveState* pOldState, CTimeBarControl* pTimeBar, CMediaTranscodeEventHandler* pEventHandler, int iEncodeQuality);
    ~CMfveTranscodeState();
    
    virtual HRESULT HandleSeek(WORD wPos, WORD wPosMax, bool fEnd);
    virtual HRESULT HandleTimer();
    virtual HRESULT Activate();
    virtual HRESULT Init(LPCWSTR szSourceURL, LPCWSTR szOutputURL, CVideoTransformApplier* pTransformApplier, CAudioTransformApplier* pAudioTransformApplier, UINT32 unFrameRateN, UINT32 unFrameRateD);
    
    virtual bool CanStop() { return true; }
    
private:
    CMediaTranscoder* m_pTranscoder;
    CMediaTranscodeEventHandler* m_pEventHandler;
    int m_iEncodeQuality;
};