// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

struct DRMTypeValue
{
    DRMTypeValue();
    virtual ~DRMTypeValue();
    DRMTypeValue(
        const DRMTypeValue &src);
    void Cleanup();
    WMT_ATTR_DATATYPE m_Wmt;
    BYTE *m_pValue;
    WORD m_wValueLength;
};

class CDRMDumper
{
public:
    CDRMDumper();
    virtual ~CDRMDumper();

    HRESULT
    Dump(
        __in LPCWSTR pwszFilePath);

protected:
    HRESULT
    Open(
        __in LPCWSTR pwszFilePath);

    HRESULT
    Close();

    HRESULT
    QueryProperty(
        __in LPCWSTR pwszPropertyName, 
        __out DRMTypeValue *pTypeValue);

    HRESULT
    PrintProperty(
        __in LPCWSTR pwszPropertyName, 
        __in DRMTypeValue typeValue);

    HRESULT
    Cleanup();

    BOOL IsLicenseProperty(
        __in LPCWSTR pwszPropertyName);

    HRESULT
    PrintLicenseStateData(
        __in LPCWSTR pwszPropertyName, 
        __in WM_LICENSE_STATE_DATA *pWMLicenseStateData);

    IWMMetadataEditor *m_pEditor;
    IWMMetadataEditor2 *m_pEditor2;
    IWMDRMEditor *m_pDRMEditor;
};

