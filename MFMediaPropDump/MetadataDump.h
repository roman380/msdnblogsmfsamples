// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

class CMetadataDumper
{
public:
    HRESULT
    Dump(
        __in IMFMediaSource *pMediaSrc);

    HRESULT
    DumpMFMetadata(
        __in IMFMetadata *pMFMetadata);

    HRESULT
    DumpShellMetadata(
        __in IPropertyStore *pShellMetadata);

protected:
    HRESULT PropVariantToString(
        __in PROPVARIANT varPropVal, 
        __in LPWSTR pwszPropName,  
        __out LPWSTR pwszPropVal);
};

