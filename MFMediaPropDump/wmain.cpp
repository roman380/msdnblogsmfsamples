// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "stdafx.h"
#include "common.h"
#include "mffriendlyerrors.h"
#include "helper.h"
#include "mediapropdump.h"
#include "wmain.h"

///////////////////////////////////////////////////////////////////////////////
HRESULT ParseParams(
    __in int iArgCount,
    __in_ecount_opt(iArgCount) WCHAR * awszArgs[],
    __out LPWSTR pwszFilePath)
{
    HRESULT hr = S_OK;
    int i = 1;

    if (iArgCount < 2 || 
        iArgCount > 3)
    {
        PrintHelp();
		CHECK_HR(hr = E_INVALIDARG);
    }

    while (i < iArgCount)
    {
        if (0 == wcscmp(L"-f", awszArgs[i]) || 
            0 == wcscmp(L"/f", awszArgs[i]))
        {
            // Just to make sure "-f" or "/f" is not the last argument
            if (iArgCount < i + 2)
            {
                PrintHelp();
                CHECK_HR(hr = E_INVALIDARG);
            }

            CHECK_HR(hr = StringCchCopy(
                pwszFilePath, 
                MAX_PATH, 
                awszArgs[i + 1]));
            i += 2;
        }
        else if (0 == wcscmp(L"-h", awszArgs[i]) || 
            0 == wcscmp(L"/h", awszArgs[i]) || 
            0 == wcscmp(L"-?", awszArgs[i]) || 
            0 == wcscmp(L"/?", awszArgs[i]))
        {
            PrintHelp();
            goto done;
        }
        else
        {
            PrintHelp();
            CHECK_HR(hr = E_INVALIDARG);
        }
    }

done:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
void PrintHelp()
{
    CDumperHelper::PrintColor(
        COLOR_DEFAULT, 
        L"\r\n"
        L"Tool dumping media file properties.\r\n"
        L"Usage:\r\n\r\n"
        L"    MediaPropDump.exe [-?]\r\n"
        L"                      -f file\r\n"
        L"Options:\r\n"
        L"    -f : path to a file with any format supported by Media Foundation\r\n");
}

///////////////////////////////////////////////////////////////////////////////
int __cdecl wmain(
    __in int iArgCount,
    __in_ecount_opt(iArgCount) WCHAR * awszArgs[])
{
    HRESULT hr = S_OK;
    int iRet = 0;
    WCHAR pwszFilePath[MAX_PATH] = L"";
    CMediaPropDumper *pMediaPropDumper = NULL;
    BOOL bCoInit = FALSE;
    BOOL bMFInit = FALSE;

    HeapSetInformation(
        NULL, 
        HeapEnableTerminationOnCorruption, 
        NULL, 
        0);

    CHECK_HR(hr = ParseParams(
        iArgCount, 
        awszArgs, 
        pwszFilePath));

    CHECK_HR(hr = CoInitializeEx(
        NULL, 
        COINIT_APARTMENTTHREADED));
    bCoInit = TRUE;

    CHECK_HR(hr = MFStartup(MF_VERSION));
    bMFInit = TRUE;

    pMediaPropDumper = new (std::nothrow) CMediaPropDumper;
    if (NULL == pMediaPropDumper)
    {
        CHECK_HR(hr = E_OUTOFMEMORY);
    }

    CHECK_HR(hr = pMediaPropDumper->Dump(pwszFilePath));

done:
    if (S_OK != hr)
    {
        LPCSTR psz = NULL;
        MFFriendlyConvertHRESULT(
            hr, 
            &psz);
        CDumperHelper::PrintColor(
            COLOR_RED, 
            L"MediaPropDump.exe failed, hr=0x%08X, %S\r\n", 
            hr, 
            psz);
        iRet = -1;
    }

    SAFE_DELETE(pMediaPropDumper);

    if (TRUE == bMFInit)
    {
        MFShutdown();
    }

    if (TRUE == bCoInit)
    {
        CoUninitialize();
    }

    return iRet;
};

