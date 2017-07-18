// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE
//
// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "stdafx.h"

#include "rgbmft.h"
#include "mferror.h"
#include "rgbutils.h"
#include "rgbtransform.h"
#include "rgbtypehandler.h"
#include "wmcodecdsp.h"
#include "mfveapi.h"

DEFINE_GUID(CLSID_CResizerDMO, 0x1EA1EA14, 0x48F4, 0x4054, 0xAD, 0x1A, 0xE8, 0xAE, 0xE1, 0x0A, 0xC8, 0x05);

CRgbMFT::CRgbMFT(CRGBImageTransform* pTransform)
    : m_pImageTransform(pTransform)
{
    Init(new CRGB1in1outAutoCopyTypeHandler());
}

CRgbMFT::~CRgbMFT()
{
    delete PTypeHandler();
}

HRESULT CRgbMFT::GetStreamIDs(DWORD dwInputIDArraySize, DWORD *pdwInputIDs, DWORD dwOutputIDArraySize, DWORD *pdwOutputIDs)
{
    if(dwInputIDArraySize < 1 || dwOutputIDArraySize < 1)
    {
        return E_INVALIDARG;
    }

    if(NULL == pdwInputIDs || NULL == pdwOutputIDs)
    {
        return E_POINTER;
    }

    *pdwInputIDs = 0;
    *pdwOutputIDs = 0;

    return S_OK;
}

HRESULT CRgbMFT::GetInputStatus(DWORD dwInputStreamID, DWORD *pdwFlags)
{
    if( dwInputStreamID > 1 )
    {
        return( MF_E_INVALIDSTREAMNUMBER );
    }

    if( NULL == pdwFlags )
    {
        return( E_INVALIDARG );
    }

    *pdwFlags = MFT_INPUT_STATUS_ACCEPT_DATA;

    return( S_OK );
}

HRESULT CRgbMFT::GetOutputStatus(DWORD *pdwFlags)
{
    return E_NOTIMPL;
}

HRESULT CRgbMFT::ProcessMessage(MFT_MESSAGE_TYPE eMessage, ULONG_PTR ulParam)
{
    switch(eMessage)
    {
        case MFT_MESSAGE_COMMAND_FLUSH:
            m_spCurrentSample.Release();
            break;
    }
    
    return S_OK;
}

HRESULT CRgbMFT::ProcessInput(DWORD dwInputStreamID, IMFSample* pSample, DWORD dwFlags)
{
    if(dwInputStreamID > 0)
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    if(NULL != m_spCurrentSample.p)
    {
        return MF_E_NOTACCEPTING;
    }

    if(NULL == pSample)
    {
        return E_POINTER;
    }

    m_spCurrentSample = pSample;

    return S_OK;
}

HRESULT CRgbMFT::ProcessOutput(DWORD dwFlags, DWORD cOutputBufferCount, MFT_OUTPUT_DATA_BUFFER* pOutputSamples, DWORD *pdwStatus)
{
    HRESULT hr = S_OK;

    if(NULL == m_spCurrentSample.p)
    {
        return MF_E_TRANSFORM_NEED_MORE_INPUT;
    }

    if(NULL != pOutputSamples->pSample)
    {
        CComPtr<IMFMediaType> spType;

        LONGLONG hnsTime;
        hr = m_spCurrentSample->GetSampleTime(&hnsTime);
        if (SUCCEEDED(hr)) 
        {
            pOutputSamples->pSample->SetSampleTime(hnsTime);
        }

        LONGLONG hnsDuration;
        hr = m_spCurrentSample->GetSampleDuration(&hnsDuration);
        if (SUCCEEDED(hr)) 
        {
            pOutputSamples->pSample->SetSampleDuration(hnsDuration);
        }
        hr = S_OK;

        CComPtr<IMFMediaBuffer> spInBuffer;
        BYTE* pbInBuffer;
        DWORD cbInLength;
        CComPtr<IMFMediaBuffer> spOutBuffer;
        BYTE* pbOutBuffer;
        DWORD cbOutLength;
            
        CHECK_HR( hr = m_spCurrentSample->GetBufferByIndex(0, &spInBuffer) );
        spInBuffer->Lock(&pbInBuffer, &cbInLength, NULL);
           
        CHECK_HR( hr = pOutputSamples->pSample->GetBufferByIndex(0, &spOutBuffer) );
        spOutBuffer->Lock(&pbOutBuffer, &cbOutLength, NULL);

        DWORD dwMinLength = (cbOutLength > cbInLength) ? cbInLength : cbOutLength;
        spOutBuffer->SetCurrentLength(dwMinLength);
    
        PTypeHandler()->GetInputCurrentType(0, &spType);

        UINT32 unWidth, unHeight;
        MFGetAttributeSize(spType, MF_MT_FRAME_SIZE, &unWidth, &unHeight);
    
        m_pImageTransform->Transform(unWidth, unHeight, (RGBQUAD*) pbInBuffer, (RGBQUAD*) pbOutBuffer);
        
        spInBuffer->Unlock();
        spOutBuffer->Unlock();
        
        m_spCurrentSample.Release();
    }

done:
    return hr;
}

/////////////////////////////////

CHistogramEqualizationMFT::CHistogramEqualizationMFT()
    : CRgbMFT(&m_HistEqualTransform)
{
}

CHistogramEqualizationMFT::~CHistogramEqualizationMFT()
{
}

/////////////////////////////////

CNoiseRemovalMFT::CNoiseRemovalMFT()
    : CRgbMFT(&m_NoiseRemovalTransform)
{
}

CNoiseRemovalMFT::~CNoiseRemovalMFT()
{
}

/////////////////////////////////

CUnsharpMaskMFT::CUnsharpMaskMFT()
    : m_UnsharpTransform(1, 1.5f)
    , CRgbMFT(&m_UnsharpTransform)
{
    MFCreateAttributes(&m_spAttributes, 1);
}

// protected constructor, used by Clone
CUnsharpMaskMFT::CUnsharpMaskMFT(float gamma)
    : m_UnsharpTransform(1, gamma)
    , CRgbMFT(&m_UnsharpTransform)
{
}

CUnsharpMaskMFT::~CUnsharpMaskMFT()
{
}

HRESULT CUnsharpMaskMFT::GetAttributes(IMFAttributes** ppAttributes)
{
    *ppAttributes = m_spAttributes;
    (*ppAttributes)->AddRef();
    
    return S_OK;
}

HRESULT CUnsharpMaskMFT::GetFriendlyName(__out LPWSTR* pszFriendlyName)
{
    CAtlString str(L"Unsharp Mask");
    *pszFriendlyName = (LPWSTR) CoTaskMemAlloc(sizeof(WCHAR) * (str.GetLength() + 1));
    wcscpy_s(*pszFriendlyName, str.GetLength(), str.GetString());
    return S_OK;
}

HRESULT CUnsharpMaskMFT::QueryRequiresConfiguration(__out BOOL* pfRequiresConfiguration)
{
    *pfRequiresConfiguration = TRUE;
    
    return S_OK;
}

HRESULT CUnsharpMaskMFT::Configure(LONG_PTR hWnd, __in IMFSample* pExampleSample, __in IMFMediaType* pSampleMediaType)
{
    HRESULT hr;
    UINT32 unSampleWidth, unSampleHeight;

    hr = MFGetAttributeSize(pSampleMediaType, MF_MT_FRAME_SIZE, &unSampleWidth, &unSampleHeight);

    if(SUCCEEDED(hr))
    {
        CUnsharpMaskConfigurationDialog dialog(1.5f, pExampleSample, unSampleWidth, unSampleHeight);
    
        INT_PTR RetCode = dialog.DoModal((HWND) hWnd);
        if(RetCode == IDOK)
        {
            m_spAttributes->SetDouble(UNSHARP_PARAM_GAMMA, dialog.GetChosenGamma());
            m_UnsharpTransform.SetGamma(dialog.GetChosenGamma());
        }
        else
        {
            hr = E_ABORT;
        }
    }
    
    return hr;
}

HRESULT CUnsharpMaskMFT::CloneMFT(__out IMFTConfiguration** ppClonedTransform)
{
    HRESULT hr;
    
    CComObject<CUnsharpMaskMFT>* pMFT = NULL;
    CHECK_HR( hr = CComObject<CUnsharpMaskMFT>::CreateInstance(&pMFT) );
    CHECK_HR( hr = pMFT->QueryInterface(IID_IMFTConfiguration, (void**) ppClonedTransform) );
    CUnsharpMaskMFT* pUMMFT = (CUnsharpMaskMFT*) *ppClonedTransform;
    m_spAttributes->CopyAllItems(pUMMFT->m_spAttributes);
    
done:
    return hr;
}

HRESULT CUnsharpMaskMFT::GetParamCount(__out DWORD* pcParams)
{
    if(NULL == pcParams) return E_POINTER;
    
    *pcParams = 1;
    
    return S_OK;
}

HRESULT CUnsharpMaskMFT::GetParam(DWORD dwIndex, __deref_out LPWSTR* pszName, __out MFVE_PARAM_TYPE* pParamType, __out GUID* pgidKey)
{
    if(dwIndex > 0) return E_INVALIDARG;
    if(NULL == pParamType) return E_POINTER;
    if(NULL == pgidKey) return E_POINTER;
    
    *pParamType = MFVE_PARAM_DOUBLE;
    *pgidKey = UNSHARP_PARAM_GAMMA;
    
    return S_OK;
}

HRESULT CUnsharpMaskMFT::GetParamConstraint(DWORD dwIndex, __out PROPVARIANT* pvarMin, __out PROPVARIANT* pvarMax, __out PROPVARIANT* pvarStep)
{
    if(dwIndex > 0) return E_INVALIDARG;
    if(NULL == pvarMin) return E_POINTER;
    if(NULL == pvarMax) return E_POINTER;
    if(NULL == pvarStep) return E_POINTER;
    
    (*pvarMin).vt = VT_R8;
    (*pvarMin).dblVal = 0.0;
    
    (*pvarMax).vt = VT_R8;
    (*pvarMax).dblVal = 3.0;
    
    (*pvarStep).vt = VT_R8;
    (*pvarStep).dblVal = 0.1;
    
    return S_OK;
}

HRESULT CUnsharpMaskMFT::GetParamValues(DWORD dwIndex, __out DWORD* pcValues, __out PROPVARIANT** ppvarValues)
{
    return E_NOTIMPL;
}

HRESULT CUnsharpMaskMFT::UpdateConfiguration()
{
    m_UnsharpTransform.SetGamma(static_cast<float>(MFGetAttributeDouble(m_spAttributes, UNSHARP_PARAM_GAMMA, 1.5)));
    
    return S_OK;
}

const float CUnsharpMaskConfigurationDialog::ms_MaxGamma = 3.0f;
const float CUnsharpMaskConfigurationDialog::ms_MinGamma = 0.0f;

CUnsharpMaskConfigurationDialog::CUnsharpMaskConfigurationDialog(float StartGamma, IMFSample* pExampleSample, UINT32 unSampleWidth, UINT32 unSampleHeight)
    : m_ChosenGamma(StartGamma)
    , m_pSampleOutputWindow(NULL)
    , m_spExampleSample(pExampleSample)
    , m_unSampleWidth(unSampleWidth)
    , m_unSampleHeight(unSampleHeight)
    , m_Transform(1, StartGamma)
{
}

CUnsharpMaskConfigurationDialog::~CUnsharpMaskConfigurationDialog()
{
    delete m_pSampleOutputWindow;
}

float CUnsharpMaskConfigurationDialog::GetChosenGamma()
{
    return m_ChosenGamma;
}

LRESULT CUnsharpMaskConfigurationDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_hTextBox = GetDlgItem(IDC_GAMMAEDIT);
    m_hSlider = GetDlgItem(IDC_GAMMASLIDER);
    
    CAtlStringW strTextBox;
    strTextBox.Format(L"%f", m_ChosenGamma);
    ::SetWindowText(m_hTextBox, strTextBox.GetString());
    
    ::SendMessage(m_hSlider, TBM_SETRANGE, (WPARAM) TRUE, (LPARAM) MAKELONG(0, 30));
    ::SendMessage(m_hSlider, TBM_SETPOS, (WPARAM) TRUE, 15);

    RECT rectClient;
    GetClientRect(&rectClient);
    
    RECT rectNewWindow;
    rectNewWindow.left = rectClient.left + 5;
    rectNewWindow.right = rectClient.right - 5;
    rectNewWindow.top = rectClient.top + 30;
    rectNewWindow.bottom = rectClient.bottom - 40;
    
    m_pSampleOutputWindow = new CSampleOutputWindow();
    m_pSampleOutputWindow->Create(m_hWnd, rectNewWindow, L"Sample Output Window", WS_CHILD | WS_VISIBLE, 0, 0U, NULL);
    m_pSampleOutputWindow->SetSampleSize(m_unSampleWidth, m_unSampleHeight);
    OutputExampleSample();
    
    return 0;
}

LRESULT CUnsharpMaskConfigurationDialog::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_ChosenGamma = GetTextBoxGamma();
    EndDialog(IDCANCEL);

    return 0;
}

LRESULT CUnsharpMaskConfigurationDialog::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT wPos = ::SendMessage(m_hSlider, TBM_GETPOS, 0, 0);
 
    m_ChosenGamma = wPos * .1f;
    
    CAtlStringW strTextBox;
    strTextBox.Format(L"%f", m_ChosenGamma);
    ::SetWindowText(m_hTextBox, strTextBox.GetString());
    
    m_Transform.SetGamma(m_ChosenGamma);
    OutputExampleSample();
    
    return 0;
}

LRESULT CUnsharpMaskConfigurationDialog::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    m_ChosenGamma = GetTextBoxGamma();
    EndDialog(IDOK);
    
    return 0;
}

LRESULT CUnsharpMaskConfigurationDialog::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(IDCANCEL);
    
    return 0;
}

float CUnsharpMaskConfigurationDialog::GetTextBoxGamma()
{
    int iLen = ::GetWindowTextLength(m_hTextBox);
    LPWSTR szTextBox = new WCHAR[iLen + 1];
    ::GetWindowText(m_hTextBox, szTextBox, iLen + 1);
    
    float gamma = static_cast<float>(_wtof(szTextBox));
    
    delete[] szTextBox;
    
    return gamma;
}

HRESULT CUnsharpMaskConfigurationDialog::OutputExampleSample()
{
    HRESULT hr = S_OK;
    CComPtr<IMFMediaBuffer> spBuffer;
    CComPtr<IMFSample> spOutSample;
    CComPtr<IMFMediaBuffer> spInBuffer;
    BYTE* pbInBuffer;
    DWORD cbInLength;
    BYTE* pbOutBuffer;
    DWORD cbOutLength;
    
    CHECK_HR( hr = MFCreateSample(&spOutSample) );
    CHECK_HR( hr = MFCreateMemoryBuffer(m_unSampleWidth * m_unSampleHeight * 4, &spBuffer) );
    CHECK_HR( hr = spOutSample->AddBuffer(spBuffer) );

    CHECK_HR( hr = m_spExampleSample->GetBufferByIndex(0, &spInBuffer) );
    spInBuffer->Lock(&pbInBuffer, &cbInLength, NULL);
    spBuffer->Lock(&pbOutBuffer, &cbOutLength, NULL);

    DWORD dwMinLength = (cbOutLength > cbInLength) ? cbInLength : cbOutLength;
    spBuffer->SetCurrentLength(dwMinLength);

    m_Transform.Transform(m_unSampleWidth, m_unSampleHeight, (RGBQUAD*) pbInBuffer, (RGBQUAD*) pbOutBuffer);
    
    spInBuffer->Unlock();
    spBuffer->Unlock();
    
    m_pSampleOutputWindow->SetOutputSample(spOutSample);
    
done:
    return hr;
}

////////////////////////////////////////////

CResizeCropMFT::CResizeCropMFT()
    : m_fConfigured(false)
{
    Init(new CResizeTypeHandler(this));
    MFCreateAttributes(&m_spAttributes, 3);
}

// protected constructor; used by CloneMFT
CResizeCropMFT::CResizeCropMFT(UINT32 unOutputWidth, UINT32 unOutputHeight)
    : m_fConfigured(true)
{
    Init(new CResizeTypeHandler(this));
    MFCreateAttributes(&m_spAttributes, 3);
}

CResizeCropMFT::~CResizeCropMFT()
{
    delete PTypeHandler();
}

HRESULT CResizeCropMFT::GetAttributes(IMFAttributes** ppAttributes)
{
    *ppAttributes = m_spAttributes;
    (*ppAttributes)->AddRef();
    
    return S_OK;
}

void CResizeCropMFT::HandleInputTypeSet()
{
    CComPtr<IMFMediaType> spType;
    UINT32 rectCrop[4];

    PTypeHandler()->GetInputCurrentType(0, &spType);
    m_spResizer->SetInputType(0, spType, 0);

    HRESULT hr = m_spAttributes->GetBlob(RESIZE_PARAM_CROP, (UINT8*) rectCrop, 4 * sizeof(UINT32), NULL);
    if(SUCCEEDED(hr))
    {
        SetCropParams(rectCrop[0], rectCrop[1], rectCrop[2], rectCrop[3]);
    }
}

void CResizeCropMFT::HandleOutputTypeSet()
{
    CComPtr<IMFMediaType> spType;
    UINT32 rectCrop[4];

    PTypeHandler()->GetOutputCurrentType(0, &spType);
    m_spResizer->SetOutputType(0, spType, 0);

    HRESULT hr = m_spAttributes->GetBlob(RESIZE_PARAM_CROP, (UINT8*) rectCrop, 4 * sizeof(UINT32), NULL);
    if(SUCCEEDED(hr))
    {
        SetCropParams(rectCrop[0], rectCrop[1], rectCrop[2], rectCrop[3]);
    }
}

HRESULT CResizeCropMFT::GetFriendlyName(__out LPWSTR* pszFriendlyName)
{
    CAtlString str(L"Resize & Crop");
    *pszFriendlyName = (LPWSTR) CoTaskMemAlloc(sizeof(WCHAR) * (str.GetLength() + 1));
    wcscpy_s(*pszFriendlyName, str.GetLength(), str.GetString());
    return S_OK;
}

HRESULT CResizeCropMFT::QueryRequiresConfiguration(__out BOOL* pfRequiresConfiguration)
{
    *pfRequiresConfiguration = TRUE;
    
    return S_OK;
}

HRESULT CResizeCropMFT::Configure(LONG_PTR hWnd, __in IMFSample* pExampleSample, __in IMFMediaType* pSampleMediaType)
{
    HRESULT hr = S_OK;

    hr = MFGetAttributeSize(pSampleMediaType, MF_MT_FRAME_SIZE, &m_unSampleWidth, &m_unSampleHeight);
    
    if(SUCCEEDED(hr))
    {
        CResizeCropDialog dialog(pExampleSample, m_unSampleWidth, m_unSampleHeight, 0, 0, m_unSampleWidth, m_unSampleHeight);
        if(dialog.DoModal() == IDOK)
        {
            CHECK_HR( hr = InitResizer(dialog.GetOutputWidth(), dialog.GetOutputHeight(), dialog.GetInputX(), m_unSampleHeight - (dialog.GetInputY() + dialog.GetInputHeight()),
                    dialog.GetInputWidth(), dialog.GetInputHeight()) );
                
            m_spAttributes->SetUINT32(RESIZE_PARAM_DESTWIDTH, dialog.GetOutputWidth());
            m_spAttributes->SetUINT32(RESIZE_PARAM_DESTHEIGHT, dialog.GetOutputHeight());
        
            UINT32 rectCrop[4];
            rectCrop[0] = dialog.GetInputX();
            rectCrop[1] = m_unSampleHeight - (dialog.GetInputY() + dialog.GetInputHeight());
            rectCrop[2] = dialog.GetInputWidth();
            rectCrop[3] = dialog.GetInputHeight();
            m_spAttributes->SetBlob(RESIZE_PARAM_CROP, (UINT8*) &rectCrop, 4 * sizeof(UINT32));
        }
        else
        {
            hr = E_ABORT;
        }
    }

done:
    return hr;
}

HRESULT CResizeCropMFT::CloneMFT(__out IMFTConfiguration** ppClonedTransform)
{
    HRESULT hr;
    CComPtr<IUnknown> spTransformUnk;
    
    CComObject<CResizeCropMFT>* pMFT = NULL;
    CHECK_HR( hr = CComObject<CResizeCropMFT>::CreateInstance(&pMFT) );
    CHECK_HR( hr = pMFT->QueryInterface(IID_IMFTConfiguration, (void**) ppClonedTransform) );
    CResizeCropMFT* pRCMFT = (CResizeCropMFT*) *ppClonedTransform;
    m_spAttributes->CopyAllItems(pRCMFT->m_spAttributes);
    pRCMFT->UpdateConfiguration();
   
done:
    return hr;
}

HRESULT CResizeCropMFT::GetParamCount(__out DWORD* pcParams)
{
    *pcParams = 3;
    
    return S_OK;
}

HRESULT CResizeCropMFT::GetParam(DWORD dwIndex, __deref_out LPWSTR* pszName, __out MFVE_PARAM_TYPE* pParamType, __out GUID* pgidKey)
{
    if(dwIndex > 2) return E_INVALIDARG;
    if(NULL == pParamType) return E_POINTER;
    if(NULL == pgidKey) return E_POINTER;
    
    switch(dwIndex)
    {
    case 0:
        *pParamType = MFVE_PARAM_UINT32;
        *pgidKey = RESIZE_PARAM_DESTWIDTH;
        break;
    case 1:
        *pParamType = MFVE_PARAM_UINT32;
        *pgidKey = RESIZE_PARAM_DESTHEIGHT;
        break;
    case 2:
        *pParamType = MFVE_PARAM_IMAGE_RECT;
        *pgidKey = RESIZE_PARAM_CROP;
        break;
    }
    
    return S_OK;
}

HRESULT CResizeCropMFT::GetParamConstraint(DWORD dwIndex, __out PROPVARIANT* pvarMin, __out PROPVARIANT* pvarMax, __out PROPVARIANT* pvarStep)
{
    if(dwIndex > 2) return E_INVALIDARG;
    if(NULL == pvarMin) return E_POINTER;
    if(NULL == pvarMax) return E_POINTER;
    if(NULL == pvarStep) return E_POINTER;
    
    switch(dwIndex)
    {
    case 0:
        pvarMin->vt = VT_EMPTY;
        pvarMax->vt = VT_EMPTY;
    
        pvarStep->vt = VT_UI4;
        pvarStep->ulVal = 1;
        break;
    case 1:
        pvarMin->vt = VT_EMPTY;
        pvarMax->vt = VT_EMPTY;
    
        pvarStep->vt = VT_UI4;
        pvarStep->ulVal = 1;
        break;
    case 2:
        pvarMin->vt = VT_EMPTY;
        pvarMax->vt = VT_EMPTY;
        pvarStep->vt = VT_EMPTY;
    }
    
    return S_OK;
}

HRESULT CResizeCropMFT::UpdateConfiguration()
{
    UINT32 rectCrop[4];
    HRESULT hr = m_spAttributes->GetBlob(RESIZE_PARAM_CROP, (UINT8*) rectCrop, 4 * sizeof(UINT32), NULL);
 
    if(SUCCEEDED(hr))
    {
        InitResizer(    MFGetAttributeUINT32(m_spAttributes, RESIZE_PARAM_DESTWIDTH,0),
                        MFGetAttributeUINT32(m_spAttributes, RESIZE_PARAM_DESTHEIGHT, 0),
                        rectCrop[0], rectCrop[1], rectCrop[2], rectCrop[3]
                    );
    }
    
    return hr;
}

HRESULT CResizeCropMFT::GetParamValues(DWORD dwIndex, __out DWORD* pcValues, __out PROPVARIANT** ppvarValues)
{
    return E_NOTIMPL;
}

HRESULT CResizeCropMFT::GetStreamIDs(DWORD dwInputIDArraySize, DWORD *pdwInputIDs, DWORD dwOutputIDArraySize, DWORD *pdwOutputIDs)
{
    if(!m_fConfigured) return MF_E_NOT_INITIALIZED;
    return m_spResizer->GetStreamIDs(dwInputIDArraySize, pdwInputIDs, dwOutputIDArraySize, pdwOutputIDs);
}

HRESULT CResizeCropMFT::GetInputStatus(DWORD dwInputStreamID, DWORD *pdwFlags)
{
    if(!m_fConfigured) return MF_E_NOT_INITIALIZED;
    return m_spResizer->GetInputStatus(dwInputStreamID, pdwFlags);
}

HRESULT CResizeCropMFT::GetOutputStatus(DWORD *pdwFlags)
{
    if(!m_fConfigured) return MF_E_NOT_INITIALIZED;
    return m_spResizer->GetOutputStatus(pdwFlags);
}

HRESULT CResizeCropMFT::ProcessMessage(MFT_MESSAGE_TYPE eMessage, ULONG_PTR ulParam)
{
    if(!m_fConfigured) return MF_E_NOT_INITIALIZED;
    return m_spResizer->ProcessMessage(eMessage, ulParam);
}

HRESULT CResizeCropMFT::ProcessInput(DWORD dwInputStreamID, IMFSample* pSample, DWORD dwFlags)
{
    if(!m_fConfigured) return MF_E_NOT_INITIALIZED;
    return m_spResizer->ProcessInput(dwInputStreamID, pSample, dwFlags);
}

HRESULT CResizeCropMFT::ProcessOutput(DWORD dwFlags, DWORD cOutputBufferCount, MFT_OUTPUT_DATA_BUFFER* pOutputSamples, DWORD *pdwStatus)
{
    if(!m_fConfigured) return MF_E_NOT_INITIALIZED;
    return m_spResizer->ProcessOutput(dwFlags, cOutputBufferCount, pOutputSamples, pdwStatus);
}

HRESULT CResizeCropMFT::InitResizer(UINT32 unOutputWidth, UINT32 unOutputHeight, UINT32 unInputX, UINT32 unInputY, UINT32 unInputWidth, UINT32 unInputHeight)
{
    HRESULT hr;
    CComPtr<IUnknown> spTransformUnk;
    
    CHECK_HR( hr = CoCreateInstance(CLSID_CResizerDMO, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**) &spTransformUnk) );
    CHECK_HR( hr = spTransformUnk->QueryInterface(IID_IMFTransform, (void**) &m_spResizer) );
    
    ((CRGB1in1outTypeHandler*) PTypeHandler())->SetOutputFrameSize(unOutputWidth, unOutputHeight);
    
    CHECK_HR( hr = SetCropParams(unInputX, unInputY, unInputWidth, unInputHeight) );
    
    m_fConfigured = true;
    
done:
    return hr;
}

HRESULT CResizeCropMFT::SetCropParams(UINT32 unInputX, UINT32 unInputY, UINT32 unInputWidth, UINT32 unInputHeight)
{
    HRESULT hr;
    PROPVARIANT var;
    CComPtr<IPropertyStore> spResizerProps;

    PropVariantInit(&var);
    var.vt = VT_I4;

    CHECK_HR( hr = m_spResizer->QueryInterface(IID_PPV_ARGS(&spResizerProps)) );

    var.lVal = unInputX;
    CHECK_HR( hr = spResizerProps->SetValue(MFPKEY_RESIZE_SRC_LEFT, var) );
    var.lVal = unInputY;
    CHECK_HR( hr = spResizerProps->SetValue(MFPKEY_RESIZE_SRC_TOP, var) );
    var.lVal = unInputWidth;
    CHECK_HR( hr = spResizerProps->SetValue(MFPKEY_RESIZE_SRC_WIDTH, var) );
    var.lVal = unInputHeight;
    CHECK_HR( hr = spResizerProps->SetValue(MFPKEY_RESIZE_SRC_HEIGHT, var) );

done:
    PropVariantClear(&var);

    return hr;
}

CResizeCropDialog::CResizeCropDialog(IMFSample* pExampleSample, UINT32 unSampleWidth, UINT32 unSampleHeight, UINT32 unInputLeft, UINT32 unInputTop, UINT32 unInputWidth, UINT32 unInputHeight)
    : m_unOutputWidth(unSampleWidth)
    , m_unOutputHeight(unSampleHeight)
    , m_unInputX(unInputLeft)
    , m_unInputY(unInputTop)
    , m_unInputWidth(unInputWidth)
    , m_unInputHeight(unInputHeight)
    , m_spExampleSample(pExampleSample)
    , m_unSampleWidth(unSampleWidth)
    , m_unSampleHeight(unSampleHeight)
    , m_fInitialized(false)
{
}

CResizeCropDialog::~CResizeCropDialog()
{
}

LRESULT CResizeCropDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RECT rectClient;
    GetClientRect(&rectClient);
    
    RECT rectNewWindow;
    rectNewWindow.left = rectClient.left + 5;
    rectNewWindow.right = rectClient.right - 5;
    rectNewWindow.top = rectClient.top + 120;
    rectNewWindow.bottom = rectClient.bottom - 40;
    
    m_pSampleOutputWindow = new CSampleOutputWindow();
    m_pSampleOutputWindow->Create(m_hWnd, rectNewWindow, L"Sample Output Window", WS_CHILD | WS_VISIBLE, 0, 0U, NULL);
    m_pSampleOutputWindow->SetSampleSize(m_unSampleWidth, m_unSampleHeight);
    m_pSampleOutputWindow->SetOutputSample(m_spExampleSample);
    m_pSampleOutputWindow->SetSelection(0, 0, m_unSampleWidth, m_unSampleHeight);
    
    SetTextBoxValue(IDC_OUTPUTWIDTH, m_unOutputWidth);
    SetTextBoxValue(IDC_OUTPUTHEIGHT, m_unOutputHeight);
    SetTextBoxValue(IDC_SOURCELEFT, m_unInputX);
    SetTextBoxValue(IDC_SOURCETOP, m_unInputY);
    SetTextBoxValue(IDC_SOURCEWIDTH, m_unInputWidth);
    SetTextBoxValue(IDC_SOURCEHEIGHT, m_unInputHeight);
    
    m_fInitialized = true;
    
    return 0;
}

LRESULT CResizeCropDialog::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_unOutputWidth = GetTextBoxValue(IDC_OUTPUTWIDTH);
    m_unOutputHeight = GetTextBoxValue(IDC_OUTPUTHEIGHT);
    m_unInputX = GetTextBoxValue(IDC_SOURCELEFT);
    m_unInputY = GetTextBoxValue(IDC_SOURCETOP);
    m_unInputWidth = GetTextBoxValue(IDC_SOURCEWIDTH);
    m_unInputHeight = GetTextBoxValue(IDC_SOURCEHEIGHT);
    
    if(m_unInputX > m_unSampleWidth) m_unInputX = m_unSampleWidth;
    if(m_unInputY > m_unSampleHeight) m_unInputY = m_unSampleHeight;
    if(m_unInputX + m_unInputWidth > m_unSampleWidth) m_unInputWidth = m_unSampleWidth - m_unInputX;
    if(m_unInputY + m_unInputHeight > m_unSampleHeight) m_unInputHeight = m_unSampleHeight - m_unInputY;
    
    EndDialog(IDCANCEL);

    return 0;
}

LRESULT CResizeCropDialog::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    m_unOutputWidth = GetTextBoxValue(IDC_OUTPUTWIDTH);
    m_unOutputHeight = GetTextBoxValue(IDC_OUTPUTHEIGHT);
    m_unInputX = GetTextBoxValue(IDC_SOURCELEFT);
    m_unInputY = GetTextBoxValue(IDC_SOURCETOP);
    m_unInputWidth = GetTextBoxValue(IDC_SOURCEWIDTH);
    m_unInputHeight = GetTextBoxValue(IDC_SOURCEHEIGHT);
    
    if(m_unInputX > m_unSampleWidth) m_unInputX = m_unSampleWidth;
    if(m_unInputY > m_unSampleHeight) m_unInputY = m_unSampleHeight;
    if(m_unInputX + m_unInputWidth > m_unSampleWidth) m_unInputWidth = m_unSampleWidth - m_unInputX;
    if(m_unInputY + m_unInputHeight > m_unSampleHeight) m_unInputHeight = m_unSampleHeight - m_unInputY;
    
    EndDialog(IDOK);
    
    return 0;
}

LRESULT CResizeCropDialog::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(IDCANCEL);
    
    return 0;
}

LRESULT CResizeCropDialog::OnCropChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if(!m_fInitialized) return 0;
    
    m_unInputX = GetTextBoxValue(IDC_SOURCELEFT);
    m_unInputY = GetTextBoxValue(IDC_SOURCETOP);
    m_unInputWidth = GetTextBoxValue(IDC_SOURCEWIDTH);
    m_unInputHeight = GetTextBoxValue(IDC_SOURCEHEIGHT);
    
    m_pSampleOutputWindow->SetSelection(m_unInputX, m_unInputY, m_unInputX + m_unInputWidth, m_unInputY + m_unInputHeight);
    
    return 0;
}

UINT32 CResizeCropDialog::GetTextBoxValue(UINT32 unControlID)
{
    HWND hWnd = GetDlgItem(unControlID);
    int iLen = ::GetWindowTextLength(hWnd);
    LPWSTR szTextBox = new WCHAR[iLen + 1];
    ::GetWindowText(hWnd, szTextBox, iLen + 1);
    
    UINT32 unResult = _wtoi(szTextBox);
    
    delete[] szTextBox;
    
    return unResult;
}

void CResizeCropDialog::SetTextBoxValue(UINT32 unControlID, UINT32 unValue)
{
    CAtlString str;
    str.Format(L"%d", unValue);
    
    ::SetWindowText(GetDlgItem(unControlID), str.GetString());
}