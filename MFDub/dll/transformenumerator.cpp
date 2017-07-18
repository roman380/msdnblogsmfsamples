// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE
//
// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "stdafx.h"

//
// make sure that HRESULT_FROM_WIN32 is the inline version, 
// because the macro version evaluates the parameter multiple times
//
#define INLINE_HRESULT_FROM_WIN32

#undef HIDWORD
#include <intsafe.h>
#include <strsafe.h>

#define GUID_STRING_LENGTH  36
#define GUID_BUFFER_SIZE    37

#define MFT_REGISTRY_ROOT   L"MFVE\\Transforms"
#define DESC_STR      L"Description"
#define CATEGORIES_STR      L"Categories"

#define MAX_KEY_NAME_LENGTH     255
#define MAX_KEY_NAME_BUFSIZE    256

#define MAX_MFT_NAME_LENGTH     80
#define MAX_MFT_DESC_LENGTH     1024

//
// we have our own HRESULT_FROM_WIN32 to avoid problems with the macro version; we want to be absolutely
// sure we get the inline version
//
static 
inline
HRESULT 
_HR_FROM_W32(long x) 
{ 
    return x <= 0 ? (HRESULT)x : (HRESULT) (((x) & 0x0000FFFF) | (FACILITY_WIN32 << 16) | 0x80000000);
}

#define MACRO_HR_FROM_W32(x) ((HRESULT)(x) <= 0 ? ((HRESULT)(x)) : ((HRESULT) (((x) & 0x0000FFFF) | (FACILITY_WIN32 << 16) | 0x80000000)))


#define _HR_NO_MORE_ITEMS   MACRO_HR_FROM_W32(ERROR_NO_MORE_ITEMS)
#define _HR_KEY_NOT_FOUND   MACRO_HR_FROM_W32(ERROR_FILE_NOT_FOUND)
#define _HR_MORE_DATA       MACRO_HR_FROM_W32(ERROR_MORE_DATA)

static
LPWSTR
StringFromGUID(
    __in                                GUID    guidIn,
    __out_ecount(GUID_BUFFER_SIZE)      LPWSTR  szOut
    )
{
   (void)StringCchPrintfW(szOut, 
                          GUID_BUFFER_SIZE, 
                          L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                          guidIn.Data1, 
                          guidIn.Data2, 
                          guidIn.Data3, 
                          guidIn.Data4[0], 
                          guidIn.Data4[1],
                          guidIn.Data4[2], 
                          guidIn.Data4[3], 
                          guidIn.Data4[4], 
                          guidIn.Data4[5],
                          guidIn.Data4[6], 
                          guidIn.Data4[7]
                          );

   return szOut;
}

//
// we need to use swscanf, as swscanf_s isn't available downleve
//
#pragma warning(push)
#pragma warning(disable: 4995)
static
HRESULT
GUIDFromString(
    __in_ecount(GUID_BUFFER_SIZE)   LPWSTR  szIn,
    __out                           GUID*   pguidOut
    )
{
    UINT32  aunTemp[8];
    int     nRet = 0;
    HRESULT hr = S_OK;

    //
    // the byte-sized params must be copied into UINT32s because sscanf doesn't
    // have a hex byte format specifier
    //
    nRet = swscanf_s(szIn, 
                   L"%08x-%04hx-%04hx-%02x%02x-%02x%02x%02x%02x%02x%02x",
                   &pguidOut->Data1, 
                   &pguidOut->Data2, 
                   &pguidOut->Data3,
                   &aunTemp[0], 
                   &aunTemp[1], 
                   &aunTemp[2], 
                   &aunTemp[3],
                   &aunTemp[4], 
                   &aunTemp[5], 
                   &aunTemp[6], 
                   &aunTemp[7]);
    if (nRet != 11) 
    {
        hr = E_FAIL;
        goto out;
    }

    //
    // copy the temp values into the GUID
    //
    for (UINT32 i = 0; i < 8; ++i)
    {
        pguidOut->Data4[i] = BYTE(aunTemp[i]);
    }

out:
    if (FAILED(hr))
    {
        *pguidOut = GUID_NULL;
    }

    return hr;
}
#pragma warning(pop)

HRESULT OpenRootKey(
    __out   HKEY*   phkey,
    __in    REGSAM  rights = KEY_READ
    )
{
    return _HR_FROM_W32(RegOpenKeyExW(HKEY_CLASSES_ROOT, MFT_REGISTRY_ROOT, 0, rights, phkey));
}

HRESULT OpenCategoryRootKey(
    __out   HKEY*   phkey,
    __in    REGSAM  rights = KEY_READ
    )
{
    HRESULT hr = S_OK;
    HKEY    hkeyRoot = NULL;

    hr = OpenRootKey(&hkeyRoot);
    if (FAILED(hr))
    {
        goto out;
    }

    hr = _HR_FROM_W32(RegOpenKeyExW(hkeyRoot, CATEGORIES_STR, 0, KEY_READ, phkey));
    if (FAILED(hr))
    {
        goto out;
    }

out:
    if (hkeyRoot != NULL)
    {
        (void)RegCloseKey(hkeyRoot);
    }

    return hr;
}

HRESULT OpenCategoryKeyByIndex(
    __in    UINT32  unIndex,
    __out   HKEY*   phkey,
    __in    REGSAM  rights = KEY_READ
    )
{
    HRESULT hr = S_OK;
    WCHAR   szGUID[GUID_BUFFER_SIZE];
    HKEY    hkeyRoot = NULL;
    UINT32  cchName = GUID_BUFFER_SIZE;
    UINT32  cSubkeys = 0;
    

    hr = OpenCategoryRootKey(&hkeyRoot, rights);
    if (FAILED(hr)) 
    {
        goto out;
    }

    hr = _HR_FROM_W32(RegQueryInfoKeyW(hkeyRoot, NULL, NULL, NULL, (DWORD*)&cSubkeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    if (FAILED(hr)) 
    {
        goto out;
    }

    //
    // make sure they aren't asking for a non-existent index
    //
    if (cSubkeys >= unIndex) 
    {
        hr = _HR_NO_MORE_ITEMS;
        goto out;
    }

    hr = _HR_FROM_W32(RegEnumKeyExW(hkeyRoot, unIndex, szGUID, (DWORD*)&cchName, NULL, NULL, NULL, NULL));
    if (FAILED(hr)) 
    {
        goto out;
    }
    if (cchName != GUID_STRING_LENGTH) 
    {
        //
        // someone put something that isn't a GUID in the category section
        //
        hr = E_UNEXPECTED;
        goto out;
    }

    hr = _HR_FROM_W32(RegOpenKeyExW(hkeyRoot, szGUID, 0, rights, phkey));
    if (FAILED(hr)) 
    {
        goto out;
    }

out:
    if (hkeyRoot != NULL) 
    {
        (void)RegCloseKey(hkeyRoot);
    }

    return hr;
}

HRESULT DeleteKeyAndSubkeys(
    __in    HKEY    hkeyRoot,
    __in    LPCWSTR szSubkey
    )
{
    HKEY    hkeySub = NULL;
    HRESULT hr = S_OK;

    //
    // open the subkey so we can enumerate any subkeys below it
    //
    hr = _HR_FROM_W32(RegOpenKeyExW(hkeyRoot, szSubkey, 0, KEY_READ | KEY_WRITE | DELETE, &hkeySub));
    if (FAILED(hr)) 
    {
        hkeySub = NULL;

        //
        // if there's no key to begin with, it's fine - call it deleted and get out
        //
        if (hr == _HR_KEY_NOT_FOUND) {
            hr = S_OK;
        }
        goto out;
    }

    //
    // enumerate the subkeys, if any, and delete them
    //
    while(1) {
        WCHAR   szName[MAX_KEY_NAME_BUFSIZE];
        DWORD   cchName = MAX_KEY_NAME_BUFSIZE;

        //
        // we always enumerate the 0th subkey, since it gets deleted and the next one is now the 0th
        //
        hr = _HR_FROM_W32(RegEnumKeyExW(hkeySub, 0, szName, &cchName, NULL, NULL, NULL, NULL));
        if (hr == _HR_NO_MORE_ITEMS)
        {
            hr = S_OK;
            break;
        }
        if (FAILED(hr)) 
        {
            goto out;
        }

        //
        // recurse and delete this one
        //
        hr = DeleteKeyAndSubkeys(hkeySub, szName);
        if (FAILED(hr))
        {
            goto out;
        }
    }

    (void)RegCloseKey(hkeySub);
    hkeySub = NULL;

    hr = _HR_FROM_W32(RegDeleteKeyW(hkeyRoot, szSubkey));
    if (FAILED(hr))
    {
        goto out;
    }

out:
    if (hkeySub != NULL)
    {
        (void)RegCloseKey(hkeySub);
    }

    return hr;
}

HRESULT CreateRootKey(
    __out   HKEY*   phkey
    )
{
    return _HR_FROM_W32(RegCreateKeyExW(HKEY_CLASSES_ROOT, MFT_REGISTRY_ROOT, 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, phkey, NULL));
}

HRESULT CreateCategoryRootKey(
    __out   HKEY*   phkey
    )
{
    HRESULT hr = S_OK;
    HKEY    hkeyRoot = NULL;

    hr = CreateRootKey(&hkeyRoot);
    if (FAILED(hr)) 
    {
        goto out;
    }

    hr = _HR_FROM_W32(RegCreateKeyExW(hkeyRoot, CATEGORIES_STR, 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, phkey, NULL));
    if (FAILED(hr)) 
    {
        goto out;
    }

out:
    if (hkeyRoot != NULL) 
    {
        (void)RegCloseKey(hkeyRoot);
    }

    return hr;
}

HRESULT OpenMFTKey(
    __in    CLSID   clsidMFT,
    __out   HKEY*   phkey
    )
{
    HRESULT hr = S_OK;
    WCHAR   szGUID[GUID_BUFFER_SIZE];
    HKEY    hkeyRoot = NULL;

    hr = OpenRootKey(&hkeyRoot);
    if (FAILED(hr)) 
    {
        goto out;
    }

    hr = _HR_FROM_W32(RegOpenKeyExW(hkeyRoot, StringFromGUID(clsidMFT, szGUID), 0, KEY_READ, phkey));
    if (FAILED(hr))
    {
        goto out;
    }

out:
    if (hkeyRoot != NULL)
    {
        (void)RegCloseKey(hkeyRoot);
    }
    return hr;
}

HRESULT CreateMFTKey(
    __in    CLSID   clsidMFT,
    __out   HKEY*   phkey
    )
{
    HRESULT hr = S_OK;
    WCHAR   szGUID[GUID_BUFFER_SIZE];
    HKEY    hkeyRoot = NULL;

    hr = CreateRootKey(&hkeyRoot);
    if (FAILED(hr)) 
    {
        goto out;
    }

    hr = _HR_FROM_W32(RegCreateKeyExW(hkeyRoot, StringFromGUID(clsidMFT, szGUID), 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, phkey, NULL));
    if (FAILED(hr)) 
    {
        goto out;
    }

out:
    if (hkeyRoot != NULL) 
    {
        (void)RegCloseKey(hkeyRoot);
    }

    return hr;
}

HRESULT DeleteMFTKey(
    __in    CLSID   clsidMFT
    )
{
    HRESULT hr = S_OK;
    HRESULT hrFinal = S_OK;
    WCHAR   szGUID[GUID_BUFFER_SIZE];
    HKEY    hkeyRoot = NULL;
    DWORD   dwIndex = 0;

    hr = OpenRootKey(&hkeyRoot, KEY_READ | KEY_WRITE | DELETE);
    if (FAILED(hr)) 
    {

        //
        // if there's no root key, there are clearly no MFT keys to delete
        // so declare victory and go home.
        //
        if (hr == _HR_KEY_NOT_FOUND) 
        {
            hr = S_OK;
        }

        //
        // whether it's an error or it's not there, if we can't open the root key
        // we can't do anything more, so get out.
        //
        hrFinal = hr;
        goto out;
    }

    //
    // we'll continue even if we don't succeed on this one, since we still want to try to erase any trace of
    // this clsid
    //
    hr = DeleteKeyAndSubkeys(hkeyRoot, StringFromGUID(clsidMFT, szGUID));
    if (FAILED(hr)) 
    {
        hrFinal = hr;
    }

    (void)RegCloseKey(hkeyRoot);
    hkeyRoot = NULL;

    for (UINT32 unIndex = 0; TRUE; unIndex++)
    {
        hr = OpenCategoryKeyByIndex(unIndex, &hkeyRoot, KEY_READ | KEY_WRITE | DELETE);
        if (hr == _HR_NO_MORE_ITEMS) 
        {
            hr = S_OK;
            break;
        }
        if (FAILED(hr)) 
        {
            //
            // save the error code and keep going - maybe we can succeed somewhere
            //
            hrFinal = hr;
            continue;
        }

        //
        // we'll continue even if we don't succeed on this one, since it's possible the listings are corrupt,
        // and this MFT may not be in this category
        //
        hr = DeleteKeyAndSubkeys(hkeyRoot, StringFromGUID(clsidMFT, szGUID));
        if (FAILED(hr)) 
        {
            //
            // save the error code and keep going - maybe we can succeed somewhere
            //
            hrFinal = hr;
        }

        (void)RegCloseKey(hkeyRoot);
        hkeyRoot = NULL;
    }
    
out:
    if (hkeyRoot != NULL) 
    {
        (void)RegCloseKey(hkeyRoot);
    }

    return hrFinal;
}

HRESULT GetNumSubkeys(
    __in    HKEY    hkey,
    __out   UINT32* pcSubkeys
    )
{
    return _HR_FROM_W32(RegQueryInfoKey(hkey, 
                                              NULL, 
                                              NULL, 
                                              NULL, 
                                              (DWORD*)pcSubkeys, 
                                              NULL, 
                                              NULL, 
                                              NULL, 
                                              NULL, 
                                              NULL, 
                                              NULL, 
                                              NULL));
}

HRESULT GetNameFromRegistry(
    __in    HKEY    hkeyMFT,
    __out   LPWSTR* ppszName
    )
{
    DWORD   dwType = 0;
    DWORD   cbData = 0;
    HRESULT hr = S_OK;

    *ppszName = NULL;

    //
    // get the size
    //
    hr = _HR_FROM_W32(RegQueryValueEx(hkeyMFT, NULL, NULL, &dwType, NULL, &cbData));
    if (FAILED(hr)) 
    {
        goto out;
    }
    
    //
    // basic sanity checks
    //
    if (cbData == 0 || dwType != REG_SZ) 
    {
        hr = E_FAIL;
        goto out;
    }

    //
    // allocate room
    //
    *ppszName = (WCHAR*)CoTaskMemAlloc(cbData);
    if (*ppszName == NULL) 
    {
        hr = E_OUTOFMEMORY;
        goto out;
    }

    //
    // get the actual string
    //
    hr = _HR_FROM_W32(RegQueryValueEx(hkeyMFT, NULL, NULL, NULL, (BYTE*)(*ppszName), &cbData));
    if (FAILED(hr)) 
    {
        goto out;
    }

out:
    if (FAILED(hr) && *ppszName != NULL) 
    {
        CoTaskMemFree(*ppszName);
        *ppszName = NULL;
    }

    return hr;
}

HRESULT
GetDescFromRegistry(
    __in    HKEY    hkeyMFT,
    __out   LPWSTR* ppszDesc
    )
{
    DWORD   dwType = 0;
    DWORD   cbData = 0;
    HRESULT hr = S_OK;

    *ppszDesc = NULL;

    //
    // get the size
    //
    hr = _HR_FROM_W32(RegQueryValueEx(hkeyMFT, DESC_STR, NULL, &dwType, NULL, &cbData));
    if (FAILED(hr)) 
    {
        goto out;
    }
    
    //
    // basic sanity checks
    //
    if (cbData == 0 || dwType != REG_SZ) 
    {
        hr = E_FAIL;
        goto out;
    }

    //
    // allocate room
    //
    *ppszDesc = (WCHAR*)CoTaskMemAlloc(cbData);
    if (*ppszDesc == NULL) 
    {
        hr = E_OUTOFMEMORY;
        goto out;
    }

    //
    // get the actual string
    //
    hr = _HR_FROM_W32(RegQueryValueEx(hkeyMFT, DESC_STR, NULL, NULL, (BYTE*)(*ppszDesc), &cbData));
    if (FAILED(hr)) 
    {
        goto out;
    }

out:
    if (FAILED(hr) && *ppszDesc != NULL) 
    {
        CoTaskMemFree(*ppszDesc);
        *ppszDesc = NULL;
    }

    return hr;
}

HRESULT
SetNameToRegistry(
    __in    HKEY    hkeyMFT,
    __in    LPWSTR  pszName
    )
{
    HRESULT hr = S_OK;
    size_t  unStrLen = 0;
    UINT32  cbSize = 0;

    unStrLen = wcslen(pszName);

    //
    // sanity check - this seems like a more than reasonable name limit
    //
    if (unStrLen > MAX_MFT_NAME_LENGTH)
    {
        hr = E_INVALIDARG;
        goto out;
    }

    cbSize = (UINT32(unStrLen) + 1) * sizeof(WCHAR);

    hr = _HR_FROM_W32(RegSetValueEx(hkeyMFT, NULL, 0, REG_SZ, (BYTE*)pszName, cbSize));
    if (FAILED(hr))
    {
        goto out;
    }

out:
    return hr;
}

HRESULT SetDescToRegistry(
    __in    HKEY    hkeyMFT,
    __in    LPWSTR  pszDesc
    )
{
    HRESULT hr = S_OK;
    size_t  unStrLen = 0;
    UINT32  cbSize = 0;

    unStrLen = wcslen(pszDesc);

    if (unStrLen > MAX_MFT_DESC_LENGTH)
    {
        hr = E_INVALIDARG;
        goto out;
    }

    cbSize = (UINT32(unStrLen) + 1) * sizeof(WCHAR);

    hr = _HR_FROM_W32(RegSetValueEx(hkeyMFT, DESC_STR, 0, REG_SZ, (BYTE*)pszDesc, cbSize));
    if (FAILED(hr))
    {
        goto out;
    }

out:
    return hr;
}

HRESULT OpenCategoryKey(
    __in    GUID    guidCategory,
    __out   HKEY*   phkey
    )
{
    HRESULT hr = S_OK;
    WCHAR   szGUID[GUID_BUFFER_SIZE];
    HKEY    hkeyRoot = NULL;

    hr = OpenCategoryRootKey(&hkeyRoot);
    if (FAILED(hr)) 
    {
        goto out;
    }

    hr = _HR_FROM_W32(RegOpenKeyExW(hkeyRoot, StringFromGUID(guidCategory, szGUID), 0, KEY_READ, phkey));
    if (FAILED(hr)) 
    {
        goto out;
    }

out:
    if (hkeyRoot != NULL) 
    {
        (void)RegCloseKey(hkeyRoot);
    }

    return hr;
}

HRESULT CreateCategoryKey(
    __in    GUID    guidCategory,
    __out   HKEY*   phkey
    )
{
    HRESULT hr = S_OK;
    WCHAR   szGUID[GUID_BUFFER_SIZE];
    HKEY    hkeyRoot = NULL;

    hr = CreateCategoryRootKey(&hkeyRoot);
    if (FAILED(hr)) 
    {
        goto out;
    }

    hr = _HR_FROM_W32(RegCreateKeyExW(hkeyRoot, StringFromGUID(guidCategory, szGUID), 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, phkey, NULL));
    if (FAILED(hr)) 
    {
        goto out;
    }

out:
    if (hkeyRoot != NULL) 
    {
        (void)RegCloseKey(hkeyRoot);
    }

    return hr;
}

HRESULT AddMFTToCategory(
    __in    GUID    guidCategory,
    __in    CLSID   clsidMFT
    )
{
    HRESULT hr = S_OK;
    WCHAR   szGUID[GUID_BUFFER_SIZE];
    HKEY    hkeyCategory = NULL;
    HKEY    hkeyMFT = NULL;

    hr = CreateCategoryKey(guidCategory, &hkeyCategory);
    if (FAILED(hr)) 
    {
        goto out;
    }

    hr = _HR_FROM_W32(RegCreateKeyEx(hkeyCategory, StringFromGUID(clsidMFT, szGUID), 0, NULL, 0, KEY_WRITE, NULL, &hkeyMFT, NULL));
    if (FAILED(hr)) 
    {
        goto out;
    }

out:
    if (hkeyCategory != NULL) 
    {
        (void)RegCloseKey(hkeyCategory);
    }

    if (hkeyMFT != NULL) 
    {
        (void)RegCloseKey(hkeyMFT);
    }

    return hr;
}

STDAPI
MFVEMFTRegister(
    __in                            CLSID                   clsidMFT,
    __in                            GUID                    guidCategory,
    __in                            LPWSTR                  pszName,
    __in                            LPWSTR                  pszDesc
    )
{
    HKEY    hkeyMFT = NULL;
    HRESULT hr = S_OK;
    
    //
    // create the MFT's own key
    //
    hr = CreateMFTKey(clsidMFT, &hkeyMFT);
    if(FAILED(hr))
    {
        goto out;
    }

    hr = SetNameToRegistry(hkeyMFT, pszName);
    if(FAILED(hr))
    {
        goto out;
    }

    hr = SetDescToRegistry(hkeyMFT, pszDesc);
    if(FAILED(hr))
    {
        goto out;
    }
    
    hr = AddMFTToCategory(guidCategory, clsidMFT);
    if (FAILED(hr))
    {
        goto out;
    }

    
out:
    if (hkeyMFT != NULL) 
    {
        (void)RegCloseKey(hkeyMFT);
    }

    if (FAILED(hr)) 
    {
        (void)DeleteMFTKey(clsidMFT);
    }
    return hr;
}

STDAPI
MFVEMFTUnregister(
    __in    CLSID   clsidMFT
    )
{
    HRESULT hr = S_OK;
    HRESULT hrFinal = S_OK;

    //
    // delete this clsid from the main key and categories
    //
    hr = DeleteMFTKey(clsidMFT);
    if (FAILED(hr)) 
    {
        //
        // continue anyway, but save the error code
        //
        hrFinal = hr;
    }

    return hrFinal;
}

STDAPI
MFVEMFTEnum(
    __in                    GUID                    guidCategory,
    __out_ecount(*pcMFTs)   CLSID**                 ppclsidMFT, // must be freed with CoTaskMemFree
    __out                   UINT32*                 pcMFTs                  
    )
{
    HKEY    hkeyRoot = NULL;
    HRESULT hr = S_OK;
    CLSID*  pclsidCur = NULL;
    CLSID*  pclsidEnd = NULL;
    UINT32  cSubkeys = 0;

    *ppclsidMFT = NULL;
    *pcMFTs = 0;

    if (guidCategory != GUID_NULL)
    {
        hr = OpenCategoryKey(guidCategory, &hkeyRoot);
    } 
    else
    {
        hr = OpenRootKey(&hkeyRoot);
    }

    if (FAILED(hr)) 
    {
        if (hr == _HR_KEY_NOT_FOUND) 
        {
            //
            // there aren't any, but that's not an error
            //
            hr = S_OK;
        }
        goto out;
    }

    //
    // we'll just allocate enough room for all possible subkeys, because this isn't a
    // situation where we need to conserve memory
    //
    hr = GetNumSubkeys(hkeyRoot, &cSubkeys);
    if (FAILED(hr)) 
    {
        goto out;
    }

    *ppclsidMFT = (CLSID*)CoTaskMemAlloc(cSubkeys * sizeof(CLSID));
    if (*ppclsidMFT == NULL) 
    {
        hr = E_OUTOFMEMORY;
        goto out;
    }
    pclsidCur = *ppclsidMFT;
    pclsidEnd = *ppclsidMFT + cSubkeys;

    for (UINT32 unIndex = 0; pclsidCur != pclsidEnd; unIndex++) {
        CLSID   clsidMFT;
        BOOL    bMatch = FALSE;
        WCHAR   szGUID[GUID_BUFFER_SIZE];
        UINT32  cchBufSize = GUID_BUFFER_SIZE;

        hr = _HR_FROM_W32(RegEnumKeyExW(hkeyRoot, unIndex, szGUID, (DWORD*)&cchBufSize, NULL, NULL, NULL, NULL));
        if (hr == _HR_NO_MORE_ITEMS) 
        {
            hr = S_OK;
            break;
        }
        if (hr == _HR_MORE_DATA)
        {
            //
            // the name was bigger than a GUID - skip this one
            //
            hr = S_OK;
            continue;
        }
        if (FAILED(hr)) 
        {
            goto out;
        }

        hr = GUIDFromString(szGUID, &clsidMFT);
        if (FAILED(hr)) 
        {
            //
            // it's not a guid - skip this one
            //
            hr = S_OK;
            continue;
        }

        *pclsidCur = clsidMFT;
        pclsidCur++;
        (*pcMFTs)++;

        if (pclsidCur == pclsidEnd) 
        {
        //
        // We hit the end of the buffer - must have added one during the enumeration or something.
        // We're done.
        //
        break;
        }
    }
    
out:
    if (hkeyRoot != NULL) 
    {
        (void)RegCloseKey(hkeyRoot);
    }

    if (FAILED(hr) || *pcMFTs == 0) 
    {
        CoTaskMemFree(*ppclsidMFT);
        *ppclsidMFT = NULL;
        *pcMFTs = 0;
    }

    return hr;
}
STDAPI
MFVEMFTGetInfo(
    CLSID                       clsidMFT,
    __out LPWSTR*                     ppszName,
    __out LPWSTR*                     ppszDesc
    )
{
    HRESULT hr = S_OK;
    HKEY    hkeyMFT = NULL;

    if(ppszName)
    {
        *ppszName = NULL;
    }
    
    if(ppszDesc)
    {
        *ppszDesc = NULL;
    }

    hr = OpenMFTKey(clsidMFT, &hkeyMFT);
    if(FAILED(hr))
    {
        goto out;
    }

    if (ppszName)
    {
        hr = GetNameFromRegistry(hkeyMFT, ppszName);
        if(FAILED(hr))
        {
            goto out;
        }
    }
    
    if(ppszDesc)
    {
        hr = GetDescFromRegistry(hkeyMFT, ppszDesc);
        if(FAILED(hr))
        {
            goto out;
        }
    }
out:
    if (hkeyMFT != NULL) 
    {
        (void)RegCloseKey(hkeyMFT);
    }

    if (FAILED(hr)) 
    {
        //
        // cleanup anything that's been allocated
        //
        if (ppszName && *ppszName) 
        {
            CoTaskMemFree(*ppszName);
            *ppszName = NULL;
        }
    }

    return hr;
}