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
#include "metadatadump.h"
#include "drmdump.h"
#include "mediapropdump.h"
#include "sourceresolver.h"

///////////////////////////////////////////////////////////////////////////////
HRESULT
CMediaPropDumper::Dump(
    __out LPWSTR pwszFilePath)
{
    HRESULT hr = S_OK;
    IMFMediaSource *pMediaSrc = NULL;
    BOOL bIsDRM = FALSE;
    CMediaTypeDumper mediaTypeDumper;
    CMetadataDumper metadataDumper;
    CDRMDumper drmDumper;

    CHECK_HR(hr = CSourceResolver::CreateMediaSource(
        pwszFilePath, 
        &pMediaSrc));

    CHECK_HR(hr = mediaTypeDumper.Dump(pMediaSrc));

    // Ignoring metadata dump failures
    metadataDumper.Dump(pMediaSrc);

    // WMIsContentProtected() in some cases when it can't determine whether 
    // the content is DRM returns failing hr but sets bIsDRM to TRUE. We treat 
    // such files as NOT DRM. That means that we don't dump DRM info for 
    // them. Also we don't propagate that failing hr for such cases.
    hr = WMIsContentProtected(
        pwszFilePath, 
        &bIsDRM);
    if (S_OK != hr)
    {
        bIsDRM = FALSE;
        hr = S_OK;
    }
    else if (TRUE == bIsDRM)
    {
        // Ignoring DRM dump failures
        drmDumper.Dump(pwszFilePath);
    }

done:
    if (NULL != pMediaSrc)
    {
        pMediaSrc->Shutdown();
    }
    SAFE_RELEASE(pMediaSrc);
    return hr;
}
