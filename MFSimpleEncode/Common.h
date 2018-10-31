// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE. 
// 
// Copyright (c) Microsoft Corporation. All rights reserved 
 
#pragma once 
 
#include <mfidl.h> 
#include <mfobjects.h> 
#include <mfapi.h> 
#include <wchar.h> 
#include <assert.h> 
#include <atlstr.h> 
 
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
 
/////////////////////////////////////////////////////////////////////////////// 
//  CAsyncCallback [template] 
// 
//  Helper class that routes IMFAsyncCallback::Invoke calls to a class method 
//  on the container class. 
// 
//  Add this class as a member variable. In the container class constructor, 
//  initialize the CAsyncCallback class like this: 
//  
//    m_cb(this, &CYourClass::OnInvoke) 
//     
//  where: 
// 
//    m_cb       = CAsyncCallback object. 
//    CYourClass = container class. 
//    OnInvoke   = Method to receive Invoke calls.  
// 
//  (Change these names as appropriate.) The signature of the OnInvoke method  
//  must matche the IMFAyncCallback::Invoke method. 
////////////////////////////////////////////////////////////////////////// 
 
// T: Type of the container class. 
template<class T> 
class CAsyncCallback : public IMFAsyncCallback 
{ 
public:  
    typedef HRESULT (T::*InvokeFn)(IMFAsyncResult *pAsyncResult); 
 
    CAsyncCallback(T *pParent, InvokeFn fn) : m_pParent(pParent), m_pInvokeFn(fn) 
    { 
    } 
 
    // IUnknown 
    STDMETHODIMP_(ULONG) AddRef() {  
        // Delegate to container class. 
        return m_pParent->AddRef();  
    } 
    STDMETHODIMP_(ULONG) Release() {  
        // Delegate to container class. 
        return m_pParent->Release();  
    } 
    STDMETHODIMP QueryInterface(REFIID iid, void** ppv) 
    { 
        if (!ppv) 
        { 
            return E_POINTER; 
        } 
        if (iid == __uuidof(IUnknown)) 
        { 
            *ppv = static_cast<IUnknown*>(static_cast<IMFAsyncCallback*>(this)); 
        } 
        else if (iid == __uuidof(IMFAsyncCallback)) 
        { 
            *ppv = static_cast<IMFAsyncCallback*>(this); 
        } 
        else 
        { 
            *ppv = NULL; 
            return E_NOINTERFACE; 
        } 
        AddRef(); 
        return S_OK; 
    } 
 
    // IMFAsyncCallback methods 
    STDMETHODIMP GetParameters(DWORD*, DWORD*) 
    { 
        // Implementation of this method is optional. 
        return E_NOTIMPL; 
    } 
 
    STDMETHODIMP Invoke(IMFAsyncResult* pAsyncResult) 
    { 
        return (m_pParent->*m_pInvokeFn)(pAsyncResult); 
    } 
 
    T *m_pParent; 
    InvokeFn m_pInvokeFn; 
}; 
 
enum TranscodeMode 
{ 
    TranscodeMode_Default, 
    TranscodeMode_SplitAudio, 
    TranscodeMode_SplitVideo, 
    TranscodeMode_Remux 
}; 
 
struct CEncodeConfig 
{ 
    CEncodeConfig() 
    { 
        enumTranscodeMode = TranscodeMode_Default; 
    }; 
     
    CAtlStringW strInputFile; 
    CAtlStringW strOutputFile; 
    CAtlStringW strProfile;     
    TranscodeMode enumTranscodeMode; 
    CAtlStringW strContainerName; 
}; 
 

