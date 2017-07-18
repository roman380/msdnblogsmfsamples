// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "stdafx.h"
#include "common.h"
#include "helper.h"
#include "mediatypedump.h"

///////////////////////////////////////////////////////////////////////////////
HRESULT
CMediaTypeDumper::PropVariantToString(
    __in PROPVARIANT varPropVal, 
    __in LPWSTR pwszPropName,  
    __out LPWSTR pwszPropVal)
{
    HRESULT hr = S_OK;

    if (VT_UI8 == varPropVal.vt)
    {
        MFRatio mfRatio = {(UINT32)(varPropVal.uhVal.QuadPart >> 32), (UINT32)(varPropVal.uhVal.QuadPart)};
        if (0 == wcscmp(L"MF_MT_FRAME_SIZE", pwszPropName))
        {
            StringCchPrintf(
                pwszPropVal, 
                MAX_LEN_ONELINE, 
                L"%lux%lu", 
                mfRatio.Numerator, 
                mfRatio.Denominator);
        }
        else if (0 == wcscmp(L"MF_MT_FRAME_RATE", pwszPropName))
        {
            if (0 != mfRatio.Denominator)
            {
                StringCchPrintf(
                    pwszPropVal, 
                    MAX_LEN_ONELINE, 
                    L"%3.5ffps", 
                    (float)mfRatio.Numerator / (float)mfRatio.Denominator);
            }
            else
            {
                StringCchPrintf(
                    pwszPropVal, 
                    MAX_LEN_ONELINE, 
                    L"%lu/%lu", 
                    mfRatio.Numerator, 
                    mfRatio.Denominator);
            }
        }
        else if (0 == wcscmp(L"MF_MT_PIXEL_ASPECT_RATIO", pwszPropName))
        {
            StringCchPrintf(
                pwszPropVal, 
                MAX_LEN_ONELINE, 
                L"%lu:%lu", 
                mfRatio.Numerator, 
                mfRatio.Denominator);
        }
        else if (0 == wcscmp(L"MF_PD_DURATION", pwszPropName) || 
            0 == wcscmp(L"MF_PD_ASF_FILEPROPERTIES_PLAY_DURATION", pwszPropName) || 
            0 == wcscmp(L"MF_PD_ASF_FILEPROPERTIES_SEND_DURATION", pwszPropName))
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
    else if (VT_CLSID == varPropVal.vt)
    {
        WCHAR pwszValGUID[MAX_LEN_ONELINE] = L"";
        CHECK_HR(hr = CDumperHelper::MFGUIDToString(
            *(varPropVal.puuid), 
            pwszValGUID));
        StringCchPrintf(
            pwszPropVal, 
            MAX_LEN_ONELINE, 
            L"%ls", 
            pwszValGUID);
    }
    else if ((VT_VECTOR | VT_UI1) == varPropVal.vt)
    {
        if (0 == wcscmp(L"MF_PD_ASF_FILEPROPERTIES_CREATION_TIME", pwszPropName) || 
            0 == wcscmp(L"MF_PD_LAST_MODIFIED_TIME", pwszPropName))
        {
            ULARGE_INTEGER ul = *(ULARGE_INTEGER *)(varPropVal.caub.pElems);
            FILETIME ft;
            ft.dwHighDateTime = ul.HighPart;
            ft.dwLowDateTime = ul.LowPart;
            CHECK_HR(hr = CDumperHelper::FileTimeToString(
                &ft, 
                pwszPropVal));
        }
        else
        {
            CHECK_HR(hr = CDumperHelper::HexToString(
                varPropVal, 
                pwszPropVal));
        }
    }
    else if (VT_UI4 == varPropVal.vt)
    {
        if (0 == wcscmp(L"MF_MT_INTERLACE_MODE", pwszPropName))
        {
            CHECK_HR(CDumperHelper::VideoInterlaceModeToString(
                (MFVideoInterlaceMode)varPropVal.ulVal, 
                pwszPropVal));
        }
        else if (0 == wcscmp(L"MF_MT_MPEG2_LEVEL", pwszPropName))
        {
            CHECK_HR(CDumperHelper::MPEG2LevelToString(
                (eAVEncH264VLevel)varPropVal.ulVal, 
                pwszPropVal));
        }
        else if (0 == wcscmp(L"MF_MT_MPEG2_PROFILE", pwszPropName))
        {
            CHECK_HR(CDumperHelper::MPEG2ProfileToString(
                (eAVEncH264VProfile)varPropVal.ulVal, 
                pwszPropVal));
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
CMediaTypeDumper::Dump(
    __in IMFMediaSource *pMediaSrc)
{
    HRESULT hr = S_OK;
    IMFPresentationDescriptor *pPD = NULL;
    DWORD cStreams = 0;
    DWORD cMTCount = 0;
    BOOL bSelected = FALSE;
    IMFStreamDescriptor *pSD = NULL;
    IMFMediaTypeHandler *pMTH = NULL;
    IMFMediaType *pMTWrapper = NULL;
    IMFMediaType *pMT = NULL;

    CHECK_HR(hr = pMediaSrc->CreatePresentationDescriptor(&pPD));

    if (NULL != pPD)
    {
        CDumperHelper::PrintColor(
            COLOR_LIGHTBLUE, 
            L"Container attributes:\r\n");
        CHECK_HR(hr = PrintMFAttributes(pPD));
        CHECK_HR(hr = pPD->GetStreamDescriptorCount(&cStreams));
        CDumperHelper::PrintColor(
            COLOR_LIGHTBLUE, 
            L"Media content has %d stream(s)\r\n", 
            cStreams);
        for (DWORD i = 0; i < cStreams; i++)
        {
            bSelected = FALSE;
            SAFE_RELEASE(pSD);
            SAFE_RELEASE(pMTH);
            CHECK_HR(hr = pPD->GetStreamDescriptorByIndex(
                i,
                &bSelected,
                &pSD));
            if (FALSE == bSelected)
            {
                CHECK_HR(hr = pPD->SelectStream(i));
            }
            CDumperHelper::PrintColor(
                COLOR_LIGHTBLUE, 
                L"    Stream #%d attributes:\r\n", 
                i);
            CHECK_HR(hr = PrintMFAttributes(pSD));
            CHECK_HR(hr = pSD->GetMediaTypeHandler(&pMTH));
            CHECK_HR(hr = pMTH->GetMediaTypeCount(&cMTCount));
            CDumperHelper::PrintColor(
                COLOR_LIGHTBLUE, 
                L"    Stream #%d has %d media type(s)\r\n", 
                i, 
                cMTCount);
            for (DWORD j = 0; j < cMTCount; j++)
            {
                GUID guidMajorType = GUID_NULL;
                SAFE_RELEASE(pMTWrapper);
                CHECK_HR(hr = pMTH->GetMediaTypeByIndex(j, &pMTWrapper));
                pMTWrapper->GetMajorType(&guidMajorType);

                // For protected content media type needs to be unwrapped to 
                // be readable
                if (MFMediaType_Protected == guidMajorType)
                {
                    CHECK_HR(MFUnwrapMediaType(pMTWrapper, &pMT));
                }
                else
                {
                    pMT = pMTWrapper;
                    SAFE_ADDREF(pMT);
                }

                CDumperHelper::PrintColor(
                    COLOR_LIGHTBLUE, 
                    L"    Media type #%d:\r\n", 
                    j);
                CHECK_HR(hr = PrintMFAttributes(pMT));
            }
        }
    }

done:
    SAFE_RELEASE(pPD);
    SAFE_RELEASE(pSD);
    SAFE_RELEASE(pMTH);
    SAFE_RELEASE(pMTWrapper);
    SAFE_RELEASE(pMT);

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT 
CMediaTypeDumper::PrintMFAttributes(
    __in IMFAttributes *pAttrs)
{
    HRESULT hr = S_OK;
    
    GUID itemGUID = GUID_NULL;
    PROPVARIANT varValue;
    WCHAR pwszAttrName[MAX_LEN_ONELINE] = L"";
    UINT32 cAttributes = 0;
    WCHAR pwszPropVal[MAX_LEN_MULTILINE] = L"";

    PropVariantInit(&varValue);

    CHECK_HR(hr = pAttrs->GetCount(&cAttributes));

    // Go through attributes
    for (DWORD i = 0; i < cAttributes; i++)
    {
        PropVariantClear(&varValue);
        if ('\0' != pwszAttrName[0])
        {
            pwszAttrName[0] = '\0';
        }
        if ('\0' != pwszPropVal[0])
        {
            pwszPropVal[0] = '\0';
        }
        hr = pAttrs->GetItemByIndex(i, 
            &itemGUID, 
            &varValue);
        if (S_OK != hr)
        {
            break;
        }

        hr = CDumperHelper::MFGUIDToString(
            itemGUID, 
            pwszAttrName);
        if (S_OK != hr)
        {
            break;
        }

        hr = PropVariantToString(
            varValue, 
            pwszAttrName, 
            pwszPropVal);
        if (S_OK != hr)
        {
            break;
        }
        CDumperHelper::PrintColor(
            COLOR_DEFAULT, 
            L"        %ls: %ls\r\n", 
            pwszAttrName, 
            pwszPropVal);

    }

done:
    PropVariantClear(&varValue);

    return hr;
}

