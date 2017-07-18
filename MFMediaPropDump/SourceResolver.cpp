// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "stdafx.h"
#include "common.h"
#include "helper.h"
#include "sourceresolver.h"

///////////////////////////////////////////////////////////////////////////////
HRESULT
CSourceResolver::CreateMediaSource(
    __in LPWSTR pwszFilePath, 
    __out IMFMediaSource **ppMediaSrc)
{
    HRESULT hr = S_OK;
    IUnknown *pUnk = NULL;
    IMFSourceResolver *pResolver = NULL;
    MF_OBJECT_TYPE ObjectType = MF_OBJECT_INVALID;

    *ppMediaSrc = NULL;
    CHECK_HR(hr = MFCreateSourceResolver(
        &pResolver));
    if (NULL != pResolver)
    {
        // File format may not match its extension so we ignore the extension
        CHECK_HR(hr = pResolver->CreateObjectFromURL(
            pwszFilePath, 
            MF_RESOLUTION_MEDIASOURCE | MF_RESOLUTION_READ | MF_RESOLUTION_CONTENT_DOES_NOT_HAVE_TO_MATCH_EXTENSION_OR_MIME_TYPE, 
            NULL, 
            &ObjectType, 
            &pUnk));
    }

    if (NULL != pUnk)
    {
        CHECK_HR(hr = pUnk->QueryInterface(
            IID_IMFMediaSource, 
            (void**)(ppMediaSrc)));
    }

done:
    SAFE_RELEASE(pResolver);
    SAFE_RELEASE(pUnk);

    return hr;
}

