// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE. 
// 
// Copyright (c) Microsoft Corporation. All rights reserved 
 
#pragma once 
 
#include <atlstr.h> 
#include "common.h" 
 
class CSourceManager 
{ 
public: 
    CSourceManager(); 
    ~CSourceManager(); 
     
    HRESULT Init(); 
    HRESULT Load( __in LPCWSTR pszFilePath ); 
    HRESULT GetSource( __out IMFMediaSource** ppSrc ); 
    void GetDuration( __out UINT64* phnsDuration ); 
    HRESULT GetPresentationDescriptor( __out IMFPresentationDescriptor** ppPD ); 
    HRESULT GetCharacteristics( __out DWORD* pdwCharacteristics );     
    void Reset(); 
 
private: 
    void ShutdownSource(); 
 
private: 
    IMFMediaSource* m_pSrc;  
    IMFPresentationDescriptor* m_pPD; 
    IMFSourceResolver* m_pMFSrcResolver; 
    UINT64 m_hnsDuration; 
}; 