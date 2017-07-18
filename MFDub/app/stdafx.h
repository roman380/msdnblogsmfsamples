// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#ifndef STRICT
#define STRICT
#endif

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows 95 and Windows NT 4 or later.
#define WINVER  _WIN32_WINNT_WIN7		// Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows NT 4 or later.
#define _WIN32_WINNT  _WIN32_WINNT_WIN7	// Change this to the appropriate value to target Windows 2000 or later.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 4.0 or later.
#define _WIN32_IE 0x0500	// Change this to the appropriate value to target IE 5.0 or later.
#endif

#define _ATL_APARTMENT_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

// turns off ATL's hiding of some common and often safely ignored warning messages
#define _ATL_ALL_WARNINGS

#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <atltypes.h>
#include <atlctl.h>
#include <atlhost.h>
#include <atlstr.h>
#include <atlcoll.h>

#include <wingdi.h>

#include <mfidl.h>
#include <mfapi.h>
#include <mftransform.h>
#include <mfreadwrite.h>

#include "common.h"

///////////////////////////////////////////////////////////////////////////////
//

using namespace ATL;

#define METHODASYNCCALLBACKEX(Callback, Parent, Flag, Queue) \
class Callback##AsyncCallback; \
friend class Callback##AsyncCallback; \
class Callback##AsyncCallback : public IMFAsyncCallback \
{ \
public: \
    STDMETHOD_( ULONG, AddRef )() \
    { \
        Parent * pThis = ((Parent*)((BYTE*)this - offsetof(Parent, m_x##Callback))); \
        return pThis->AddRef(); \
    } \
    STDMETHOD_( ULONG, Release )() \
    { \
        Parent * pThis = ((Parent*)((BYTE*)this - offsetof(Parent, m_x##Callback))); \
        return pThis->Release(); \
    } \
    STDMETHOD( QueryInterface )( REFIID riid, void **ppvObject ) \
    { \
        if(riid == IID_IMFAsyncCallback || riid == IID_IUnknown) \
        { \
            (*ppvObject) = this; \
            AddRef(); \
            return S_OK; \
        } \
        (*ppvObject) = NULL; \
        return E_NOINTERFACE; \
    } \
    STDMETHOD( GetParameters )( \
        DWORD *pdwFlags, \
        DWORD *pdwQueue) \
    { \
        *pdwFlags = Flag; \
        *pdwQueue = Queue; \
        return S_OK; \
    } \
    STDMETHOD( Invoke )( IMFAsyncResult * pResult ) \
    { \
        Parent * pThis = ((Parent*)((BYTE*)this - offsetof(Parent, m_x##Callback))); \
        pThis->Callback( pResult ); \
        return S_OK; \
    } \
} m_x##Callback; 

////////////////////////////////////////////////////////
//
   
#define METHODASYNCCALLBACK(Callback, Parent) \
    METHODASYNCCALLBACKEX(Callback, Parent, 0, MFASYNC_CALLBACK_QUEUE_STANDARD)
