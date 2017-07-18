// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE
//
// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "stdafx.h"

#include "pcmmft.h"
#include "mferror.h"
#include "pcmtypehandler.h"
#include "wmcodecdsp.h"
#include "mfveapi.h"

// CPcmMFT is a generic MFT that takes a plugin object that
// performs the actual transform computation.  
CPcmMFT::CPcmMFT(CPCMSampleTransform* pTransform)
    : m_pSampleTransform(pTransform)
{
    Init(new CPCM1in1outTypeHandler());
}

CPcmMFT::~CPcmMFT()
{
    delete PTypeHandler();
}

HRESULT CPcmMFT::GetStreamIDs(DWORD dwInputIDArraySize, DWORD *pdwInputIDs, DWORD dwOutputIDArraySize, DWORD *pdwOutputIDs)
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

HRESULT CPcmMFT::GetInputStatus(DWORD dwInputStreamID, DWORD *pdwFlags)
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

HRESULT CPcmMFT::GetOutputStatus(DWORD *pdwFlags)
{
    return E_NOTIMPL;
}

HRESULT CPcmMFT::ProcessMessage(MFT_MESSAGE_TYPE eMessage, ULONG_PTR ulParam)
{
    switch(eMessage)
    {
        case MFT_MESSAGE_COMMAND_FLUSH:
            m_spCurrentSample.Release();
            break;
    }
    
    return S_OK;
}

HRESULT CPcmMFT::ProcessInput(DWORD dwInputStreamID, IMFSample* pSample, DWORD dwFlags)
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

HRESULT CPcmMFT::ProcessOutput(DWORD dwFlags, DWORD cOutputBufferCount, MFT_OUTPUT_DATA_BUFFER* pOutputSamples, DWORD *pdwStatus)
{
    HRESULT hr = S_OK;
    CComPtr<IMFMediaBuffer> spInBuffer;
    BYTE* pbInBuffer = NULL;
    CComPtr<IMFMediaBuffer> spOutBuffer;
    BYTE* pbOutBuffer = NULL;

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

        DWORD cbInLength;
        DWORD cbOutLength;
            
        CHECK_HR( hr = m_spCurrentSample->GetBufferByIndex(0, &spInBuffer) );
        spInBuffer->Lock(&pbInBuffer, NULL, &cbInLength);
           
        CHECK_HR( hr = pOutputSamples->pSample->GetBufferByIndex(0, &spOutBuffer) );
        spOutBuffer->Lock(&pbOutBuffer, &cbOutLength, NULL);

        DWORD cbMinLength = (cbOutLength > cbInLength) ? cbInLength : cbOutLength;
        spOutBuffer->SetCurrentLength(cbMinLength);

        PTypeHandler()->GetInputCurrentType(0, &spType);

        UINT32 nBitsPerSample, nChannels, nSampleRate;
        CHECK_HR( hr = spType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &nBitsPerSample) );
        CHECK_HR( hr = spType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &nChannels) );
        CHECK_HR( hr = spType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &nSampleRate) );
    
        m_pSampleTransform->Transform(nBitsPerSample, nChannels, nSampleRate, cbMinLength, pbInBuffer, pbOutBuffer);
        
        m_spCurrentSample.Release();
    }

done:
    if(pbInBuffer)
    {
        spInBuffer->Unlock();
    }

    if(pbOutBuffer)
    {
        spOutBuffer->Unlock();
    }

    return hr;
}

////////////////////////////////////////////////////////

CVolumeCompressionMFT::CVolumeCompressionMFT()
    : m_VolumeCompressionTransform(1.0f)
    , CPcmMFT(&m_VolumeCompressionTransform)
{
    MFCreateAttributes(&m_spAttributes, 1);
}

CVolumeCompressionMFT::~CVolumeCompressionMFT()
{
}

HRESULT CVolumeCompressionMFT::GetAttributes(__out IMFAttributes** ppAttributes)
{
    *ppAttributes = m_spAttributes;
    (*ppAttributes)->AddRef();

    return S_OK;
}

HRESULT CVolumeCompressionMFT::GetFriendlyName(__out LPWSTR* pszFriendlyName)
{
    CAtlString str(L"Volume Compression");
    *pszFriendlyName = (LPWSTR) CoTaskMemAlloc(sizeof(WCHAR) * (str.GetLength() + 1));
    wcscpy_s(*pszFriendlyName, str.GetLength(), str.GetString());

    return S_OK;
}

HRESULT CVolumeCompressionMFT::QueryRequiresConfiguration(__out BOOL* pfRequiresConfiguration)
{
    *pfRequiresConfiguration = TRUE;

    return S_OK;
}

HRESULT CVolumeCompressionMFT::Configure(LONG_PTR hWnd, __in IMFSample* pExampleSample, __in IMFMediaType* pSampleMediaType)
{
    CVolumeCompressionDialog dialog(pExampleSample, 1.0f);
    if(dialog.DoModal() == IDOK)
    {
        m_spAttributes->SetDouble(VOLCOMP_PARAM_FACTOR, dialog.GetCompressionFactor());
        m_VolumeCompressionTransform.SetFactor(dialog.GetCompressionFactor());
    }
    else
    {
        return E_ABORT;
    }

    return S_OK;
}

HRESULT CVolumeCompressionMFT::CloneMFT(__out IMFTConfiguration** ppClonedTransform)
{
    HRESULT hr;
    CComPtr<IUnknown> spTransformUnk;
    
    CComObject<CVolumeCompressionMFT>* pMFT = NULL;
    CHECK_HR( hr = CComObject<CVolumeCompressionMFT>::CreateInstance(&pMFT) );
    CHECK_HR( hr = pMFT->QueryInterface(IID_PPV_ARGS(ppClonedTransform)) );
    this->m_spAttributes->CopyAllItems(pMFT->m_spAttributes);
    pMFT->UpdateConfiguration();
   
done:
    return hr;
}

HRESULT CVolumeCompressionMFT::GetParamCount(__out DWORD* pcParams)
{
    *pcParams = 1;

    return S_OK;
}

HRESULT CVolumeCompressionMFT::GetParam(DWORD dwIndex, __deref_out LPWSTR* pszName, __out MFVE_PARAM_TYPE* pParamType, __out GUID* pgidKey)
{
    if(dwIndex > 0) return E_INVALIDARG;
    
    switch(dwIndex)
    {
    case 0:
        *pParamType = MFVE_PARAM_DOUBLE;
        *pgidKey = VOLCOMP_PARAM_FACTOR;
        break;
    }
    
    return S_OK;
}

HRESULT CVolumeCompressionMFT::GetParamConstraint(DWORD dwIndex, __out PROPVARIANT* pvarMin, __out PROPVARIANT* pvarMax, __out PROPVARIANT* pvarStep)
{
    if(dwIndex > 0) return E_INVALIDARG;
    
    switch(dwIndex)
    {
    case 0:
        pvarMin->vt = VT_R8;
        pvarMin->dblVal = 0.0;

        pvarMax->vt = VT_EMPTY;
    
        pvarStep->vt = VT_R8;
        pvarStep->dblVal = .1;
        break;
    }
    
    return S_OK;
}

HRESULT CVolumeCompressionMFT::GetParamValues(DWORD dwIndex, __out DWORD* pcValues, __out PROPVARIANT** ppvarValues)
{
    return E_NOTIMPL;
}

HRESULT CVolumeCompressionMFT::UpdateConfiguration()
{
    double dblFactor;

    HRESULT hr = m_spAttributes->GetDouble(VOLCOMP_PARAM_FACTOR, &dblFactor);
    m_VolumeCompressionTransform.SetFactor(static_cast<float>(dblFactor));

    return hr;
}

CVolumeCompressionDialog::CVolumeCompressionDialog(IMFSample* pExampleSample, float flFactor)
: _spExampleSample(pExampleSample)
, _flFactor(flFactor)
{
}

CVolumeCompressionDialog::~CVolumeCompressionDialog()
{
}

LRESULT CVolumeCompressionDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    CAtlString strFormat;
    strFormat.Format(L"%2.1f", _flFactor);

    ::SetWindowText(GetDlgItem(IDC_FACTOR), strFormat.GetString());

    return 0;
}

LRESULT CVolumeCompressionDialog::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HWND hWnd = GetDlgItem(IDC_FACTOR);
    int iLen = ::GetWindowTextLength(hWnd);
    LPWSTR szTextBox = new WCHAR[iLen + 1];
    ::GetWindowText(hWnd, szTextBox, iLen + 1);

    _flFactor = static_cast<float>(_wtof(szTextBox));

    delete[] szTextBox;

    EndDialog(IDCANCEL);

    return 0;
}
    
LRESULT CVolumeCompressionDialog::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HWND hWnd = GetDlgItem(IDC_FACTOR);
    int iLen = ::GetWindowTextLength(hWnd);
    LPWSTR szTextBox = new WCHAR[iLen + 1];
    ::GetWindowText(hWnd, szTextBox, iLen + 1);

    _flFactor = static_cast<float>(_wtof(szTextBox));

    delete[] szTextBox;

    EndDialog(IDOK);

    return 0;
}

LRESULT CVolumeCompressionDialog::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(IDCANCEL);
    
    return 0;
}