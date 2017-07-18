// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

#include <windows.h>

#include <stdio.h>
#include <strsafe.h>
#include <intsafe.h>

#include <propvarutil.h>
#include <shobjidl.h>

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>

#include <wmcodecdsp.h>

#ifndef SAFE_RELEASE
template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}
#define SAFE_RELEASE(p) { SafeRelease(&(p)); }
#endif 

#ifndef SAFE_ADDREF
template <class T> void SafeAddRef(T *pT)
{
    if (pT)
    {
        (pT)->AddRef();
    }
}
#define SAFE_ADDREF(p) { SafeAddRef(p); }
#endif 

#ifndef SAFE_DELETE
#define SAFE_DELETE(p) { delete (p); (p) = NULL; }
#endif 

#ifndef SAFE_ARRAY_DELETE
#define SAFE_ARRAY_DELETE(p) { delete [] (p); (p) = NULL; }
#endif


#ifndef CHECK_HR
#define CHECK_HR(val) { if ( (val) != S_OK ) { goto done; } }
#endif

