// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE
//
// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "stdafx.h"

#include "mfvestate.h"
#include "sampleoutputwindow.h"
#include "sampleprovider.h"
#include "mediatranscoder.h"
#include "playbackhandler.h"
#include "timebar.h"
#include "transformapplier.h"
#include "mferror.h"
#include "maintoolbar.h"
#include "transporttoolbar.h"
#include "topologybuilder.h"
#include "dialogs.h"
#include "resource.h"

#define HNS_TO_HOURS 36000000000
#define HNS_TO_MINUTES 600000000
#define HNS_TO_SECONDS 10000000
#define HNS_TO_MILLISECONDS 10000

void MFTIMEtoParts(MFTIME hnsTime, DWORD* pdwHours, DWORD* pdwMinutes, DWORD* pdwSeconds, DWORD* pdwMilliseconds)
{
    *pdwHours = static_cast<DWORD>(hnsTime / HNS_TO_HOURS);
    hnsTime = hnsTime % HNS_TO_HOURS;
    *pdwMinutes = static_cast<DWORD>(hnsTime / HNS_TO_MINUTES);
    hnsTime = hnsTime % HNS_TO_MINUTES;
    *pdwSeconds = static_cast<DWORD>(hnsTime / HNS_TO_SECONDS);
    hnsTime = hnsTime % HNS_TO_SECONDS;
    *pdwMilliseconds = static_cast<DWORD>(hnsTime / HNS_TO_MILLISECONDS);
}

CMainToolbar* CMfveState::ms_pMainToolbar = NULL;
CTransportToolbar* CMfveState::ms_pTransportToolbar = NULL;

CMfveState::CMfveState(CMfveState* pOldState, CTimeBarControl* pTimeBar)
    : m_pOldState(pOldState)
    , m_pTimeBar(pTimeBar)
{
}

CMfveState::~CMfveState()
{
}

//////////////////////////////

CMfveClosedState::CMfveClosedState(CMfveState* pOldState, CTimeBarControl* pTimeBar)
    : CMfveState(pOldState, pTimeBar)
{
}

// HandleSeek
// A seek is meaningless when there is no open file.
HRESULT CMfveClosedState::HandleSeek(WORD wPos, WORD wPosMax, bool fEnd)
{
    return S_OK;
}

// HandleTimer
// Timers are ignored when there is no open file.
HRESULT CMfveClosedState::HandleTimer()
{
    return S_OK;
}

// Activate
// Set the state of UI elements for this state.
HRESULT CMfveClosedState::Activate()
{
    PMainToolbar()->EnableButtonByCommand(ID_OPEN, TRUE);
    PMainToolbar()->EnableButtonByCommand(ID_SAVE, FALSE);
    PMainToolbar()->EnableButtonByCommand(ID_ADDTRANSFORM, FALSE);
    PMainToolbar()->EnableButtonByCommand(ID_ADDAUDIOTRANSFORM, FALSE);
    PMainToolbar()->EnableButtonByCommand(ID_PROPERTIES, FALSE);
    
    PTransportToolbar()->EnableButtonByCommand(ID_PLAYOUTPUT, FALSE);
    PTransportToolbar()->EnableButtonByCommand(ID_PLAYPREVIEW, FALSE);
    PTransportToolbar()->EnableButtonByCommand(ID_STOP, FALSE);
    PTransportToolbar()->EnableButtonByCommand(ID_GO_BEGINNING, FALSE);
    PTransportToolbar()->EnableButtonByCommand(ID_GO_END, FALSE);
    PTransportToolbar()->EnableButtonByCommand(ID_NEXTKEYFRAME, FALSE);
    PTransportToolbar()->EnableButtonByCommand(ID_PREVKEYFRAME, FALSE);
    
    return S_OK;
}

//////////////////////////////

CMfveScrubState::CMfveScrubState(CMfveState* pOldState, CTimeBarControl* pTimeBar, CSampleOutputWindow* pOutputWindow, 
    CSampleOutputWindow* pPreviewWindow, CVideoTransformApplier* pTransformApplier, CAudioTransformApplier* pAudioTransformApplier)
    : CMfveState(pOldState, pTimeBar)
    , m_pOutputWindow(pOutputWindow)
    , m_pPreviewWindow(pPreviewWindow)
    , m_pSampleProvider(NULL)
    , m_pTransformApplier(pTransformApplier)
    , m_pAudioTransformApplier(pAudioTransformApplier)
    , m_wLastPos(0)
{
}

CMfveScrubState::~CMfveScrubState()
{
    delete m_pSampleProvider;
}

// HandleSeek
// Scrub to the sample specified by wPos.
HRESULT CMfveScrubState::HandleSeek(WORD wPos, WORD wPosMax, bool fEnd)
{
    HRESULT hr = S_OK;
    MFTIME hnsDuration;

    if(wPos > wPosMax) wPos = wPosMax;
    if(wPos == m_wLastPos) return S_OK;
    m_wLastPos = wPos;
    
    CHECK_HR( hr = m_pSampleProvider->GetMediaDuration(&hnsDuration) );

    MFTIME hnsSeekPos = MFllMulDiv(wPos, hnsDuration, 1000, 0);
    CHECK_HR( hr = PumpSample(hnsSeekPos) );

done:
    return hr;
}

// HandleTimer
// Timer ticks are not used in the scrub state.
HRESULT CMfveScrubState::HandleTimer()
{
    return S_OK;
}

// HandleNextKeyframe
// Jump the scrub position to the next keyframe.
HRESULT CMfveScrubState::HandleNextKeyframe()
{
    return E_NOTIMPL;
}

// HandlePrevKeyframe
// Jump the scrub position to the previous keyframe.
HRESULT CMfveScrubState::HandlePrevKeyframe()
{
    return E_NOTIMPL;
}

// HandleShowMetadata
// Obtain metadata from the sample provider and display the metadata dialog.
HRESULT CMfveScrubState::HandleShowMetadata()
{
    CComPtr<IMFSourceReader> spSourceReader = m_pSampleProvider->GetSourceReader();
    CComPtr<IMFMediaType> spAudioType = m_pSampleProvider->GetAudioMediaType();
    CComPtr<IMFMediaType> spVideoType = m_pSampleProvider->GetVideoMediaType();
    DWORD dwFrameCount = 0;
    DWORD dwKeyframeCount = 0;
    
    CMetadataDialog dialog(m_strFileName.GetString(), spSourceReader, spAudioType, spVideoType, dwFrameCount, dwKeyframeCount);
    dialog.DoModal();
    
    return S_OK;
}

// Activate
// Set the state of UI elements for this state.
HRESULT CMfveScrubState::Activate()
{
    HRESULT hr;
    CAtlString strSampleTimeText;
    UINT nFormatID;

    PMainToolbar()->EnableButtonByCommand(ID_OPEN, TRUE);
    PMainToolbar()->EnableButtonByCommand(ID_SAVE, TRUE);
    PMainToolbar()->EnableButtonByCommand(ID_ADDTRANSFORM, TRUE);
    PMainToolbar()->EnableButtonByCommand(ID_ADDAUDIOTRANSFORM, TRUE);
    PMainToolbar()->EnableButtonByCommand(ID_PROPERTIES, TRUE);
    
    PTransportToolbar()->EnableButtonByCommand(ID_PLAYOUTPUT, TRUE);
    PTransportToolbar()->EnableButtonByCommand(ID_PLAYPREVIEW, TRUE);
    PTransportToolbar()->EnableButtonByCommand(ID_STOP, FALSE);
    PTransportToolbar()->EnableButtonByCommand(ID_GO_BEGINNING, TRUE);
    PTransportToolbar()->EnableButtonByCommand(ID_GO_END, TRUE);
    PTransportToolbar()->EnableButtonByCommand(ID_NEXTKEYFRAME, FALSE);
    PTransportToolbar()->EnableButtonByCommand(ID_PREVKEYFRAME, FALSE);
    
    m_pOutputWindow->Invalidate();
    m_pPreviewWindow->Invalidate();
    
    PTimeBar()->SetRange(0, 1000);
    PTimeBar()->SetPos(m_wLastPos);
    
    MFTIME hnsSample;
    CHECK_HR( hr = m_spCurrentDisplaySample->GetSampleTime(&hnsSample) );
    
    DWORD dwHours, dwMinutes, dwSeconds, dwMilliseconds;
    MFTIMEtoParts(hnsSample, &dwHours, &dwMinutes, &dwSeconds, &dwMilliseconds);
    
    if(m_fIsCurrentFrameKey)
    {
        nFormatID = IDS_FRAME_INFO_KEY;
    }
    else
    {
        nFormatID = IDS_FRAME_INFO;
    }
    strSampleTimeText.Format(nFormatID, m_dwCurrentSampleNum, dwHours, dwMinutes, dwSeconds, dwMilliseconds);

    PTransportToolbar()->SetSampleTimeLabelText(strSampleTimeText.GetString());
done:
    return S_OK;
}

// Init
// Create sample provide and initialize display windows and transform chains.
HRESULT CMfveScrubState::Init(LPCWSTR szFileName, UINT32* punFrameRateN, UINT32* punFrameRateD)
{
    HRESULT hr = S_OK;
    CComPtr<IMFSample> spSample;
    CComPtr<IMFSample> spPreviewSample;
    CComPtr<IUnknown> spTransformUnk;
    CComPtr<IMFTransform> spTransform;
    CComPtr<IMFMediaType> spType;

    m_pTransformApplier->Reset();
    m_pOutputWindow->Reset();
    m_pPreviewWindow->Reset();
    
    m_strFileName = szFileName;
    
    m_pSampleProvider = new CSampleProvider();
    if(NULL == m_pSampleProvider)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    CHECK_HR( hr = m_pSampleProvider->LoadSource(szFileName) );

    UINT32 unWidth, unHeight;
    CHECK_HR( hr = m_pSampleProvider->GetSampleSize(&unWidth, &unHeight) );
    
    CHECK_HR( hr = m_pOutputWindow->SetSampleSize(unWidth, unHeight) );
    CHECK_HR( hr = m_pPreviewWindow->SetSampleSize(unWidth, unHeight) );
    m_pTransformApplier->SetInputType(m_pSampleProvider->GetVideoMediaType());
    m_pAudioTransformApplier->SetInputType(m_pSampleProvider->GetAudioMediaType());

    CHECK_HR( hr = PumpSample(0) );
    
    spType = m_pSampleProvider->GetVideoMediaType();
    CHECK_HR( hr = MFGetAttributeRatio(spType, MF_MT_FRAME_RATE, punFrameRateN, punFrameRateD) );
    
done:
    return hr;
}

// PumpSample(hnsSeekTime)
// Obtain a video frame from the sample provider based upon a seek
// time and process the sample.
HRESULT CMfveScrubState::PumpSample(MFTIME hnsSeekTime)
{
    HRESULT hr;
    CComPtr<IMFSample> spOldSample;
    
    if(m_spCurrentDisplaySample.p) 
    {
        spOldSample.Attach(m_spCurrentDisplaySample.Detach());
    }
    
    DWORD dwSampleNum;
    bool fIsKey;
    CHECK_HR( hr = m_pSampleProvider->GetVideoSample(hnsSeekTime, &m_spCurrentDisplaySample, &dwSampleNum, &fIsKey) );
    
    CHECK_HR( hr = ProcessSample(m_spCurrentDisplaySample, dwSampleNum, fIsKey) );
    
done:
    if(hr == MF_E_END_OF_STREAM)
    {
        m_spCurrentDisplaySample = spOldSample;
        hr = S_OK;
    }
    
    return hr;
}

// PumpSample(dwSampleNum)
// Obtain a video frame from the sample provider based upon a sample
// number and process the sample.
HRESULT CMfveScrubState::PumpSampleNum(DWORD dwSampleNum)
{
    HRESULT hr;
    CComPtr<IMFSample> spOldSample;
    
    if(m_spCurrentDisplaySample.p) 
    {
        spOldSample.Attach(m_spCurrentDisplaySample.Detach());
    }
    
    DWORD dwTemp;
    bool fIsKey;
    CHECK_HR( hr = m_pSampleProvider->GetVideoSample(dwSampleNum, &m_spCurrentDisplaySample, &dwTemp, &fIsKey) );
    
    CHECK_HR( hr = ProcessSample(m_spCurrentDisplaySample, dwSampleNum, fIsKey) );
    
done:
    if(hr == MF_E_END_OF_STREAM)
    {
        m_spCurrentDisplaySample = spOldSample;
        hr = S_OK;
    }
    
    return hr;
}

// ProcessSample
// Generate the sample label text, process the sample through the transform chain,
// and then output the sample and preview sample to the respective output windows.
HRESULT CMfveScrubState::ProcessSample(IMFSample* pSample, DWORD dwSampleNum, bool fIsKey)
{
    HRESULT hr;
    CComPtr<IMFSample> spPreviewSample;
    CAtlString strSampleTimeText;
    UINT nFormatID;

    m_dwCurrentSampleNum = dwSampleNum;
    m_fIsCurrentFrameKey = fIsKey;
    
    MFTIME hnsSample;
    CHECK_HR( hr = m_spCurrentDisplaySample->GetSampleTime(&hnsSample) );
    
    DWORD dwHours, dwMinutes, dwSeconds, dwMilliseconds;
    MFTIMEtoParts(hnsSample, &dwHours, &dwMinutes, &dwSeconds, &dwMilliseconds);
    
    if(fIsKey)
    {
        nFormatID = IDS_FRAME_INFO_KEY;
    }
    else
    {
        nFormatID = IDS_FRAME_INFO;
    }
    strSampleTimeText.Format(nFormatID, dwSampleNum, dwHours, dwMinutes, dwSeconds, dwMilliseconds);
    
    PTransportToolbar()->SetSampleTimeLabelText(strSampleTimeText.GetString());
    
    CHECK_HR( hr = m_pTransformApplier->ProcessSample(pSample, &spPreviewSample) );

    CHECK_HR( hr = m_pOutputWindow->SetOutputSample(pSample) );
    CHECK_HR( hr = m_pPreviewWindow->SetOutputSample(spPreviewSample) );
    
done:
    return hr;
}
////////////////////////////////////////////////

CMfvePlaybackState::CMfvePlaybackState(CMfveState* pOldState, CTimeBarControl* pTimeBar, CSampleOutputWindow* pOutputWindow, CMediaEventHandler* pEventHandler)
    : CMfveState(pOldState, pTimeBar)
    , m_pOutputWindow(pOutputWindow)
    , m_pPlaybackHandler(NULL)
    , m_pEventHandler(pEventHandler)
{
}

CMfvePlaybackState::~CMfvePlaybackState()
{
   m_pPlaybackHandler->Shutdown();
   m_pPlaybackHandler->Release();
}

// HandleSeek
// Calculate a seek time and seek to the new playback position.
HRESULT CMfvePlaybackState::HandleSeek(WORD wPos, WORD wPosMax, bool fEnd)
{
    // Only seek if this is the end of a position change
    if(fEnd)
    {
        MFTIME hnsDuration;
        HRESULT hr = m_pPlaybackHandler->GetPresentationDuration(&hnsDuration);
    
        if(SUCCEEDED(hr))
        {
            MFTIME hnsSeekTime = wPos * hnsDuration / wPosMax;
            m_pPlaybackHandler->Start(hnsSeekTime);
        }
    }
    
    return S_OK;
}

// HandleTimer
// Update time display each time tick.
HRESULT CMfvePlaybackState::HandleTimer()
{
    CAtlString strSampleTimeText;
    MFTIME hnsTime, hnsDuration;
    HRESULT hr = m_pPlaybackHandler->GetPresentationTime(&hnsTime);
    HRESULT hr2 = m_pPlaybackHandler->GetPresentationDuration(&hnsDuration);

    if(SUCCEEDED(hr) && SUCCEEDED(hr2))
    {
        PTimeBar()->SetPosByTime(hnsTime, hnsDuration);
        
        DWORD dwHours, dwMinutes, dwSeconds, dwMilliseconds;
        MFTIMEtoParts(hnsTime, &dwHours, &dwMinutes, &dwSeconds, &dwMilliseconds);
        
        DWORD dwDurHours, dwDurMinutes, dwDurSeconds, dwDurMilliseconds;
        MFTIMEtoParts(hnsDuration, &dwDurHours, &dwDurMinutes, &dwDurSeconds, &dwDurMilliseconds);
        
        strSampleTimeText.Format(IDS_PLAYBACK_INFO, dwHours, dwMinutes, dwSeconds, dwMilliseconds, dwDurHours, dwDurMinutes, dwDurSeconds, dwDurMilliseconds);
        PTransportToolbar()->SetSampleTimeLabelText(strSampleTimeText.GetString());
    }
    
    return S_OK;
}

// Activate
// Set the state of UI elements for this state.
HRESULT CMfvePlaybackState::Activate()
{
    PMainToolbar()->EnableButtonByCommand(ID_OPEN, TRUE);
    PMainToolbar()->EnableButtonByCommand(ID_SAVE, FALSE);
    PMainToolbar()->EnableButtonByCommand(ID_ADDTRANSFORM, FALSE);
    PMainToolbar()->EnableButtonByCommand(ID_ADDAUDIOTRANSFORM, FALSE);
    
    PTransportToolbar()->EnableButtonByCommand(ID_PLAYOUTPUT, FALSE);
    PTransportToolbar()->EnableButtonByCommand(ID_PLAYPREVIEW, FALSE);
    PTransportToolbar()->EnableButtonByCommand(ID_STOP, TRUE);
    PTransportToolbar()->EnableButtonByCommand(ID_GO_BEGINNING, FALSE);
    PTransportToolbar()->EnableButtonByCommand(ID_GO_END, FALSE);
    PTransportToolbar()->EnableButtonByCommand(ID_NEXTKEYFRAME, FALSE);
    PTransportToolbar()->EnableButtonByCommand(ID_PREVKEYFRAME, FALSE);
    
    return S_OK;
}

// Init
// Build a playback topology and begin playback.
// The playback handler uses the MF pipeline directly for playback, having the EVR
// render directly to the video output window.  It would also be possible to do this
// using the source reader and some manual processing, or using the MFPlay API.
HRESULT CMfvePlaybackState::Init(LPCWSTR szSourceURL, CTransformApplier* pTransformApplier, CTransformApplier* pAudioTransformApplier)
{
    HRESULT hr;

    m_pPlaybackHandler = new CPlaybackHandler();
    m_pPlaybackHandler->AddRef();
    
    CTopologyBuilder* pTopologyBuilder = new CTopologyBuilder();
    CHECK_HR( hr = pTopologyBuilder->LoadSource(szSourceURL) );
    
    // Clone transforms from the transform chain before adding them to the
    // topology builder.  The MF pipeline may use different media types
    // than the transform chain does, and this is simpler than renegotiating
    // the media types after the transition back to the scrub state.
    if(pTransformApplier)
    {
        for(size_t i = 0; i < pTransformApplier->GetTransformCount(); i++)
        {
            CComPtr<IMFTransform> spTransform;
            
            CHECK_HR( hr = pTransformApplier->CloneTransform(i, &spTransform) );
            CHECK_HR( hr = pTopologyBuilder->AddVideoTransform(spTransform) );
        }
    }

    if(pAudioTransformApplier)
    {
        for(size_t i = 0; i < pAudioTransformApplier->GetTransformCount(); i++)
        {
            CComPtr<IMFTransform> spTransform;

            CHECK_HR( hr = pAudioTransformApplier->CloneTransform(i, &spTransform) );
            CHECK_HR( hr = pTopologyBuilder->AddAudioTransform(spTransform) );
        }
    }
    
    m_pPlaybackHandler->SetEventHandler(m_pEventHandler);
    CHECK_HR( hr = m_pPlaybackHandler->InitSession(pTopologyBuilder, m_pOutputWindow->m_hWnd) );
    
    MFTIME hnsDuration;
    LONG lPos = static_cast<LONG>(PTimeBar()->SendMessage(TBM_GETPOS, 0, 0));
    LONG lMax = static_cast<LONG>(PTimeBar()->SendMessage(TBM_GETRANGEMAX, 0, 0));
    CHECK_HR( hr = m_pPlaybackHandler->GetPresentationDuration(&hnsDuration) );
    CHECK_HR( hr = m_pPlaybackHandler->Start(lPos * hnsDuration / lMax) );
    
done:
    return hr;
}

/////////////////////////////////////

CMfveTranscodeState::CMfveTranscodeState(CMfveState* pOldState, CTimeBarControl* pTimeBar, CMediaTranscodeEventHandler* pEventHandler, int iEncodeQuality)
    : CMfveState(pOldState, pTimeBar)
    , m_pEventHandler(pEventHandler)
    , m_iEncodeQuality(iEncodeQuality)
{
}

CMfveTranscodeState::~CMfveTranscodeState()
{
    m_pTranscoder->Release();
}

// HandleSeek
// Seeking during a transcode is not supported.  This would have
// limited benefit.
HRESULT CMfveTranscodeState::HandleSeek(WORD wPos, WORD wPosMax, bool fEnd)
{
    return S_OK;
}

// HandleTimer
// Each timer tick, update the transcode progress label.
HRESULT CMfveTranscodeState::HandleTimer()
{
    MFTIME hnsTime, hnsDuration;
    HRESULT hr = m_pTranscoder->GetPresentationTime(&hnsTime);
    HRESULT hr2 = m_pTranscoder->GetPresentationDuration(&hnsDuration);

    if(SUCCEEDED(hr) && SUCCEEDED(hr2))
    {
        PTimeBar()->SetPosByTime(hnsTime, hnsDuration);
        
        CAtlString strSampleTimeText;
        DWORD dwPercentFinished = static_cast<DWORD>(hnsTime * 100 / hnsDuration);
        strSampleTimeText.Format(IDS_ENCODING_INFO, dwPercentFinished);
        PTransportToolbar()->SetSampleTimeLabelText(strSampleTimeText.GetString());
    }
    
    return S_OK;
}

// Activate
// Set the state of UI elements for this state.
HRESULT CMfveTranscodeState::Activate()
{
    PMainToolbar()->EnableButtonByCommand(ID_OPEN, FALSE);
    PMainToolbar()->EnableButtonByCommand(ID_SAVE, FALSE);
    PMainToolbar()->EnableButtonByCommand(ID_ADDTRANSFORM, FALSE);
    PMainToolbar()->EnableButtonByCommand(ID_ADDAUDIOTRANSFORM, FALSE);
    
    PTransportToolbar()->EnableButtonByCommand(ID_PLAYOUTPUT, FALSE);
    PTransportToolbar()->EnableButtonByCommand(ID_PLAYPREVIEW, FALSE);
    PTransportToolbar()->EnableButtonByCommand(ID_STOP, TRUE);
    PTransportToolbar()->EnableButtonByCommand(ID_GO_BEGINNING, FALSE);
    PTransportToolbar()->EnableButtonByCommand(ID_GO_END, FALSE);
    PTransportToolbar()->EnableButtonByCommand(ID_NEXTKEYFRAME, FALSE);
    PTransportToolbar()->EnableButtonByCommand(ID_PREVKEYFRAME, FALSE);
    
    return S_OK;
}

// Init
// Create a transcoder and begin transcode.
HRESULT CMfveTranscodeState::Init(LPCWSTR szSourceURL, LPCWSTR szOutputURL, CVideoTransformApplier* pTransformApplier,
    CAudioTransformApplier* pAudioTransformApplier, UINT32 unFrameRateN, UINT32 unFrameRateD)
{
    HRESULT hr = S_OK;

    m_pTranscoder = new CMediaTranscoder(szSourceURL, pTransformApplier, pAudioTransformApplier, unFrameRateN, unFrameRateD, szOutputURL, m_iEncodeQuality, m_pEventHandler, hr);
    if(NULL == m_pTranscoder)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    CHECK_HR( hr = hr );
    
    CHECK_HR( hr = m_pTranscoder->BeginTranscode() );
    
done:
    return hr;
}
