// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE
//
// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "stdafx.h"

#include "mfveutil.h"
#include "resource.h"
#include "edgefindermft.h"
#include "rgbmft.h"
#include "pcmmft.h"

#include <initguid.h>
#include "mfveapi.h"

class CMfveModule : public CAtlDllModuleT<CMfveModule>
{
public:
    DECLARE_LIBID(LIBID_MFVEUTIL);
    DECLARE_REGISTRY_APPID_RESOURCEID(IDR_MFVE, "{D7A51D0D-15E7-4c26-B6FA-2FCA3BE26642}");
// Override CAtlDllModuleT members
};

CMfveModule _Module;

OBJECT_ENTRY_AUTO(CLSID_CEdgeFinderMFT, CEdgeFinderMFT)
OBJECT_ENTRY_AUTO(CLSID_CHistogramEqualizationMFT, CHistogramEqualizationMFT)
OBJECT_ENTRY_AUTO(CLSID_CNoiseRemovalMFT, CNoiseRemovalMFT)
OBJECT_ENTRY_AUTO(CLSID_CUnsharpMaskMFT, CUnsharpMaskMFT)
OBJECT_ENTRY_AUTO(CLSID_CResizeCropMFT, CResizeCropMFT)
OBJECT_ENTRY_AUTO(CLSID_CVolumeCompressionMFT, CVolumeCompressionMFT)

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{

    return _Module.DllMain(dwReason, lpReserved);
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow()
{
    return _Module.DllCanUnloadNow();
}


STDAPI DllRegisterServer(void)
{
    MFVEMFTRegister(CLSID_CEdgeFinderMFT, MFVE_CATEGORY_VIDEO, L"Edge Finder", L"Locates edges in the display image");
    MFVEMFTRegister(CLSID_CHistogramEqualizationMFT, MFVE_CATEGORY_VIDEO, L"Histogram Equalization", L"Equalizes the histogram in the image, increasing the contrast");
    MFVEMFTRegister(CLSID_CNoiseRemovalMFT, MFVE_CATEGORY_VIDEO, L"Noise Removal", L"Removes noise, such as speckles, from the image.");
    MFVEMFTRegister(CLSID_CUnsharpMaskMFT, MFVE_CATEGORY_VIDEO, L"Unsharp Mask", L"The unsharp mask can either sharpen or blur an image.  With a gamma parameter of > 1, the image will be sharpened.  With a gamma parameter of < 1, the image will be blurred.");
    MFVEMFTRegister(CLSID_CResizeCropMFT, MFVE_CATEGORY_VIDEO, L"Resize & Crop", L"Resize and crop the output video");

    MFVEMFTRegister(CLSID_CVolumeCompressionMFT, MFVE_CATEGORY_AUDIO, L"Volume Compression", L"Raise or lower the volume of the output audio");
    
    return _Module.DllRegisterServer();
}

STDAPI DllUnregisterServer(void)
{
    MFVEMFTUnregister(CLSID_CEdgeFinderMFT);
    MFVEMFTUnregister(CLSID_CHistogramEqualizationMFT);
    MFVEMFTUnregister(CLSID_CNoiseRemovalMFT);
    MFVEMFTUnregister(CLSID_CUnsharpMaskMFT);
    MFVEMFTUnregister(CLSID_CResizeCropMFT);

    MFVEMFTUnregister(CLSID_CVolumeCompressionMFT);
    
    return _Module.DllUnregisterServer();
}

STDAPI DllGetClassObject(
  const CLSID & rclsid,  
  const IID & riid,      
  void ** ppv
)
{
    return _Module.DllGetClassObject(rclsid, riid, ppv);
}


