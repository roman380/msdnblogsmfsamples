#ifndef __MFVEAPP__
#define __MFVEAPP__

#include "resource.h"
#include "maintoolbar.h"
#include "transporttoolbar.h"
#include "mediatranscoder.h"
#include "playbackhandler.h"
#include "sampleoutputwindow.h"
#include "transformdisplay.h"

class CTimeBarControl;
class CTransformApplier;
class CMfveMediaEventHandler;
class CTransformDisplay;
class CMfveSampleWindowEventHandler;
class CMfveState;
class CMfveTransformDisplayEventListener;

#define WM_USER_CHILDSIZE (WM_USER + 1)
///////////////////////////////////////////////////////////////////////////////
// 

class CMfveApp 
    : public CWindowImpl<CMfveApp>
{
public:
    CMfveApp();
    ~CMfveApp();

    HRESULT Init(LPCWSTR lpCmdLine);

    void HandleSeekerScroll(WORD wPos, bool fEnd);
    void CheckReposition(LPVOID pvInitiatorWindow, WINDOWPOS* pWindowPos);
    void RepositionWindows(LPVOID pvInitiatorWindow, const WINDOWPOS* pWindowPos);

    void NotifyStateFinished();
    void NotifyStateError(HRESULT hrError);
    HRESULT TransformRefresh();
    
protected:    
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
    LRESULT OnOpen(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnSave(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnAddTransform(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnAddAudioTransform(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnPlayOutput(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnPlayPreview(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnStop(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnEncodeOptions(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnNextKeyframe(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnPrevKeyframe(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnGoBeginning(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnGoEnd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    BEGIN_MSG_MAP(CMfveApp)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_MOVE, OnMove)

        COMMAND_HANDLER(ID_OPEN, 0, OnOpen)
        COMMAND_HANDLER(ID_SAVE, 0, OnSave)
        COMMAND_HANDLER(ID_ADDTRANSFORM, 0, OnAddTransform)
        COMMAND_HANDLER(ID_ADDAUDIOTRANSFORM, 0, OnAddAudioTransform)
        COMMAND_HANDLER(ID_PLAYOUTPUT, 0, OnPlayOutput)
        COMMAND_HANDLER(ID_PLAYPREVIEW, 0, OnPlayPreview)
        COMMAND_HANDLER(ID_STOP, 0, OnStop)
        COMMAND_HANDLER(ID_ENCODEOPTIONS, 0, OnEncodeOptions)
        COMMAND_HANDLER(ID_NEXTKEYFRAME, 0, OnNextKeyframe)
        COMMAND_HANDLER(ID_PREVKEYFRAME, 0, OnPrevKeyframe)
        COMMAND_HANDLER(ID_PROPERTIES, 0, OnProperties)
        COMMAND_HANDLER(ID_GO_BEGINNING, 0, OnGoBeginning)
        COMMAND_HANDLER(ID_GO_END, 0, OnGoEnd)
    END_MSG_MAP()

    void HandleError(CAtlString strError, HRESULT hr);
    void ReturnToInitialState();
     
private:
    HMENU m_hMenu;
    CTimeBarControl* m_pTimeBarControl;
    CMainToolbar m_MainToolbar;
    CTransportToolbar m_TransportToolbar;
    CSampleOutputWindow* m_pSampleOutputWindow;
    CSampleOutputWindow* m_pPreviewOutputWindow;
    CVideoTransformApplier* m_pTransformApplier;
    CAudioTransformApplier* m_pAudioTransformApplier;
    CMfveMediaEventHandler* m_pMediaEventHandler;
    CTransformDisplay* m_pTransformDisplay;
    CTransformDisplay* m_pAudioTransformDisplay;
    CAtlString m_strSourceURL;
    CMfveSampleWindowEventHandler* m_pSampleWindowEventHandler;
    CMfveState* m_pCurrentState;
    CMfveTransformDisplayEventListener* m_pTransformDisplayListener;
    int m_iEncodeQuality;
    UINT32 m_unFrameRateN;
    UINT32 m_unFrameRateD;
    
    static const LONG m_kMargin = 5;
    static const LONG m_kdyTimeBarControl = 30;
    static const LONG m_kdxSampleWindow = 320;
    static const LONG m_kdySampleWindow = 240;
    static const LONG m_kdxTransformDisplay = 200;
    static const LONG m_kdyTransformDisplay = 250;
    static const size_t m_kcMaxTransforms = 11;

    static const UINT_PTR ms_nTimerID;
    static const DWORD ms_dwTimerLen;
};

void HandleSeekerScrollFunc(WORD wPos, bool fEnd);

class CMfveMediaEventHandler
    : public CMediaTranscodeEventHandler
    , public CMediaEventHandler
{
public:
    CMfveMediaEventHandler(CMfveApp* pApp);
    
    void HandleFinished();
    void HandleError(HRESULT hr);

private:
    CMfveApp* m_pApp;
};

class CMfveSampleWindowEventHandler
    : public CWindowEventListener
{
public:
    CMfveSampleWindowEventHandler(CMfveApp* pApp);
    
    void CheckResize(LPVOID pvWindow, WINDOWPOS* pWindowPos);
    void HandleResize(LPVOID pvWindow, const WINDOWPOS* pWindowPos);

    
private:
    CMfveApp* m_pApp;
};

class CMfveTransformDisplayEventListener
    : public CTransformDisplayEventListener
{
public:
    CMfveTransformDisplayEventListener(CMfveApp* pApp);
    
    void HandleTransformRemoved();
    void HandleTransformMoved();
    
private:
    CMfveApp* m_pApp;
};

#endif

