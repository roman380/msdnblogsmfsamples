// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

#ifndef MAX_LEN_ONELINE
#define MAX_LEN_ONELINE 80
#endif

#ifndef MAX_LEN_MULTILINE
#define MAX_LEN_MULTILINE 8192
#endif

#ifndef MAX_DATETIME_LEN
#define MAX_DATETIME_LEN 255
#endif

#ifndef COLOR_LIGHTBLUE
#define COLOR_LIGHTBLUE \
            FOREGROUND_INTENSITY | \
            FOREGROUND_BLUE | \
            FOREGROUND_GREEN
#endif

#ifndef COLOR_RED
#define COLOR_RED       \
            FOREGROUND_INTENSITY | \
            FOREGROUND_RED
#endif

#ifndef COLOR_DEFAULT
#define COLOR_DEFAULT   \
            FOREGROUND_BLUE | \
            FOREGROUND_GREEN | \
            FOREGROUND_RED
#endif

C_ASSERT(MAX_LEN_MULTILINE > MAX_LEN_ONELINE);

class CDumperHelper
{
public:
    static HRESULT
    PropVariantToString(
        __in PROPVARIANT varPropVal, 
        __out LPWSTR pwszPropVal);

    static HRESULT
    MFGUIDToString(
        __in GUID guid, 
        __out LPWSTR pwszName);

    static HRESULT
    MFGUIDToString(
        __in LPWSTR pwszGUID, 
        __out LPWSTR pwszName);

    static HRESULT
    VideoInterlaceModeToString(
        __in MFVideoInterlaceMode mfVIM, 
        __out LPWSTR pwszName);

    static HRESULT
    MPEG2LevelToString(
        __in eAVEncH264VLevel h264VLevel, 
        __out LPWSTR pwszName);

    static HRESULT
    MPEG2ProfileToString(
        __in eAVEncH264VProfile h264VProfile, 
        __out LPWSTR pwszName);

    static HRESULT
    PKeyToString(
        __in PROPERTYKEY pkey, 
        __out LPWSTR pwszName);

    static HRESULT
    FileTimeToString(
        __in FILETIME *ft, 
        __out LPWSTR pwszPropVal);

    static HRESULT
    TimeToString(
        __in ULARGE_INTEGER time, 
        __out LPWSTR pwszPropVal);

    static HRESULT
    HexToString(
        __in PROPVARIANT varPropVal, 
        __out LPWSTR pwszPropVal);

    static HRESULT
    HexToString(
        __in BYTE *pBuf, 
        __in DWORD cBuf, 
        __out LPWSTR pwszPropVal);

    static HRESULT
    PrintColor(
        __in WORD theColor, 
        __in WCHAR *pwsz, 
        __in ...);

};
