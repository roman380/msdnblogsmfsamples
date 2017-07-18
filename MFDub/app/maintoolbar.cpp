// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE
//
// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "stdafx.h"

#include "maintoolbar.h"
#include "resource.h"

CMainToolbar::CMainToolbar()
{

}

CMainToolbar::~CMainToolbar()
{

}

void CMainToolbar::EnableButtonByCommand(int iID, BOOL fEnable)
{
    SendMessage(TB_ENABLEBUTTON, iID, MAKELONG(fEnable, 0));
}

LRESULT CMainToolbar::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SendMessage(TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessage(TB_SETBITMAPSIZE, 0, MAKELONG(24, 24));

    TBADDBITMAP StandardBitmaps;
    StandardBitmaps.hInst = HINST_COMMCTRL;
    StandardBitmaps.nID = IDB_STD_LARGE_COLOR;
    SendMessage(TB_ADDBITMAP, 0, (LPARAM) &StandardBitmaps);
    
    TBADDBITMAP AppBitmaps;
    AppBitmaps.hInst = _AtlBaseModule.GetModuleInstance();
    AppBitmaps.nID = IDB_MAINTOOL;
    LRESULT iAppBitmapIndex = SendMessage(TB_ADDBITMAP, 3, (LPARAM) &AppBitmaps);
    
    if(iAppBitmapIndex == -1)
    {
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        CAtlString str;
        str.Format(L"Failed to add toolbar images.  Reason: %x", hr);

        MessageBox(str, L"Error", MB_OK);
    }
    
    int iOpen = static_cast<int>(SendMessage(m_hWnd, TB_ADDSTRING, 0, (LPARAM) L"Open\0"));
    int iSave = static_cast<int>(SendMessage(m_hWnd, TB_ADDSTRING, 0, (LPARAM) L"Save\0"));
    int iAddVideoTransform = static_cast<int>(SendMessage(m_hWnd, TB_ADDSTRING, 0, (LPARAM) L"Video MFTs\0"));
    int iAddAudioTransform = static_cast<int>(SendMessage(m_hWnd, TB_ADDSTRING, 0, (LPARAM) L"Audio MFTs\0"));
    int iEncodeOptions = static_cast<int>(SendMessage(m_hWnd, TB_ADDSTRING, 0, (LPARAM) L"Encode Options\0"));
    int iProperties = static_cast<int>(SendMessage(m_hWnd, TB_ADDSTRING, 0, (LPARAM) L"Properties\0"));

    TBBUTTON ToolbarButtons[6];

    ToolbarButtons[0].iBitmap = STD_FILEOPEN;
    ToolbarButtons[0].idCommand = ID_OPEN;
    ToolbarButtons[0].fsState = TBSTATE_ENABLED;
    ToolbarButtons[0].fsStyle = TBSTYLE_BUTTON | BTNS_AUTOSIZE;
    ToolbarButtons[0].dwData = 0;
    ToolbarButtons[0].iString = iOpen;
    
    ToolbarButtons[1].iBitmap = STD_FILESAVE;
    ToolbarButtons[1].idCommand = ID_SAVE;
    ToolbarButtons[1].fsState = TBSTATE_ENABLED;
    ToolbarButtons[1].fsStyle = TBSTYLE_BUTTON | BTNS_AUTOSIZE;
    ToolbarButtons[1].dwData = 1;
    ToolbarButtons[1].iString = iSave;
    
    ToolbarButtons[2].iBitmap = static_cast<int>(iAppBitmapIndex++);
    ToolbarButtons[2].idCommand = ID_ADDTRANSFORM;
    ToolbarButtons[2].fsState = TBSTATE_ENABLED;
    ToolbarButtons[2].fsStyle = TBSTYLE_BUTTON | BTNS_AUTOSIZE;
    ToolbarButtons[2].dwData = 2;
    ToolbarButtons[2].iString = iAddVideoTransform;
    
    ToolbarButtons[3].iBitmap = static_cast<int>(iAppBitmapIndex++);
    ToolbarButtons[3].idCommand = ID_ADDAUDIOTRANSFORM;
    ToolbarButtons[3].fsState = TBSTATE_ENABLED;
    ToolbarButtons[3].fsStyle = TBSTYLE_BUTTON | BTNS_AUTOSIZE;
    ToolbarButtons[3].dwData = 3;
    ToolbarButtons[3].iString = iAddAudioTransform;
    
    ToolbarButtons[4].iBitmap = static_cast<int>(iAppBitmapIndex++);
    ToolbarButtons[4].idCommand = ID_ENCODEOPTIONS;
    ToolbarButtons[4].fsState = TBSTATE_ENABLED;
    ToolbarButtons[4].fsStyle = TBSTYLE_BUTTON | BTNS_AUTOSIZE;
    ToolbarButtons[4].dwData = 4;
    ToolbarButtons[4].iString = iEncodeOptions;
    
    ToolbarButtons[5].iBitmap = STD_PROPERTIES;
    ToolbarButtons[5].idCommand = ID_PROPERTIES;
    ToolbarButtons[5].fsState = TBSTATE_ENABLED;
    ToolbarButtons[5].fsStyle = TBSTYLE_BUTTON | BTNS_AUTOSIZE;
    ToolbarButtons[5].dwData = 5;
    ToolbarButtons[5].iString = iProperties;

    if(FALSE == SendMessage(TB_ADDBUTTONS, 6, (LPARAM) (LPTBBUTTON) ToolbarButtons))
    {
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        CAtlString str;
        str.Format(L"Failed to add toolbar buttons.  Reason: %x", hr);

        MessageBox(str, L"Error", MB_OK);
    }

    SendMessage(TB_AUTOSIZE, 0, 0);

    return 0;
}
