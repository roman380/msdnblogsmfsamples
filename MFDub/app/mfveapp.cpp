// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE
//
// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "stdafx.h"

#include "resource.h"
#include "mfapi.h"
#include "mferror.h"
#include "commdlg.h"
#include "mfveapp.h"
#include "timebar.h"
#include "sampleoutputwindow.h"
#include "transformapplier.h"
#include "dialogs.h"
#include "transformdisplay.h"
#include "mfveutil.h"
#include "topologybuilder.h"
#include "mfvestate.h"

#include <initguid.h>
#include "mfveapi.h"

#include <assert.h>

DEFINE_GUID(IID_IMFTConfiguration, 0x3A2B9766,0x2835,0x4520,0xAA,0x66,0x39,0xF0,0x77,0xC2,0x21,0x73);

const UINT_PTR CMfveApp::ms_nTimerID = 0;
const DWORD CMfveApp::ms_dwTimerLen = 200;

///////////////////////////////////////////////////////////////////////////////
//
HINSTANCE g_hInst = NULL;
CMfveApp * g_pApp;

inline LONG RectWidth(const RECT& rect)
{
    return rect.right - rect.left;
}
inline LONG RectHeight(const RECT& rect)
{
    return rect.bottom - rect.top;
}

///////////////////////////////////////////////////////////////////////////////
//
CMfveApp::CMfveApp()
    : m_pTimeBarControl(NULL)
    , m_pSampleOutputWindow(NULL)
    , m_pPreviewOutputWindow(NULL)
    , m_pTransformApplier(NULL)
    , m_pAudioTransformApplier(NULL)
    , m_pMediaEventHandler(NULL)
    , m_pTransformDisplay(NULL)
    , m_pAudioTransformDisplay(NULL)
    , m_pSampleWindowEventHandler(NULL)
    , m_pCurrentState(NULL)
    , m_pTransformDisplayListener(NULL)
    , m_iEncodeQuality(300000)
{
    CoInitialize( NULL );
	
    if(FAILED(MFStartup( MF_VERSION )))
    {
        assert(false);
    }
}

CMfveApp::~CMfveApp()
{
    delete m_pTimeBarControl;
    delete m_pSampleOutputWindow;
    delete m_pPreviewOutputWindow;
    delete m_pTransformApplier;
    delete m_pAudioTransformApplier;
    delete m_pMediaEventHandler;
    delete m_pTransformDisplay;
    delete m_pAudioTransformDisplay;
    delete m_pSampleWindowEventHandler;
    delete m_pTransformDisplayListener;
    
    while(m_pCurrentState->GetOldState() != NULL)
    {
        CMfveState* pOldState = m_pCurrentState->GetOldState();
        delete m_pCurrentState;
        m_pCurrentState = pOldState;
    }
    
    delete m_pCurrentState;

    MFShutdown();
    CoUninitialize();
}

// Init
// Create main window
HRESULT CMfveApp::Init(LPCWSTR lpCmdLine)
{
    HRESULT hr = S_OK;
    CAtlStringW szTitle;
            
    // set window name
    if(!szTitle.LoadString(IDS_APP_TITLE))
    {
        hr = E_FAIL;
        goto done;
    }

    m_hMenu = NULL;
    if(Create(NULL, NULL, (LPCTSTR) szTitle.GetBuffer(), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, 0, m_hMenu, NULL) == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

done:
    if(FAILED(hr)) 
    {
        CAtlStringW strError;
        (void)strError.LoadString(IDS_ERROR_INITIALIZING);
        HandleError(strError, hr);
    }

    return hr;
}

// HandleSeekerScroll
// Notification that the seek bar position has changed.  Delegate to the current state
void CMfveApp::HandleSeekerScroll(WORD wPos, bool fEnd)
{
    HRESULT hr = S_OK;

    if(m_pCurrentState)
    {
        hr = m_pCurrentState->HandleSeek(wPos, static_cast<WORD>(m_pTimeBarControl->GetMaxPos()), fEnd);
    }
    
    if(FAILED(hr))
    {
        CAtlStringW strError;
        (void)strError.LoadString(IDS_ERROR_SEEKING);
        HandleError(strError, hr);
    }
}

// CheckReposition
// Before a window reposition completes, modify the new location so that the window stays
// in its proper 'stuck' position.
void CMfveApp::CheckReposition(LPVOID pvInitiatorWindow, WINDOWPOS* pWindowPos)
{
    if(NULL == m_pSampleOutputWindow || NULL == m_pPreviewOutputWindow || NULL == m_pTransformDisplay || NULL == m_pAudioTransformDisplay)
    {
        // Not fully initialized; wait.
        return;
    }

    RECT rectToolbar;
    m_MainToolbar.GetWindowRect(&rectToolbar);

    if(pvInitiatorWindow == m_pSampleOutputWindow)
    {
        // The original output window should stick on the top left corner
        pWindowPos->x = m_kMargin;
        pWindowPos->y = RectHeight(rectToolbar) + m_kMargin;
    }
    else if(pvInitiatorWindow == m_pPreviewOutputWindow)
    {
        // The preview output window should stick to the right of the sample output window.
        RECT rectOutputWindow;
        m_pSampleOutputWindow->GetWindowRect(&rectOutputWindow);

        pWindowPos->x = m_kMargin + RectWidth(rectOutputWindow) + m_kMargin;
        pWindowPos->y = RectHeight(rectToolbar) + m_kMargin;
    }
}

// RepositionWindows
// After a window reposition completes, move other windows where their position depends on the initiator window.
void CMfveApp::RepositionWindows(LPVOID pvInitiatorWindow, const WINDOWPOS* pWindowPos)
{
    if(NULL == m_pSampleOutputWindow || NULL == m_pPreviewOutputWindow || NULL == m_pTransformDisplay || NULL == m_pAudioTransformDisplay)
    {
        // Not fully initialized; wait.
        return;
    }

    RECT rectToolbar;
    m_MainToolbar.GetWindowRect(&rectToolbar);
    
    if(pvInitiatorWindow == m_pSampleOutputWindow)
    {
        // If the sample output window has resized, the preview window and transform windows need to move.
        m_pPreviewOutputWindow->SetWindowPos(HWND_TOP, pWindowPos->cx + m_kMargin, RectHeight(rectToolbar) + m_kMargin, 0, 0, SWP_NOSIZE);

        RECT rectPreviewWindow;
        m_pPreviewOutputWindow->GetWindowRect(&rectPreviewWindow);
        LONG lMaxWindowHeight = pWindowPos->cy > RectHeight(rectPreviewWindow) ? pWindowPos->cy : RectHeight(rectPreviewWindow);

        m_pTransformDisplay->SetWindowPos(HWND_TOP, m_kMargin, RectHeight(rectToolbar) + m_kMargin + lMaxWindowHeight + m_kMargin, 0, 0, SWP_NOSIZE);

        RECT rectTransformDisplay;
        m_pTransformDisplay->GetWindowRect(&rectTransformDisplay);
        m_pAudioTransformDisplay->SetWindowPos(HWND_TOP, m_kMargin + m_kMargin + RectWidth(rectTransformDisplay), RectHeight(rectToolbar) + m_kMargin + lMaxWindowHeight + m_kMargin, 0, 0, SWP_NOSIZE);
    }
    else if(pvInitiatorWindow == m_pPreviewOutputWindow)
    {
        // If the preview window has resized, the transform windows need to move.
        RECT rectOutputWindow;
        m_pSampleOutputWindow->GetWindowRect(&rectOutputWindow);

        LONG lMaxWindowHeight = pWindowPos->cy > RectHeight(rectOutputWindow) ? pWindowPos->cy : RectHeight(rectOutputWindow);

        m_pTransformDisplay->SetWindowPos(HWND_TOP, m_kMargin, RectHeight(rectToolbar) + m_kMargin + lMaxWindowHeight + m_kMargin, 0, 0, SWP_NOSIZE);

        RECT rectTransformDisplay;
        m_pTransformDisplay->GetWindowRect(&rectTransformDisplay);
        m_pAudioTransformDisplay->SetWindowPos(HWND_TOP, m_kMargin + m_kMargin + RectWidth(rectTransformDisplay), RectHeight(rectToolbar) + m_kMargin + lMaxWindowHeight + m_kMargin, 0, 0, SWP_NOSIZE);
    }
}

// NotifyStateFinished
// Notification that the current state has completed execution; for example, a
// transcode completes. Clean up the current state and pop the previous state 
// off of the chain.
void CMfveApp::NotifyStateFinished()
{
    CMfveState* pOldState = m_pCurrentState->GetOldState();
    delete m_pCurrentState;
    m_pCurrentState = pOldState;
    
    m_pCurrentState->Activate();
}

// NotifyStateError
// Notification that the current state resulted in an error.  Clean up the current
// state, pop the previous state off the chain, and notify the user of the error.
void CMfveApp::NotifyStateError(HRESULT hrError)
{
    CMfveState* pOldState = m_pCurrentState->GetOldState();
    delete m_pCurrentState;
    m_pCurrentState = pOldState;
    
    CAtlStringW strError;
    (void)strError.LoadString(IDS_ERROR_GENERIC);
    HandleError(strError, hrError);
    
    m_pCurrentState->Activate();
}

// TransformRefresh
// Update state information that may be invalidated when the transform chain changes.
HRESULT CMfveApp::TransformRefresh()
{
    HRESULT hr;
    CComPtr<IMFSample> spSample;
    CComPtr<IMFSample> spNewSample;
    
    UINT32 unWidth = m_pTransformApplier->GetOutputWidth();
    UINT32 unHeight = m_pTransformApplier->GetOutputHeight();
    
    CHECK_HR( hr = m_pPreviewOutputWindow->SetSampleSize(unWidth, unHeight, FALSE) );
    
    // The preview window's video frame needs to be updated as well.
    CHECK_HR( hr = m_pSampleOutputWindow->GetCurrentSample(&spSample) );
    CHECK_HR( hr = m_pTransformApplier->ProcessSample(spSample, &spNewSample) );
    CHECK_HR( hr = m_pPreviewOutputWindow->SetOutputSample(spNewSample) );   
    
    m_pTransformDisplay->Invalidate();

done:
    return hr;
}

// OnCreate
// Create all child windows.
LRESULT CMfveApp::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HRESULT hr = S_OK;
    CAtlStringW strVideoTransforms;
    CAtlStringW strAudioTransforms;

    RECT rectClient;
    GetClientRect(&rectClient);
    RECT rectToolbar;
    RECT rectTransportToolbar;
    RECT rectNewWindow;
    LONG lMainWindowHeight;

    m_pSampleWindowEventHandler = new CMfveSampleWindowEventHandler(this);
    if(NULL == m_pSampleWindowEventHandler)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    
    m_MainToolbar.Create(m_hWnd, NULL, L"Toolbar", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0U, NULL);
    m_MainToolbar.GetWindowRect(&rectToolbar);

    m_TransportToolbar.Create(m_hWnd, NULL, L"Transport Controls Toolbar", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | CCS_BOTTOM, 0, 1U, NULL);
    m_TransportToolbar.GetWindowRect(&rectTransportToolbar);
    
    rectNewWindow = rectClient;
    rectNewWindow.top = rectNewWindow.bottom  - RectHeight(rectTransportToolbar) - m_kdyTimeBarControl;
    rectNewWindow.bottom = rectNewWindow.top + m_kdyTimeBarControl;
    m_pTimeBarControl = new CTimeBarControl();
    if(NULL == m_pTimeBarControl)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    CHECK_HR( hr = m_pTimeBarControl->Init(m_hWnd, rectNewWindow, true, true) );
    m_pTimeBarControl->SetScrollCallback(&HandleSeekerScrollFunc);

    lMainWindowHeight = RectHeight(rectToolbar) - m_kMargin + rectNewWindow.top;
    if(lMainWindowHeight < 0) lMainWindowHeight = 0;

    rectNewWindow = rectClient;
    rectNewWindow.left += m_kMargin;
    rectNewWindow.top += RectHeight(rectToolbar) + m_kMargin;
    rectNewWindow.right = rectNewWindow.left + m_kdySampleWindow;
    rectNewWindow.bottom = lMainWindowHeight > m_kdySampleWindow ? rectNewWindow.top + m_kdySampleWindow : lMainWindowHeight;
    m_pSampleOutputWindow = new CSampleOutputWindow();
    if(NULL == m_pSampleOutputWindow)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    m_pSampleOutputWindow->Create(m_hWnd, rectNewWindow, L"Sample Output Window", WS_CHILD | WS_VISIBLE | WS_THICKFRAME, 0, 0U, NULL);
    m_pSampleOutputWindow->SetEventListener(m_pSampleWindowEventHandler);

    rectNewWindow.left += m_kdxSampleWindow + m_kMargin;
    rectNewWindow.right += m_kdxSampleWindow + m_kMargin;
    m_pPreviewOutputWindow = new CSampleOutputWindow();
    if(NULL == m_pPreviewOutputWindow)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    m_pPreviewOutputWindow->Create(m_hWnd, rectNewWindow, L"Preview Output Window", WS_CHILD | WS_VISIBLE | WS_THICKFRAME, 0, 0U, NULL);
    m_pPreviewOutputWindow->SetEventListener(m_pSampleWindowEventHandler);

    lMainWindowHeight -= rectNewWindow.bottom;
    if(lMainWindowHeight < 0) lMainWindowHeight = 0;

    m_pTransformApplier = new CVideoTransformApplier();
    if(NULL == m_pTransformApplier)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    
    m_pAudioTransformApplier = new CAudioTransformApplier();
    if(NULL == m_pAudioTransformApplier)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
   
    (void)strVideoTransforms.LoadString(IDS_VIDEO_TRANSFORMS);

    rectNewWindow.left = m_kMargin;
    rectNewWindow.right = rectNewWindow.left + m_kdxTransformDisplay;
    rectNewWindow.top = rectNewWindow.bottom + m_kMargin + m_kMargin;
    rectNewWindow.bottom += lMainWindowHeight > m_kdyTransformDisplay ? m_kdyTransformDisplay : lMainWindowHeight;
    m_pTransformDisplay = new CTransformDisplay(m_pTransformApplier);
    if(NULL == m_pTransformDisplay)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    m_pTransformDisplay->Create(m_hWnd, rectNewWindow, strVideoTransforms.GetBuffer(), WS_CHILD | WS_VISIBLE, 0, 0U, NULL);
    
    (void)strAudioTransforms.LoadString(IDS_AUDIO_TRANSFORMS);

    rectNewWindow.left = rectNewWindow.right + m_kMargin + m_kMargin;
    rectNewWindow.right = rectNewWindow.left + m_kdxTransformDisplay;
    m_pAudioTransformDisplay = new CTransformDisplay(m_pAudioTransformApplier);
    if(NULL == m_pAudioTransformDisplay)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    m_pAudioTransformDisplay->Create(m_hWnd, rectNewWindow, strAudioTransforms.GetBuffer(), WS_CHILD | WS_VISIBLE, 0, 0U, NULL);
    
    m_pMediaEventHandler = new CMfveMediaEventHandler(this);
    if(NULL == m_pMediaEventHandler)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    
    m_pTransformDisplayListener = new CMfveTransformDisplayEventListener(this);
    if(NULL == m_pTransformDisplayListener)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    m_pTransformDisplay->SetEventListener(m_pTransformDisplayListener);
    
    CMfveState::SetMainToolbar(&m_MainToolbar);
    CMfveState::SetTransportToolbar(&m_TransportToolbar);
    
    m_pCurrentState = new CMfveClosedState(NULL, m_pTimeBarControl);
    if(NULL == m_pCurrentState)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    m_pCurrentState->Activate();
    
    SetTimer(ms_nTimerID, ms_dwTimerLen, NULL);
    
done:
    if(FAILED(hr))
    {
        CAtlStringW strError;
        (void)strError.LoadString(IDS_ERROR_WINDOW_CREATION);
        HandleError(strError, hr);
        return (LRESULT) hr;
    }

    return 0;
}

LRESULT CMfveApp::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    PostQuitMessage(0);
    return 0;
}

// OnSize
// Resize windows to fit the new client area.
LRESULT CMfveApp::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if(wParam == SIZE_MINIMIZED)
    {
        return 0;
    }

    RECT rectClient;
    GetClientRect(&rectClient);
    
    m_MainToolbar.SetWindowPos(HWND_TOP, 0, 0, rectClient.right, 20, SWP_NOMOVE);
    m_TransportToolbar.SetWindowPos(HWND_TOP, 0, rectClient.bottom - 20, rectClient.right, 20, SWP_NOMOVE);
    
    RECT rectMainToolbar;
    m_MainToolbar.GetWindowRect(&rectMainToolbar);
    LONG dyMainToolbar = RectHeight(rectMainToolbar);

    RECT rectTransportToolbar;
    m_TransportToolbar.GetWindowRect(&rectTransportToolbar);
    LONG dyTransportToolbar = RectHeight(rectTransportToolbar);

    m_pTimeBarControl->SetWindowPos(HWND_TOP, 0, rectClient.bottom - dyTransportToolbar - 
        m_kdyTimeBarControl, rectClient.right, m_kdyTimeBarControl, 0);

    LONG lMainWindowHeight = RectHeight(rectClient) - dyMainToolbar - dyTransportToolbar - m_kdyTimeBarControl;
    if(lMainWindowHeight < 0) lMainWindowHeight = 0;

    // Sample window and preview window may need to decrease in size if the main window client area
    // is too small to accommodate them.
    RECT rectSampleWindow;
    m_pSampleOutputWindow->GetWindowRect(&rectSampleWindow);
    LONG dwSampleWidth = RectWidth(rectSampleWindow);
    LONG dwSampleHeight = RectHeight(rectSampleWindow);
    m_pSampleOutputWindow->SetWindowPos(HWND_TOP, 0, 0, dwSampleWidth, lMainWindowHeight > dwSampleHeight ? dwSampleHeight : lMainWindowHeight, SWP_NOMOVE);

    RECT rectPreviewWindow;
    m_pPreviewOutputWindow->GetWindowRect(&rectPreviewWindow);
    LONG dwPreviewWidth = RectWidth(rectPreviewWindow);
    LONG dwPreviewHeight = RectHeight(rectPreviewWindow);
    m_pPreviewOutputWindow->SetWindowPos(HWND_TOP, 0, 0, dwPreviewWidth, lMainWindowHeight > dwPreviewHeight ? dwPreviewHeight : lMainWindowHeight, SWP_NOMOVE);

    if(dwPreviewHeight > dwSampleHeight)
    {
        lMainWindowHeight -= dwPreviewHeight;
    }
    else
    {
        lMainWindowHeight -= dwSampleHeight;
    }
    if(lMainWindowHeight < 0) lMainWindowHeight = 0;

    m_pTransformDisplay->SetWindowPos(HWND_TOP, 0, 0, m_kdxTransformDisplay, lMainWindowHeight > m_kdyTransformDisplay ? m_kdyTransformDisplay : lMainWindowHeight, SWP_NOMOVE);
    m_pAudioTransformDisplay->SetWindowPos(HWND_TOP, 0, 0, m_kdxTransformDisplay, lMainWindowHeight > m_kdyTransformDisplay ? m_kdyTransformDisplay : lMainWindowHeight, SWP_NOMOVE);

    return 0;
}

// OnTimer
// Notify the current state of a timer tick.
LRESULT CMfveApp::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if(wParam == ms_nTimerID)
    {
        if(m_pCurrentState)
        {
            m_pCurrentState->HandleTimer();
        }
    }

    SetTimer(ms_nTimerID, ms_dwTimerLen, NULL);
    return 0;
}

LRESULT CMfveApp::OnMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_pTransformDisplay->SendMessage(WM_MOVE, 0, 0);
    
    bHandled = false;
    return 0;
}

// OnOpen
// Present the file open dialog and load the file for scrubbing.
LRESULT CMfveApp::OnOpen(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT hr = S_OK;
    CMfveScrubState* pNewState = NULL;

    CAtlString strFilter;
    (void)strFilter.LoadString(IDS_FILTER_MEDIA_FILES);
    strFilter.SetAt(strFilter.GetLength()-1, 0);
    strFilter.SetAt(strFilter.GetLength()-2, 0);

    CAtlString strTitle;
    (void)strTitle.LoadString(IDS_SELECT_SOURCE);

    TCHAR fileBuffer[1024];
    fileBuffer[0] = 0;

    OPENFILENAME openFileInfo;
    openFileInfo.lStructSize = sizeof(OPENFILENAME);
    openFileInfo.hwndOwner = m_hWnd;
    openFileInfo.hInstance = 0;
    openFileInfo.lpstrFilter = strFilter;
    openFileInfo.lpstrCustomFilter = NULL;
    openFileInfo.nFilterIndex = 1;
    openFileInfo.lpstrFile = fileBuffer;
    openFileInfo.nMaxFile = 1024;
    openFileInfo.lpstrFileTitle = NULL;
    openFileInfo.nMaxFileTitle = 0;
    openFileInfo.lpstrInitialDir = NULL;
    openFileInfo.lpstrTitle = strTitle;
    openFileInfo.Flags = OFN_FILEMUSTEXIST;
    openFileInfo.nFileOffset = 0;
    openFileInfo.nFileExtension = 0;
    openFileInfo.lpstrDefExt = NULL;
    openFileInfo.lCustData = NULL;
    openFileInfo.pvReserved = NULL;
    openFileInfo.dwReserved = 0;
    openFileInfo.FlagsEx = 0;

    if(GetOpenFileName(&openFileInfo))
    {
        m_strSourceURL = openFileInfo.lpstrFile;
        
        ReturnToInitialState();
        m_pTransformDisplay->Reset();
        
        pNewState = new CMfveScrubState(m_pCurrentState, m_pTimeBarControl, m_pSampleOutputWindow, m_pPreviewOutputWindow, m_pTransformApplier, m_pAudioTransformApplier);
        if(NULL == pNewState)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        CHECK_HR( hr = pNewState->Init(openFileInfo.lpstrFile, &m_unFrameRateN, &m_unFrameRateD) );
        CHECK_HR( hr = pNewState->Activate() );
        
        m_pCurrentState = pNewState;
    }

done:
    if(FAILED(hr))
    {
        CAtlString strError;
        (void)strError.LoadString(IDS_ERROR_LOAD_SOURCE);

        delete pNewState;
        HandleError(strError, hr);
    }

    return 0;
}

// OnSave
// Present the file save dialog and start a transcode session to the specified output file.
LRESULT CMfveApp::OnSave(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT hr = S_OK;
    CMfveTranscodeState* pNewState = NULL;
    
    CAtlString strFilter;
    (void)strFilter.LoadString(IDS_FILTER_MEDIA_FILES);
    strFilter.SetAt(strFilter.GetLength()-1, 0);
    strFilter.SetAt(strFilter.GetLength()-2, 0);

    CAtlString strTitle;
    (void)strTitle.LoadString(IDS_SELECT_SINK);

    TCHAR fileBuffer[1024];
    fileBuffer[0] = 0;

    OPENFILENAME openFileInfo;
    openFileInfo.lStructSize = sizeof(OPENFILENAME);
    openFileInfo.hwndOwner = m_hWnd;
    openFileInfo.hInstance = 0;
    openFileInfo.lpstrFilter = strFilter;
    openFileInfo.lpstrCustomFilter = NULL;
    openFileInfo.nFilterIndex = 1;
    openFileInfo.lpstrFile = fileBuffer;
    openFileInfo.nMaxFile = 1024;
    openFileInfo.lpstrFileTitle = NULL;
    openFileInfo.nMaxFileTitle = 0;
    openFileInfo.lpstrInitialDir = NULL;
    openFileInfo.lpstrTitle = strTitle;
    openFileInfo.Flags = 0;
    openFileInfo.nFileOffset = 0;
    openFileInfo.nFileExtension = 0;
    openFileInfo.lpstrDefExt = NULL;
    openFileInfo.lCustData = NULL;
    openFileInfo.pvReserved = NULL;
    openFileInfo.dwReserved = 0;
    openFileInfo.FlagsEx = 0;

    if(GetSaveFileName(&openFileInfo)) 
    {
        pNewState = new CMfveTranscodeState(m_pCurrentState, m_pTimeBarControl, m_pMediaEventHandler, m_iEncodeQuality);
        if(NULL == pNewState)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        CHECK_HR( hr = pNewState->Init(m_strSourceURL, openFileInfo.lpstrFile, m_pTransformApplier, m_pAudioTransformApplier, m_unFrameRateN, m_unFrameRateD) );
        CHECK_HR( hr = pNewState->Activate() );
        m_pCurrentState = pNewState;
    }
    
done:
    if(FAILED(hr))
    {
        CAtlString strError;
        (void)strError.LoadString(IDS_ERROR_WRITE_OUTPUT);

        delete pNewState;
        HandleError(strError, hr);
    }
    
    return 0;
}

// OnAddTransform
// Present the transform selection dialog for video and add the chosen transform to the transform chain.
LRESULT CMfveApp::OnAddTransform(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if(m_pTransformApplier->GetTransformCount() >= m_kcMaxTransforms)
    {
        CAtlString strErrorTitle;
        (void)strErrorTitle.LoadString(IDS_ERROR_GENERIC);

        CAtlString strReason;
        (void)strReason.LoadString(IDS_ERROR_ADD_TRANSFORM_MAX);
        MessageBox(strReason, strErrorTitle, MB_OK);

        return 0;
    }

    HRESULT hr = S_OK;
    CAddTransformDialog dialog(MFVE_CATEGORY_VIDEO);
    
    if(dialog.DoModal() == IDOK)
    {
        CComPtr<IMFTransform> spTransform;
        CComPtr<IMFSample> spSample;
        
        CHECK_HR( hr = CoCreateInstance(dialog.GetChosenCLSID(), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&spTransform)) );
                
        CHECK_HR( hr = m_pPreviewOutputWindow->GetCurrentSample(&spSample) );
        
        CHECK_HR( hr = m_pTransformApplier->AddTransform(spTransform, dialog.GetChosenName(), dialog.GetChosenCLSID(), m_hWnd, spSample) );
        
        CHECK_HR( hr = TransformRefresh() );
    }
    
done:
    if(FAILED(hr) && hr != E_ABORT)
    {
        CAtlString strError;
        (void)strError.LoadString(IDS_ERROR_ADD_TRANSFORM);
        HandleError(strError, hr);
    }
    
    return 0;
}

// OnAddTransform
// Present the transform selection dialog for audio and add the chosen transform to the transform chain
LRESULT CMfveApp::OnAddAudioTransform(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if(m_pAudioTransformApplier->GetTransformCount() >= m_kcMaxTransforms)
    {
        CAtlString strErrorTitle;
        (void)strErrorTitle.LoadString(IDS_ERROR_GENERIC);

        CAtlString strReason;
        (void)strReason.LoadString(IDS_ERROR_ADD_TRANSFORM_MAX);
        MessageBox(strReason, strErrorTitle, MB_OK);

        return 0;
    }

    HRESULT hr = S_OK;
    CAddTransformDialog dialog(MFVE_CATEGORY_AUDIO);
    
    if(dialog.DoModal() == IDOK)
    {
        CComPtr<IMFTransform> spTransform;
        CComPtr<IMFSample> spSample;
        
        CHECK_HR( hr = CoCreateInstance(dialog.GetChosenCLSID(), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&spTransform)) );
                
        CHECK_HR( hr = m_pAudioTransformApplier->AddTransform(spTransform, dialog.GetChosenName(), dialog.GetChosenCLSID(), m_hWnd, NULL) );

        m_pAudioTransformDisplay->Invalidate();
    }

done:
    if(FAILED(hr) && hr != E_ABORT)
    {
        CAtlString strError;
        (void)strError.LoadString(IDS_ERROR_ADD_TRANSFORM);
        HandleError(strError, hr);
    }
    
    return 0;
}

// OnPlayOutput
// Start playback of the original media file.
LRESULT CMfveApp::OnPlayOutput(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT hr;
        
    CMfvePlaybackState* pNewState = new CMfvePlaybackState(m_pCurrentState, m_pTimeBarControl, m_pSampleOutputWindow, m_pMediaEventHandler);
    if(NULL == pNewState)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    CHECK_HR( hr = pNewState->Init(m_strSourceURL.GetString()) );
    CHECK_HR( hr = pNewState->Activate() );
    
    m_pCurrentState = pNewState;
        
done:
    if(FAILED(hr))
    {
        delete pNewState;

        CAtlString strError;
        (void)strError.LoadString(IDS_ERROR_PLAYBACK);
        HandleError(strError, hr);
    }
    
    return 0;
}

// OnPlayPreview
// Start playback of a preview of what the output file will look like.
LRESULT CMfveApp::OnPlayPreview(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT hr;
        
    CMfvePlaybackState* pNewState = new CMfvePlaybackState(m_pCurrentState, m_pTimeBarControl, m_pPreviewOutputWindow, m_pMediaEventHandler);
    if(NULL == pNewState)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    CHECK_HR( hr = pNewState->Init(m_strSourceURL.GetString(), m_pTransformApplier, m_pAudioTransformApplier) );
    CHECK_HR( hr = pNewState->Activate() );
    
    m_pCurrentState = pNewState;
    
done:
    if(FAILED(hr))
    {
        delete pNewState;

        CAtlString strError;
        (void)strError.LoadString(IDS_ERROR_PLAYBACK);
        HandleError(strError, hr);
    }
    
    return 0;
}

// OnStop
// Force the current state to end if possible.
LRESULT CMfveApp::OnStop(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if(m_pCurrentState->CanStop())
    {
        NotifyStateFinished();
    }
    
    return 0;
}

// OnEncodeOptions
// Present the encoding options dialog and update the encode options state.
LRESULT CMfveApp::OnEncodeOptions(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    CEncodeOptionsDialog dialog(m_iEncodeQuality, double(m_unFrameRateN) / double(m_unFrameRateD));
    
    if(dialog.DoModal() == IDOK)
    {
        m_iEncodeQuality = dialog.GetChosenValue();
        
        if(dialog.IsFrameRateChanged())
        {
            double dbFrameRate = dialog.GetFrameRate();
            MFAverageTimePerFrameToFrameRate(UINT64(10000000.0 / dbFrameRate), &m_unFrameRateN, &m_unFrameRateD);
        }
    }
    
    return 0;
}

// OnNextKeyframe
// Handle a next keyframe skip -- delegate to the current state.
LRESULT CMfveApp::OnNextKeyframe(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    m_pCurrentState->HandleNextKeyframe();
    
    return 0;
}

// OnPrevKeyframe
// Handle a previous keyframe skip -- delegate to the currrent state.
LRESULT CMfveApp::OnPrevKeyframe(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    m_pCurrentState->HandlePrevKeyframe();
    
    return 0;
}

// OnProperties
// Display metadata for the current state.
LRESULT CMfveApp::OnProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    m_pCurrentState->HandleShowMetadata();
    
    return 0;
}

// OnGoBeginning
// Handle a skip to the beginning -- delegate to the current state.
LRESULT CMfveApp::OnGoBeginning(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    m_pCurrentState->HandleSeek(0, static_cast<WORD>(m_pTimeBarControl->GetMaxPos()), true);
    m_pTimeBarControl->SetPos(0);
    
    return 0;
}

// OnGoEnd
// Handle a skip to the end -- delegate to the current state.
LRESULT CMfveApp::OnGoEnd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    m_pCurrentState->HandleSeek(static_cast<WORD>(m_pTimeBarControl->GetMaxPos()), static_cast<WORD>(m_pTimeBarControl->GetMaxPos()), true);
    m_pTimeBarControl->SetPos(m_pTimeBarControl->GetMaxPos());
    return 0;
}

// HandleError
// Error handling for errors not sourced from a UI state.  Just display an
// error dialog.
void CMfveApp::HandleError(CAtlString strError, HRESULT hr)
{
    CAtlString strErrorTitle;
    (void)strErrorTitle.LoadString(IDS_ERROR_GENERIC);

    CAtlString strFormat;
    strFormat.Format(IDS_REASON, strErrorTitle, hr);
    MessageBox(strFormat, strError, MB_OK);
}

// ReturnToInitialState
// Pop all states off of the state chain and return to the original state.
void CMfveApp::ReturnToInitialState()
{
    while(m_pCurrentState->GetOldState() != NULL)
    {
        CMfveState* pOldState = m_pCurrentState->GetOldState();
        delete m_pCurrentState;
        m_pCurrentState = pOldState;
    }
    
    m_pCurrentState->Activate();
}

// InitMfveApp
// Create and initialize the application object
HRESULT InitMfveApp(LPCWSTR lpCmdLine, int nCmdShow)
{
    HRESULT hr = S_OK;

    g_pApp = new CMfveApp;
    if(g_pApp == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    CHECK_HR(g_pApp->Init(lpCmdLine));

    /* Make the window visible; update its client area; and return "success" */
    g_pApp->ShowWindow(nCmdShow);
    g_pApp->UpdateWindow();

done:

    return hr;
}

#include "atlfile.h"

int PASCAL wWinMain( __in HINSTANCE hInstance,
                     __in_opt HINSTANCE hPrevInstance,
                     __in LPWSTR lpCmdLine,
                     __in int nCmdShow)
{
    HRESULT hr = S_OK;
    MSG  msg;
    HACCEL hAccelTable = NULL;
    INITCOMMONCONTROLSEX  iccex;

    (void)HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

    g_hInst = hInstance;

    //required to use the common controls
    InitCommonControls();

    // Initialize the INITCOMMONCONTROLSEX structure.
    iccex.dwSize = sizeof (INITCOMMONCONTROLSEX);
    iccex.dwICC = ICC_LISTVIEW_CLASSES;

    // Register tree-view control classes from the DLL for the common 
    // control.
    InitCommonControlsEx (&iccex);

    /* Perform initializations that apply to a specific instance */
    CHECK_HR(InitMfveApp(lpCmdLine, nCmdShow));

    /* Acquire and dispatch messages until a WM_QUIT uMessage is received. */
    while(GetMessage( &msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(g_pApp->m_hWnd, hAccelTable, &msg)) 
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    delete g_pApp;
    
    DestroyAcceleratorTable(hAccelTable);
    
    return (int)msg.wParam;

done:
    assert(SUCCEEDED(hr));
    return FALSE;
}

void HandleSeekerScrollFunc(WORD wPos, bool fEnd) 
{
    g_pApp->HandleSeekerScroll(wPos, fEnd);
}

////////////////////////////////////

CMfveMediaEventHandler::CMfveMediaEventHandler(CMfveApp* pApp)
    : m_pApp(pApp)
{
}

// HandleFinished
// Called by a state when the state completes.  Delegate to the application object.
void CMfveMediaEventHandler::HandleFinished()
{
    m_pApp->NotifyStateFinished();
}

// HandleError
// Called by a state when the state encounters an error condition.  Delegate to the
// application object.
void CMfveMediaEventHandler::HandleError(HRESULT hr)
{
    m_pApp->NotifyStateError(hr);
}

/////////////////////////////////////

CMfveSampleWindowEventHandler::CMfveSampleWindowEventHandler(CMfveApp* pApp)
    : m_pApp(pApp)
{
}

// CheckResize
// Called by a video window before it compeletes resizing.  Delegate to the
// application object.
void CMfveSampleWindowEventHandler::CheckResize(LPVOID pvWindow, WINDOWPOS* pWindowPos)
{
    m_pApp->CheckReposition(pvWindow, pWindowPos);
}

// HandleResize
// Called by a video window after it completes resizing.  Delegate to the
// application object.
void CMfveSampleWindowEventHandler::HandleResize(LPVOID pvWindow, const WINDOWPOS* pWindowPos)
{
    m_pApp->RepositionWindows(pvWindow, pWindowPos);
}

///////////////////////////////////////

CMfveTransformDisplayEventListener::CMfveTransformDisplayEventListener(CMfveApp* pApp)
    : m_pApp(pApp)
{
}

// HandleTransformRemoved
// Called by a transform window when a transform is removed.  Delegate to
// the application object.
void CMfveTransformDisplayEventListener::HandleTransformRemoved()
{
    m_pApp->TransformRefresh();
}

// HandleTransformMoved
// Called by a transform windwo when a transform changes position.  Delegate
// to the application object.
void CMfveTransformDisplayEventListener::HandleTransformMoved()
{
    m_pApp->TransformRefresh();
}