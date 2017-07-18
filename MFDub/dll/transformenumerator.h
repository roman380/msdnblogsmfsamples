#pragma once

STDAPI MFVEMFTGetName(CLSID clsidMFT, LPWSTR* ppszName);
STDAPI MFVEMFTEnum(CLSID** ppclsidMFT, UINT32* pcMFTs);
STDAPI MFVEMFTRegister(CLSID clsidMFT, LPWSTR pszName);
STDAPI MFVEMFTUnregister(CLSID clsidMFT);
