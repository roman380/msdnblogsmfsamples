// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "stdafx.h"
#include "common.h"
#include "helper.h"
#include "drmdump.h"

const WCHAR *g_DRMProperties[] = 
{
    g_wszWMDRM_IsDRM, // "IsDRM"
    g_wszWMDRM_IsDRMCached, // "IsDRMCached"
    g_wszWMDRM_BaseLicenseAcqURL, // "BaseLAURL"
    g_wszWMDRM_Rights, // "Rights"
    g_wszWMDRM_LicenseID, //"LID"
    g_wszWMDRM_DRMHeader_KeyID, //= "DRMHeader.KID"
    g_wszWMDRM_DRMHeader_LicenseAcqURL, // "DRMHeader.LAINFO"
    g_wszWMDRM_DRMHeader_ContentID, // "DRMHeader.CID"
    g_wszWMDRM_DRMHeader_IndividualizedVersion, // "DRMHeader.SECURITYVERSION"
    g_wszWMDRM_DRMHeader_ContentDistributor, // "DRMHeader.ContentDistributor"
    g_wszWMDRM_DRMHeader_SubscriptionContentID, // "DRMHeader.SubscriptionContentID"
    g_wszWMDRM_ActionAllowed_CollaborativePlay, // "ActionAllowed.CollaborativePlay"
    g_wszWMDRM_ActionAllowed_Copy, // "ActionAllowed.Copy"
    g_wszWMDRM_ActionAllowed_CopyToCD, // "ActionAllowed.Print.redbook"
    g_wszWMDRM_ActionAllowed_CopyToNonSDMIDevice, // "ActionAllowed.Transfer.NONSDMI"
    g_wszWMDRM_ActionAllowed_CopyToSDMIDevice, // "ActionAllowed.Transfer.SDMI"
    g_wszWMDRM_ActionAllowed_CreateThumbnailImage, // "ActionAllowed.CreateThumbnailImage"
    g_wszWMDRM_ActionAllowed_Playback, // "ActionAllowed.Play"
    g_wszWMDRM_ActionAllowed_PlaylistBurn, // "ActionAllowed.PlaylistBurn"
    g_wszWMDRM_ActionAllowed_Backup, // "ActionAllowed.Backup"
    g_wszWMDRM_LicenseState_Backup, // "LicenseStateData.Backup"
    g_wszWMDRM_LicenseState_CollaborativePlay, // "LicenseStateData.CollaborativePlay"
    g_wszWMDRM_LicenseState_Copy, // "LicenseStateData.Copy"
    g_wszWMDRM_LicenseState_CopyToCD, // "LicenseStateData.Print.redbook"
    g_wszWMDRM_LicenseState_CopyToNonSDMIDevice, // "LicenseStateData.Transfer.NONSDMI"
    g_wszWMDRM_LicenseState_CopyToSDMIDevice, // "LicenseStateData.Transfer.SDMI"
    g_wszWMDRM_LicenseState_CreateThumbnailImage, // "LicenseStateData.CreateThumbnailImage"
    g_wszWMDRM_LicenseState_Playback, // "LicenseStateData.Play"
    g_wszWMDRM_LicenseState_PlaylistBurn, // "LicenseStateData.PlaylistBurn"
    g_wszWMDRM_PRIORITY, // "PRIORITY"
    g_wszWMDRM_REVINFOVERSION, // "REVINFOVERSION"
    g_wszWMDRM_RIGHT_BACKUP, // "Backup"
    g_wszWMDRM_RIGHT_COLLABORATIVE_PLAY, // "CollaborativePlay"
    g_wszWMDRM_RIGHT_COPY, // "Copy"
    g_wszWMDRM_RIGHT_COPY_TO_CD, // "Print.redbook"
    g_wszWMDRM_RIGHT_COPY_TO_NON_SDMI_DEVICE, // "Transfer.NONSDMI"
    g_wszWMDRM_RIGHT_COPY_TO_SDMI_DEVICE, // "Transfer.SDMI"
    g_wszWMDRM_RIGHT_CREATE_THUMBNAIL_IMAGE, // "CreateThumbnailImage"
    g_wszWMDRM_RIGHT_PLAYBACK, // "Play"
    g_wszWMDRM_RIGHT_PLAYLIST_BURN, // "PlaylistBurn"
    g_wszWMDRM_SAPLEVEL, // "SAPLEVEL"
    g_wszWMDRM_SAPRequired, // "SAPRequired"
    g_wszWMDRM_SOURCEID, // "SOURCEID"
    g_wszWMDRM_UplinkID, // "UplinkID"
    g_wszWMDRMNET_Revocation, // "WMDRMNET_REVOCATION"
    g_wszWMDRM_ACTIONLIST_TAG, // "ACTIONLIST"
    g_wszWMDRM_ACTION_TAG, // "ACTION"
    g_wszWMDRM_ISSUEDATE, // "ISSUEDATE"
    g_wszWMUse_DRM, // "Use_DRM"
    g_wszWMUse_Advanced_DRM, // "Use_Advanced_DRM"
    g_wszWMDRM_Flags, // "DRM_Flags"
    g_wszWMDRM_HeaderSignPrivKey, // "DRM_HeaderSignPrivKey"
    g_wszWMDRM_KeySeed, // "DRM_KeySeed"
    g_wszWMDRM_Level, // "DRM_Level"
};

///////////////////////////////////////////////////////////////////////////////
DRMTypeValue::DRMTypeValue()
{
    m_Wmt = WMT_TYPE_DWORD;
    m_wValueLength = 0;
    m_pValue = NULL;
}

///////////////////////////////////////////////////////////////////////////////
DRMTypeValue::~DRMTypeValue()
{
    Cleanup();
}

///////////////////////////////////////////////////////////////////////////////
DRMTypeValue::DRMTypeValue(const DRMTypeValue &src)
{
    m_Wmt = src.m_Wmt;
    m_wValueLength = src.m_wValueLength;
    m_pValue = new (std::nothrow) BYTE[m_wValueLength];
    if (NULL != m_pValue)
    {
        memcpy(m_pValue, src.m_pValue, m_wValueLength);
    }
}

///////////////////////////////////////////////////////////////////////////////
void
DRMTypeValue::Cleanup()
{
    m_Wmt = WMT_TYPE_DWORD;
    m_wValueLength = 0;
    SAFE_ARRAY_DELETE(m_pValue);
}

///////////////////////////////////////////////////////////////////////////////
CDRMDumper::CDRMDumper()
{
    m_pEditor = NULL;
    m_pEditor2 = NULL;
    m_pDRMEditor = NULL;
}

///////////////////////////////////////////////////////////////////////////////
CDRMDumper::~CDRMDumper()
{
    Cleanup();
}

///////////////////////////////////////////////////////////////////////////////
HRESULT
CDRMDumper::Open(
    __in LPCWSTR pwszFileName)
{
    HRESULT hr = S_OK;

    CHECK_HR(hr = WMCreateEditor(&m_pEditor));
    CHECK_HR(hr = m_pEditor->QueryInterface(
        IID_IWMMetadataEditor2, 
        (void **)&m_pEditor2));
    CHECK_HR(hr = m_pEditor->QueryInterface(
        IID_IWMDRMEditor, 
        (void **)&m_pDRMEditor));
    CHECK_HR(hr = m_pEditor2->OpenEx(
        pwszFileName, 
        GENERIC_READ, 
        FILE_SHARE_READ));

done:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT
CDRMDumper::Close()
{
    HRESULT hr = S_OK;
    if (NULL != m_pEditor)
    {
        hr = m_pEditor->Close();
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT
CDRMDumper::Cleanup()
{
    HRESULT hr = S_OK;

    SAFE_RELEASE(m_pDRMEditor);
    SAFE_RELEASE(m_pEditor);
    SAFE_RELEASE(m_pEditor2);

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT
CDRMDumper::QueryProperty(
    __in LPCWSTR pwszPropertyName, 
    __out DRMTypeValue *pTypeValue)
{
    HRESULT hr = S_OK;

    pTypeValue->Cleanup();

    CHECK_HR(hr = m_pDRMEditor->GetDRMProperty(
        pwszPropertyName,
        &(pTypeValue->m_Wmt),
        NULL,
        &(pTypeValue->m_wValueLength)));

    pTypeValue->m_pValue = new BYTE[pTypeValue->m_wValueLength];
    if (NULL == pTypeValue->m_pValue)
    {
        CHECK_HR(hr = E_OUTOFMEMORY);
    }

    CHECK_HR(hr = m_pDRMEditor->GetDRMProperty(
        pwszPropertyName, 
        &(pTypeValue->m_Wmt), 
        pTypeValue->m_pValue, 
        &(pTypeValue->m_wValueLength)));

done:
    if (S_OK != hr)
    {
        pTypeValue->Cleanup();
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT
CDRMDumper::PrintProperty(
    __in LPCWSTR pwszPropertyName, 
    __in DRMTypeValue typeValue)
{
    HRESULT hr = S_OK;

    if (WMT_TYPE_STRING == typeValue.m_Wmt)
    {
        CDumperHelper::PrintColor(
            COLOR_DEFAULT, 
            L"        %ls: %ls\r\n", 
            pwszPropertyName, 
            (LPWSTR)(typeValue.m_pValue));
    }
    else if (WMT_TYPE_BOOL == typeValue.m_Wmt)
    {
        CDumperHelper::PrintColor(
            COLOR_DEFAULT, 
            L"        %ls: %ls\r\n", 
            pwszPropertyName, 
            (BOOL)(*(typeValue.m_pValue))==TRUE ? L"Yes" : L"No");
    }
    else if (WMT_TYPE_WORD == typeValue.m_Wmt)
    {
        CDumperHelper::PrintColor(
            COLOR_DEFAULT, 
            L"        %ls: %hu\r\n", 
            pwszPropertyName, 
            (WORD)(*(typeValue.m_pValue)));
    }
    else if (WMT_TYPE_DWORD == typeValue.m_Wmt)
    {
        CDumperHelper::PrintColor(
            COLOR_DEFAULT, 
            L"        %ls: %lu\r\n", 
            pwszPropertyName, 
            (DWORD)(*(typeValue.m_pValue)));
    }
    else if (WMT_TYPE_QWORD == typeValue.m_Wmt)
    {
        CDumperHelper::PrintColor(
            COLOR_DEFAULT, 
            L"        %ls: %llu\r\n", 
            pwszPropertyName, 
            (QWORD)(*(typeValue.m_pValue)));
    }
    else if (WMT_TYPE_GUID == typeValue.m_Wmt)
    {
        LPWSTR pwszGUID = NULL;
        CHECK_HR(hr = StringFromCLSID(
            *(LPGUID)(typeValue.m_pValue), 
            &pwszGUID));
        CDumperHelper::PrintColor(
            COLOR_DEFAULT, 
            L"        %ls: %ls\r\n", 
            pwszPropertyName, 
            pwszGUID);
      CoTaskMemFree(pwszGUID);
    }
    else if (WMT_TYPE_BINARY == typeValue.m_Wmt)
    {
        WCHAR pwszOutStr[MAX_LEN_MULTILINE] = L"";

        if (IsLicenseProperty(pwszPropertyName))
        {
            PrintLicenseStateData(
                pwszPropertyName, 
                (WM_LICENSE_STATE_DATA *)typeValue.m_pValue);
        }
        else
        {
            CDumperHelper::HexToString(
                typeValue.m_pValue, 
                typeValue.m_wValueLength, 
                pwszOutStr);
            CDumperHelper::PrintColor(
                COLOR_DEFAULT, 
                L"        %ls: %ls\r\n", 
                pwszPropertyName, 
                pwszOutStr);
        }
    }
    else
    {
        CDumperHelper::PrintColor(
            COLOR_DEFAULT, 
            L"        DRM property %ls type is unknown or mismatches",
            pwszPropertyName);
        CHECK_HR(hr = E_FAIL);
    }

done:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT
CDRMDumper::Dump(
    __in LPCWSTR pwszFilePath)
{
    HRESULT hr = S_OK;
    DRMTypeValue drmTypeValue;

    CDumperHelper::PrintColor(
        COLOR_LIGHTBLUE, 
        L"DRM Properties:\r\n");
    hr = Open(pwszFilePath);
    CHECK_HR(hr);
    for (int i = 0; i < ARRAYSIZE(g_DRMProperties); i++)
    {
        hr = QueryProperty(
            g_DRMProperties[i], 
            &drmTypeValue);
        if (SUCCEEDED(hr) && 
            (NULL != drmTypeValue.m_pValue))
        {
            PrintProperty(
                g_DRMProperties[i], 
                drmTypeValue);
        }
        SAFE_ARRAY_DELETE(drmTypeValue.m_pValue);

        // If a property is missing ignore the error and move on
        hr = S_OK;
    }
        
done:
    Close();

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
BOOL
CDRMDumper::IsLicenseProperty(
    __in LPCWSTR pwszPropertyName)
{
    BOOL fLicenseProperty = FALSE;

    if (0 == wcsncmp(
            pwszPropertyName, 
            L"LicenseStateData", 
            wcslen(L"LicenseStateData")))
    {
        fLicenseProperty = TRUE;
    }

    return fLicenseProperty;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT
CDRMDumper::PrintLicenseStateData(
    __in LPCWSTR pwszPropertyName, 
    __in WM_LICENSE_STATE_DATA *pWMLicenseStateData)
{
    HRESULT hr = S_OK;
    DRM_LICENSE_STATE_DATA *pDRMLicenseStateData = NULL;
    WCHAR pwszFileTime[MAX_LEN_ONELINE] = L"";

    CDumperHelper::PrintColor(
        COLOR_DEFAULT, 
        L"        %ls:\r\n", 
        pwszPropertyName);
    for (DWORD dwStateIndex = 0; dwStateIndex < pWMLicenseStateData->dwNumStates; dwStateIndex++)
    {
        pDRMLicenseStateData = &(pWMLicenseStateData->stateData[dwStateIndex]);
        CDumperHelper::PrintColor(
            COLOR_DEFAULT, 
            L"            DRM_LICENSE_DATA.dwStreamId: %ld\r\n", 
            pDRMLicenseStateData->dwStreamId);

        switch(pDRMLicenseStateData->dwCategory)
        {
            case WM_DRM_LICENSE_STATE_NORIGHT:
                CDumperHelper::PrintColor(
                    COLOR_DEFAULT, 
                    L"            DRM_LICENSE_DATA.dwCategory: WM_DRM_LICENSE_STATE_NORIGHT\r\n");
                break;
            case WM_DRM_LICENSE_STATE_UNLIM:
                CDumperHelper::PrintColor(
                    COLOR_DEFAULT, 
                    L"            DRM_LICENSE_DATA.dwCategory: WM_DRM_LICENSE_STATE_UNLIM\r\n");
                break;
            case WM_DRM_LICENSE_STATE_COUNT:
                CDumperHelper::PrintColor(
                    COLOR_DEFAULT, 
                    L"            DRM_LICENSE_DATA.dwCategory: WM_DRM_LICENSE_STATE_COUNT\r\n");
                break;
            case WM_DRM_LICENSE_STATE_FROM:
                CDumperHelper::PrintColor(
                    COLOR_DEFAULT, 
                    L"            DRM_LICENSE_DATA.dwCategory: WM_DRM_LICENSE_STATE_FROM\r\n");
                break;
            case WM_DRM_LICENSE_STATE_UNTIL:
                CDumperHelper::PrintColor(
                    COLOR_DEFAULT, 
                    L"            DRM_LICENSE_DATA.dwCategory: WM_DRM_LICENSE_STATE_UNTIL\r\n");
                break;
            case WM_DRM_LICENSE_STATE_FROM_UNTIL:
                CDumperHelper::PrintColor(
                    COLOR_DEFAULT, 
                    L"            DRM_LICENSE_DATA.dwCategory: WM_DRM_LICENSE_STATE_FROM_UNTIL\r\n");
                break;
            case WM_DRM_LICENSE_STATE_COUNT_FROM:
                CDumperHelper::PrintColor(
                    COLOR_DEFAULT, 
                    L"            DRM_LICENSE_DATA.dwCategory: WM_DRM_LICENSE_STATE_COUNT_FROM\r\n");
                break;
            case WM_DRM_LICENSE_STATE_COUNT_UNTIL:
                CDumperHelper::PrintColor(
                    COLOR_DEFAULT, 
                    L"            DRM_LICENSE_DATA.dwCategory: WM_DRM_LICENSE_STATE_COUNT_UNTIL\r\n");
                break;
            case WM_DRM_LICENSE_STATE_COUNT_FROM_UNTIL:
                CDumperHelper::PrintColor(
                    COLOR_DEFAULT, 
                    L"            DRM_LICENSE_DATA.dwCategory: WM_DRM_LICENSE_STATE_COUNT_FROM_UNTIL\r\n");
                break;
            case WM_DRM_LICENSE_STATE_EXPIRATION_AFTER_FIRSTUSE:
                CDumperHelper::PrintColor(
                    COLOR_DEFAULT, 
                    L"            DRM_LICENSE_DATA.dwCategory: WM_DRM_LICENSE_STATE_EXPIRATION_AFTER_FIRSTUSE\r\n");
                break;
            default:
                CDumperHelper::PrintColor(
                    COLOR_DEFAULT, 
                    L"            DRM_LICENSE_DATA.dwCategory: Unknown - %d\r\n", 
                    pDRMLicenseStateData->dwCategory);
        }

        if (0 != pDRMLicenseStateData->dwNumCounts)
        {
        CDumperHelper::PrintColor(
            COLOR_DEFAULT, 
            L"            DRM_LICENSE_DATA.dwCount: ");
            for (DWORD dwCountIndex = 0; dwCountIndex < pDRMLicenseStateData->dwNumCounts; dwCountIndex++)
            {
                CDumperHelper::PrintColor(
                    COLOR_DEFAULT, 
                    L"  %ld", 
                    pDRMLicenseStateData->dwCount[dwCountIndex]);
            }
            CDumperHelper::PrintColor(
                COLOR_DEFAULT, 
                L"\r\n");
        }

        if (0 != pDRMLicenseStateData->dwNumDates)
        {
            CDumperHelper::PrintColor(
                COLOR_DEFAULT, 
                L"            DRM_LICENSE_DATA.datetime: ");
            for (DWORD dwDateIndex = 0; dwDateIndex < pDRMLicenseStateData->dwNumDates; dwDateIndex++)
            {
                CDumperHelper::FileTimeToString(
                    &(pDRMLicenseStateData->datetime[dwDateIndex]), 
                    pwszFileTime);
                CDumperHelper::PrintColor(
                    COLOR_DEFAULT, 
                    L"  %ls", 
                    pwszFileTime);
            }
            CDumperHelper::PrintColor(
                COLOR_DEFAULT, 
                L"\r\n");
        }

        CDumperHelper::PrintColor(
            COLOR_DEFAULT, 
            L"            DRM_LICENSE_DATA.dwVague: %ld\r\n", 
            pDRMLicenseStateData->dwVague);
    }

    return hr;
}

