// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "stdafx.h"
#include "common.h"
#include "helper.h"
#include "metadatadump.h"

///////////////////////////////////////////////////////////////////////////////
HRESULT
CMetadataDumper::PropVariantToString(
    __in PROPVARIANT varPropVal, 
    __in LPWSTR pwszPropName,  
    __out LPWSTR pwszPropVal)
{
    HRESULT hr = S_OK;

    if (VT_UI8 == varPropVal.vt)
    {
        if (0 == wcscmp(L"PKEY_Media_Duration", pwszPropName))
        {
            CHECK_HR(hr = CDumperHelper::TimeToString(
                varPropVal.uhVal, 
                pwszPropVal));
        }
        else
        {
            CHECK_HR(hr = CDumperHelper::PropVariantToString(
                varPropVal, 
                pwszPropVal));
        }
    }
    else if (VT_LPWSTR == varPropVal.vt)
    {
        if (0 == wcscmp(L"PKEY_Audio_Format", pwszPropName) || 
            (0 == wcscmp(L"PKEY_Video_Compression", pwszPropName)))
        {
            CHECK_HR(hr = CDumperHelper::MFGUIDToString(
                varPropVal.pwszVal, 
                pwszPropVal));
        }
        else
        {
            CHECK_HR(hr = CDumperHelper::PropVariantToString(
                varPropVal, 
                pwszPropVal));
        }
    }
    else if (VT_BLOB == varPropVal.vt)
    {
        if (0 == wcscmp(L"ASFLeakyBucketPairs", pwszPropName))
        {
            CHECK_HR(hr = CDumperHelper::HexToString(
                varPropVal, 
                pwszPropVal));
        }
        else
        {
            CHECK_HR(hr = CDumperHelper::PropVariantToString(
                varPropVal, 
                pwszPropVal));
        }
    }
    else if (VT_UI4 == varPropVal.vt)
    {
        if (0 == wcscmp(L"PKEY_Video_FourCC", pwszPropName))
        {
            BYTE *pPropVal = (BYTE *)(&varPropVal.ulVal);
            StringCchPrintf(
                pwszPropVal, 
                MAX_LEN_ONELINE, 
                L"%c%c%c%c", 
                pPropVal[0], 
                pPropVal[1], 
                pPropVal[2], 
                pPropVal[3]);
        }
        else
        {
            CHECK_HR(hr = CDumperHelper::PropVariantToString(
                varPropVal, 
                pwszPropVal));
        }
    }
    else
    {
        CHECK_HR(hr = CDumperHelper::PropVariantToString(
            varPropVal, 
            pwszPropVal));
    }

done:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT
CMetadataDumper::DumpMFMetadata(
    __in IMFMetadata *pMFMetadata)
{
    HRESULT hr = S_OK;
    PROPVARIANT varPropNames;
    LPWSTR pwszPropName = NULL;
    PROPVARIANT propVal;
    WCHAR pwszPropVal[MAX_LEN_MULTILINE];

    PropVariantInit(&propVal);
    PropVariantInit(&varPropNames);
    CHECK_HR(hr = pMFMetadata->GetAllPropertyNames(&varPropNames));
    for(DWORD dwPropIndex = 0; dwPropIndex< varPropNames.calpwstr.cElems; dwPropIndex++)
    {
        ZeroMemory(&pwszPropVal, sizeof(pwszPropVal));
        pwszPropName = varPropNames.calpwstr.pElems[dwPropIndex];
        hr = pMFMetadata->GetProperty(
            pwszPropName, 
            &propVal);
        if (S_OK != hr)
        {
            break;
        }
        hr = PropVariantToString(
            propVal, 
            pwszPropName, 
            pwszPropVal);
        if (S_OK != hr)
        {
            break;
        }
        CDumperHelper::PrintColor(
            COLOR_DEFAULT, 
            L"        %ls: %ls\r\n", 
            pwszPropName, 
            pwszPropVal);
        PropVariantClear(&propVal);
    }

done:
    PropVariantClear(&propVal);
    PropVariantClear(&varPropNames);

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT
CMetadataDumper::DumpShellMetadata(
    __in IPropertyStore *pShellMetadata)
{
    HRESULT hr = S_OK;
    DWORD dwPropCount = 0;
    PROPERTYKEY propKey;
    PROPVARIANT propVar;
    WCHAR pwszPropKey[MAX_LEN_ONELINE] = L"";
    WCHAR pwszPropVar[MAX_LEN_MULTILINE] = L"";

    PropVariantInit(&propVar);
    CDumperHelper::PrintColor(
        COLOR_LIGHTBLUE, 
        L"Shell Metadata:\r\n");
    CHECK_HR(hr = pShellMetadata->GetCount(&dwPropCount));
    for(DWORD dwPropIndex = 0; dwPropIndex < dwPropCount; dwPropIndex++)
    {            
        ZeroMemory(&pwszPropKey, sizeof(pwszPropKey));
        ZeroMemory(&pwszPropVar, sizeof(pwszPropVar));
        PropVariantClear(&propVar);

        hr = pShellMetadata->GetAt(
            dwPropIndex, 
            &propKey);
        if (S_OK != hr)
        {
            break;
        }
        hr = CDumperHelper::PKeyToString(
            propKey, 
            pwszPropKey);
        if (S_OK != hr)
        {
            break;
        }
        hr = pShellMetadata->GetValue(
            propKey, 
            &propVar);
        if (S_OK != hr)
        {
            break;
        }
        hr = PropVariantToString(
            propVar, 
            pwszPropKey, 
            pwszPropVar);
        if (S_OK != hr)
        {
            break;
        }
        CDumperHelper::PrintColor(
            COLOR_DEFAULT, 
            L"        %ls: %ls\r\n", 
            pwszPropKey, 
            pwszPropVar);
    }

done:
    PropVariantClear(&propVar);

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT
CMetadataDumper::Dump(
    __in IMFMediaSource *pMediaSrc)
{
    HRESULT hr = S_OK;
    IMFMetadataProvider *pMetaProvider = NULL;
    IMFGetService *pMetadataService = NULL;
    IMFPresentationDescriptor *pPD = NULL;
    IMFMetadata *pMFMetadata = NULL;
    IPropertyStore *pShellMetadata = NULL;
    DWORD cStreams = 0;
    DWORD dwStreamId = 0;
    BOOL bSelected = FALSE;
    IMFStreamDescriptor *pSD = NULL;

    CHECK_HR(hr = pMediaSrc->QueryInterface(IID_IMFGetService, 
        (void**)&pMetadataService));
    if (SUCCEEDED(pMetadataService->GetService(MF_METADATA_PROVIDER_SERVICE, 
        IID_IMFMetadataProvider, 
        (void**)&pMetaProvider)))
    {
        CHECK_HR(hr = pMediaSrc->CreatePresentationDescriptor(&pPD));
        CDumperHelper::PrintColor(
            COLOR_LIGHTBLUE, 
            L"MF Metadata:\r\n");
        CHECK_HR(hr = pPD->GetStreamDescriptorCount(&cStreams));
        
        // Dump file metadata
        CHECK_HR(hr = pMetaProvider->GetMFMetadata(pPD, 
            0, 
            0, 
            &pMFMetadata));
        CDumperHelper::PrintColor(
            COLOR_LIGHTBLUE, 
            L"    File metadata:\r\n");
        CHECK_HR(hr = DumpMFMetadata(pMFMetadata));
        SAFE_RELEASE(pMFMetadata);

        // Dump metadata for each stream
        for (DWORD i = 0; i < cStreams; i++)
        {
            bSelected = FALSE;
            SAFE_RELEASE(pSD);
            SAFE_RELEASE(pMFMetadata);
            CHECK_HR(hr = pPD->GetStreamDescriptorByIndex(
                i,
                &bSelected,
                &pSD));
            if (FALSE == bSelected)
            {
                CHECK_HR(hr = pPD->SelectStream(i));
            }
            CHECK_HR(hr = pSD->GetStreamIdentifier(&dwStreamId));
            CHECK_HR(hr = pMetaProvider->GetMFMetadata(pPD, 
                dwStreamId, 
                0, 
                &pMFMetadata));
            CDumperHelper::PrintColor(
                COLOR_LIGHTBLUE, 
                L"    Stream #%d metadata:\r\n", 
                i);
            CHECK_HR(hr = DumpMFMetadata(pMFMetadata));
        }
    }
    CHECK_HR(hr = pMetadataService->GetService(MF_PROPERTY_HANDLER_SERVICE, 
        IID_IPropertyStore, 
        (void**)&pShellMetadata));
    CHECK_HR(hr = DumpShellMetadata(pShellMetadata));

done:
    SAFE_RELEASE(pMetaProvider);
    SAFE_RELEASE(pMetadataService);
    SAFE_RELEASE(pPD);
    SAFE_RELEASE(pSD);
    SAFE_RELEASE(pMFMetadata);
    SAFE_RELEASE(pShellMetadata);
    return hr;
}

