// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

#include <atlstr.h>

struct ContainerNameToGUIDMapEntry
{
    LPCWSTR szContainerName;
    GUID guidContainerType;
};

class CProfileBuilder
{
public:
    CProfileBuilder( );
    ~CProfileBuilder();

    HRESULT Load( __in LPCWSTR pszFilePath );
    HRESULT CreateProfile( __in IMFPresentationDescriptor* pPD, TranscodeMode enumTranscodeMode, __in LPCWSTR strContainerName );
    HRESULT GetProfile( __out IMFTranscodeProfile** ppProfile );
    void Reset();

private:
    HRESULT LoadMFProfile( __in LPCWSTR pszFilePath );
    HRESULT LoadXML( __in LPCWSTR pszFilePath, __out IXMLDOMElement **ppElemRoot);
    HRESULT CreateTranscodeProfile( __in IXMLDOMElement *pElemRoot, __out IMFTranscodeProfile **ppProfile );
    HRESULT CreateMediaType( __in IXMLDOMElement *pElemRoot, __out IMFMediaType **ppMT );
    HRESULT AddAttribute(__in IXMLDOMElement *pElem, __inout IMFAttributes *pAttrib );
    HRESULT CLSIDFromBSTR( __in const CComBSTR &bstr, __out GUID *pguid );
    HRESULT CreateRemuxProfile( __in IMFPresentationDescriptor* pPD, __in REFGUID guidMajorType, __out IMFAttributes** ppAttrs );
    HRESULT GetSourceStreamMediaType( __in IMFPresentationDescriptor* pPD, __in REFGUID guidMajorType, __out IMFMediaType** ppMT );
    HRESULT MapContainerNameToGUID( __in LPCWSTR strContainerName, __out GUID* pGuid );

private:
    IMFTranscodeProfile* m_pProfile;
};
