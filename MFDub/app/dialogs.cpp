// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE
//
// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "stdafx.h"

#include "mfveutil.h"
#include "dialogs.h"

#include <initguid.h>
#include "mfveapi.h"

#define HNS_TO_HOURS 36000000000
#define HNS_TO_MINUTES 600000000
#define HNS_TO_SECONDS 10000000
#define HNS_TO_MILLISECONDS 10000

CAddTransformDialog::CAddTransformDialog(GUID gidCategory)
{
    CLSID* pClsids;
    UINT32 cClsids;
    HRESULT hr = MFVEMFTEnum(gidCategory, &pClsids, &cClsids);
    
    if(FAILED(hr)) return;
    
    for(UINT32 i = 0; i < cClsids; i++)
    {
        m_CLSIDs.Add(pClsids[i]);
        
        LPWSTR szName, szDesc;
        MFVEMFTGetInfo(pClsids[i], &szName, &szDesc);
        
        m_strNames.Add(CAtlString(szName));
        m_strDescs.Add(CAtlString(szDesc));
        
        CoTaskMemFree(szName);
        CoTaskMemFree(szDesc);
    }
    
    CoTaskMemFree(pClsids);
}

CLSID CAddTransformDialog::GetChosenCLSID() const
{
    return m_CLSIDs.GetAt(m_nChosenIndex);
}

CAtlString CAddTransformDialog::GetChosenName() const
{
    return m_strNames.GetAt(m_nChosenIndex);
}

LRESULT CAddTransformDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_hList = GetDlgItem(IDC_TRANSFORMLIST);
    m_hDesc = GetDlgItem(IDC_TRANSFORMDESC);
    
    for(size_t i = 0; i < m_strNames.GetCount(); i++)
    {
        SendMessage(m_hList, LB_ADDSTRING, 0, LPARAM(m_strNames.GetAt(i).GetString()));
    }
    
    return 0;
}

LRESULT CAddTransformDialog::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_nChosenIndex = static_cast<unsigned int>(SendMessage(m_hList, LB_GETCURSEL, 0, 0));
    
    if(m_nChosenIndex > m_strNames.GetCount())
    {
        m_nChosenIndex = 0;
    }

    EndDialog(IDCANCEL);
    
    return 0;
}

LRESULT CAddTransformDialog::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    m_nChosenIndex = static_cast<unsigned int>(SendMessage(m_hList, LB_GETCURSEL, 0, 0));
    
    if(m_nChosenIndex > m_strNames.GetCount())
    {
        m_nChosenIndex = 0;
    }
    
    EndDialog(IDOK);
    
    return 0;
}

LRESULT CAddTransformDialog::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(IDCANCEL);
    
    return 0;
}

LRESULT CAddTransformDialog::OnTransformSelectionChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    m_nChosenIndex = static_cast<unsigned int>(SendMessage(m_hList, LB_GETCURSEL, 0, 0));
    
    if(m_nChosenIndex > m_strDescs.GetCount())
    {
        m_nChosenIndex = 0;
    }
    
    ::SetWindowText(m_hDesc, m_strDescs.GetAt(m_nChosenIndex).GetString());
    
    return 0;
}

/////////////////////////////

CEncodeOptionsDialog::CEncodeOptionsDialog(int iQuality, double dbOriginalFrameRate)
    : m_fFrameRateChanged(false)
    , m_iValue(iQuality)
    , m_dbFrameRate(dbOriginalFrameRate)
    , m_dbOriginalFrameRate(dbOriginalFrameRate)
{
}


int CEncodeOptionsDialog::GetChosenValue()
{
    return m_iValue;
}

LRESULT CEncodeOptionsDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_hCBRButton = GetDlgItem(IDC_CBR);
    m_hVBRButton = GetDlgItem(IDC_VBR);
    m_hLabel = GetDlgItem(IDC_BOXLABEL);
    m_hValue = GetDlgItem(IDC_VALUE);
    m_hFrameRate = GetDlgItem(IDC_FRAMERATE);
    
    ::SendMessage(m_hCBRButton, BM_SETCHECK, BST_CHECKED, 0);
    ::SetWindowText(m_hLabel, L"Bitrate:");
    
    CAtlString str;
    str.Format(L"%d", m_iValue);
    ::SetWindowText(m_hValue, str.GetString());
    
    str.Format(L"%.3f", m_dbFrameRate);
    ::SetWindowText(m_hFrameRate, str.GetString());
    
    return 0;
}

LRESULT CEncodeOptionsDialog::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_iValue = GetValueAsInteger();
    m_dbFrameRate = GetFrameRateAsDouble();
    EndDialog(IDCANCEL);

    return 0;
}

LRESULT CEncodeOptionsDialog::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    m_iValue = GetValueAsInteger();
    m_dbFrameRate = GetFrameRateAsDouble();
    EndDialog(IDOK);
    
    return 0;
}

LRESULT CEncodeOptionsDialog::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(IDCANCEL);
    
    return 0;
}

LRESULT CEncodeOptionsDialog::OnCBRClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    ::SetWindowText(m_hLabel, L"Bitrate:");
    ::SetWindowText(m_hValue, L"300000");
    
    return 0;
}

LRESULT CEncodeOptionsDialog::OnValueChangeFinished(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    return 0;
}

LRESULT CEncodeOptionsDialog::OnChangeFrameRateClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if(m_fFrameRateChanged)
    {
        m_dbFrameRate = m_dbOriginalFrameRate;
        
        CAtlString str;
        str.Format(L"%.3f", m_dbFrameRate);
        ::SetWindowText(m_hFrameRate, str.GetString());
        
        ::SendMessage(m_hFrameRate, EM_SETREADONLY, TRUE, 0);
        
        m_fFrameRateChanged = false;
    }
    else
    {
        ::SendMessage(m_hFrameRate, EM_SETREADONLY, FALSE, 0);
        
        m_fFrameRateChanged = true;
    }
    
    return 0;
}

int CEncodeOptionsDialog::GetValueAsInteger()
{
    int iLen = ::GetWindowTextLength(m_hValue);
    LPWSTR szValue = new WCHAR[iLen + 1];
    ::GetWindowText(m_hValue, szValue, iLen + 1);
    
    int iValue = _wtoi(szValue);
    
    delete[] szValue;
    
    return iValue;
}

double CEncodeOptionsDialog::GetFrameRateAsDouble()
{
    int iLen = ::GetWindowTextLength(m_hFrameRate);
    LPWSTR szValue = new WCHAR[iLen + 1];
    ::GetWindowText(m_hFrameRate, szValue, iLen + 1);
    
    double dbValue = _wtof(szValue);
    
    delete[] szValue;
    
    return dbValue;
}

/////////////////////////////////////////////

const StringGuidPair CMetadataDialog::ms_SubtypeToString[] = {
    { MFAudioFormat_Dolby_AC3_SPDIF, L"Dolby AC-3 over S/PDIF" },
    { MFAudioFormat_DRM, L"Encrypted" },
    { MFAudioFormat_DTS, L"DTS" },
    { MFAudioFormat_Float, L"Uncompressed Float" },
    { MFAudioFormat_MP3, L"MP3" },
    { MFAudioFormat_MPEG, L"MPEG-1" },
    { MFAudioFormat_MSP1, L"WMVoice9" },
    { MFAudioFormat_PCM, L"Uncompressed PCM" },
    { MFAudioFormat_WMASPDIF, L"WMA9 over S/PDIF" },
    { MFAudioFormat_WMAudio_Lossless, L"WMA9 Lossless" },
    { MFAudioFormat_WMAudioV8, L"WMA8" },
    { MFAudioFormat_WMAudioV9, L"WMA9 Professional" },
    { MFVideoFormat_DV25, L"DVCPRO 25" },
    { MFVideoFormat_DV50, L"DVCPRO 50" },
    { MFVideoFormat_DVH1, L"DVCPRO 100" },
    { MFVideoFormat_DVSD, L"SDL-DVCR" },
    { MFVideoFormat_DVSL, L"SD_DVCR" },
    { MFVideoFormat_MP43, L"MS-MPEG-4" },
    { MFVideoFormat_MP4S, L"ISO-MPEG-4" },
    { MFVideoFormat_MPEG2, L"MPEG-2" },
    { MFVideoFormat_MPG1, L"MPEG-1" },
    { MFVideoFormat_MSS1, L"WMScreen1" },
    { MFVideoFormat_MSS2, L"WMScreen9" },
    { MFVideoFormat_WMV1, L"WMV7" },
    { MFVideoFormat_WMV2, L"WMV8", },
    { MFVideoFormat_WMV3, L"WMV9", }
};

#define SubtypeToStringSize sizeof(ms_SubtypeToString) / sizeof(StringGuidPair)

CMetadataDialog::CMetadataDialog(LPCWSTR szSourceURL, IMFSourceReader* pSourceReader, IMFMediaType* pAudioType, IMFMediaType* pVideoType, DWORD dwFrameCount, DWORD dwKeyframeCount)
    : m_szSourceURL(szSourceURL)
    , m_spSourceReader(pSourceReader)
    , m_spAudioType(pAudioType)
    , m_spVideoType(pVideoType)
    , m_dwFrameCount(dwFrameCount)
    , m_dwKeyframeCount(dwKeyframeCount)
{
}

LRESULT CMetadataDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HRESULT hr;

    SetControlText(IDC_FILENAME, m_szSourceURL);

    if(m_spSourceReader.p)
    {
        CAtlString str;
        PROPVARIANT varFileSize;
        PropVariantInit(&varFileSize);
        hr = m_spSourceReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_TOTAL_FILE_SIZE, &varFileSize);
        if(SUCCEEDED(hr) && VT_UI8 == varFileSize.vt)
        {
            str.Format(L"%I64d Bytes", varFileSize.uhVal.QuadPart);
            SetControlText(IDC_FILESIZE, str);
        }
        PropVariantClear(&varFileSize);
        
        PROPVARIANT varDuration;
        PropVariantInit(&varDuration);
        hr = m_spSourceReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &varDuration);
        if(SUCCEEDED(hr) && VT_UI8 == varDuration.vt)
        {
            SetControlTime(IDC_DURATION, varDuration.uhVal.QuadPart);
        }
        PropVariantClear(&varDuration);
        
        PROPVARIANT varMimeType;
        PropVariantInit(&varMimeType);
        hr = m_spSourceReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_MIME_TYPE, &varDuration);
        if(SUCCEEDED(hr) && VT_LPWSTR == varMimeType.vt)
        {
            SetControlText(IDC_MIMETYPE, varMimeType.pwszVal);
        }
        PropVariantClear(&varMimeType);
    }
    
    if(m_spAudioType.p)
    {
        GUID gidSubtype;
        HRESULT hr = m_spAudioType->GetGUID(MF_MT_SUBTYPE, &gidSubtype);
        if(SUCCEEDED(hr))
        {
            SetControlSubtype(IDC_SUBTYPEAUDIO, gidSubtype);
        }
        
        SetControlUINT32(IDC_NUMCHANNELS, MFGetAttributeUINT32(m_spAudioType, MF_MT_AUDIO_NUM_CHANNELS, 0));
        SetControlUINT32(IDC_AVGBYTESSEC, MFGetAttributeUINT32(m_spAudioType, MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 0));
        SetControlUINT32(IDC_BLOCKALIGN, MFGetAttributeUINT32(m_spAudioType, MF_MT_AUDIO_BLOCK_ALIGNMENT, 0));
        SetControlUINT32(IDC_BITSPERSAMPLE, MFGetAttributeUINT32(m_spAudioType, MF_MT_AUDIO_BITS_PER_SAMPLE, 0));
        SetControlUINT32(IDC_SAMPLESPERSEC, MFGetAttributeUINT32(m_spAudioType, MF_MT_AUDIO_SAMPLES_PER_SECOND, 0));
    }
        
    if(m_spVideoType.p)
    {
        GUID gidSubtype;
        HRESULT hr = m_spVideoType->GetGUID(MF_MT_SUBTYPE, &gidSubtype);
        if(SUCCEEDED(hr))
        {
            SetControlSubtype(IDC_SUBTYPE, gidSubtype);
        }
        
        UINT32 unWidth, unHeight;
        hr = MFGetAttributeSize(m_spVideoType, MF_MT_FRAME_SIZE, &unWidth, &unHeight);
        if(SUCCEEDED(hr)) SetControlSizeText(IDC_FRAMESIZE, unWidth, unHeight);
        
        UINT32 unNumerator, unDenominator;
        hr = MFGetAttributeRatio(m_spVideoType, MF_MT_FRAME_RATE, &unNumerator, &unDenominator);
        if(SUCCEEDED(hr)) SetControlDouble(IDC_FPS, double(unNumerator) / double(unDenominator));
        
        hr = MFGetAttributeRatio(m_spVideoType, MF_MT_PIXEL_ASPECT_RATIO, &unNumerator, &unDenominator);
        if(SUCCEEDED(hr)) SetControlSizeText(IDC_PAR, unNumerator, unDenominator);
        
        SetControlUINT32(IDC_AVGBITRATE, MFGetAttributeUINT32(m_spVideoType, MF_MT_AVG_BITRATE, 0));
        SetControlUINT32(IDC_FRAMECOUNT, m_dwFrameCount);
        SetControlUINT32(IDC_KEYFRAMECOUNT, m_dwKeyframeCount);
        
        UINT32 unInterlaceMode = MFGetAttributeUINT32(m_spVideoType, MF_MT_INTERLACE_MODE, 0);
        switch(unInterlaceMode)
        {
            case MFVideoInterlace_Progressive:
                SetControlText(IDC_INTERLACEMODE, L"Progressive (Not interlaced)");
                break;
            case MFVideoInterlace_FieldInterleavedUpperFirst:
                SetControlText(IDC_INTERLACEMODE, L"Interlaced frames with two fields, upper field first");
                break;
            case MFVideoInterlace_FieldInterleavedLowerFirst:
                SetControlText(IDC_INTERLACEMODE, L"Interlaced frames with two fields, lower field first");
                break;
            case MFVideoInterlace_FieldSingleUpper:
                SetControlText(IDC_INTERLACEMODE, L"Interlaced frames with one field, upper field first");
                break;
            case MFVideoInterlace_FieldSingleLower:
                SetControlText(IDC_INTERLACEMODE, L"Interlaced frames with one field, lower field first");
                break;
            case MFVideoInterlace_MixedInterlaceOrProgressive:
                SetControlText(IDC_INTERLACEMODE, L"Mixed interlaced and progressive");
                break;
            default:
                SetControlText(IDC_INTERLACEMODE, L"Unknown");
                break;
        }
    }
    
    return 0;
}

LRESULT CMetadataDialog::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    EndDialog(IDOK);

    return 0;
}

LRESULT CMetadataDialog::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(IDOK);
    
    return 0;
}

void CMetadataDialog::SetControlText(DWORD dwControlID, LPCWSTR szText)
{
    HWND hWnd = GetDlgItem(dwControlID);
    ::SetWindowText(hWnd, szText);
}

void CMetadataDialog::SetControlUINT64(DWORD dwControlID, UINT64 unNum)
{
    CAtlString strNum;
    strNum.Format(L"%I64d", unNum);
    SetControlText(dwControlID, strNum);
}

void CMetadataDialog::SetControlTime(DWORD dwControlID, UINT64 unNum)
{
    CAtlString strNum;
    
    DWORD dwHours = static_cast<DWORD>(unNum / HNS_TO_HOURS);
    unNum = unNum % HNS_TO_HOURS;
    DWORD dwMinutes = static_cast<DWORD>(unNum / HNS_TO_MINUTES);
    unNum = unNum % HNS_TO_MINUTES;
    DWORD dwSeconds = static_cast<DWORD>(unNum / HNS_TO_SECONDS);
    unNum = unNum % HNS_TO_SECONDS;
    DWORD dwMilliseconds = static_cast<DWORD>(unNum / HNS_TO_MILLISECONDS);

    strNum.Format(L"%2.2d:%2.2d:%2.2d.%3.3d", dwHours, dwMinutes, dwSeconds, dwMilliseconds);
    
    SetControlText(dwControlID, strNum);
}


void CMetadataDialog::SetControlUINT32(DWORD dwControlID, UINT32 unNum)
{
    CAtlString strNum;
    strNum.Format(L"%d", unNum);
    SetControlText(dwControlID, strNum);
}

void CMetadataDialog::SetControlSizeText(DWORD dwControlID, UINT32 unWidth, UINT32 unHeight)
{
    CAtlString strNum;
    strNum.Format(L"%dx%d", unWidth, unHeight);
    SetControlText(dwControlID, strNum);
}

void CMetadataDialog::SetControlSubtype(DWORD dwControlID, GUID gidSubtype)
{
    for(DWORD i = 0; i < SubtypeToStringSize; i++)
    {
        if(gidSubtype == ms_SubtypeToString[i].m_gidSubtype)
        {
            SetControlText(dwControlID, ms_SubtypeToString[i].m_szName);
            return;
        }
    }
    
    SetControlText(dwControlID, L"Unknown");
}

void CMetadataDialog::SetControlDouble(DWORD dwControlID, double val)
{
    CAtlString strNum;
    strNum.Format(L"%f", val);
    SetControlText(dwControlID, strNum);   
}