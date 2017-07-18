// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE
//
// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "stdafx.h"

#include "transporttoolbar.h"
#include "resource.h"

CTransportToolbar::CTransportToolbar()
{
}

CTransportToolbar::~CTransportToolbar()
{
    if(m_hStaticFont) 
    {
        DeleteObject(m_hStaticFont);
    }
}

void CTransportToolbar::EnableButtonByCommand(UINT unID, BOOL fEnable)
{
    SendMessage(TB_ENABLEBUTTON, unID, MAKELONG(fEnable, 0));
}

LRESULT CTransportToolbar::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RECT rectClient;
    GetClientRect(&rectClient);
    
    SendMessage(TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);
    
    TBADDBITMAP tbAddBitmap;
    tbAddBitmap.hInst = _AtlBaseModule.GetModuleInstance();
    tbAddBitmap.nID = IDB_TRANSPORTTOOL;
    int iBitmapIndex = static_cast<int>(SendMessage(TB_ADDBITMAP, 5, (LPARAM)&tbAddBitmap));
    
    TBBUTTON ToolbarButtons[8];

    ToolbarButtons[0].iBitmap = iBitmapIndex++;
    ToolbarButtons[0].idCommand = ID_PLAYOUTPUT;
    ToolbarButtons[0].fsState = TBSTATE_ENABLED;
    ToolbarButtons[0].fsStyle = TBSTYLE_BUTTON;
    ToolbarButtons[0].dwData = 0;
    ToolbarButtons[0].iString = 0;
    
    ToolbarButtons[1].iBitmap = iBitmapIndex++;
    ToolbarButtons[1].idCommand = ID_PLAYPREVIEW;
    ToolbarButtons[1].fsState = TBSTATE_ENABLED;
    ToolbarButtons[1].fsStyle = TBSTYLE_BUTTON;
    ToolbarButtons[1].dwData = 1;
    ToolbarButtons[1].iString = 0;
    
    ToolbarButtons[2].iBitmap = iBitmapIndex++;
    ToolbarButtons[2].idCommand = ID_STOP;
    ToolbarButtons[2].fsState = TBSTATE_ENABLED;
    ToolbarButtons[2].fsStyle = TBSTYLE_BUTTON;
    ToolbarButtons[2].dwData = 2;
    ToolbarButtons[2].iString = 0;
    
    ToolbarButtons[3].iBitmap = 0;
    ToolbarButtons[3].idCommand = 0;
    ToolbarButtons[3].fsState = TBSTATE_ENABLED;
    ToolbarButtons[3].fsStyle = BTNS_SEP;
    ToolbarButtons[3].dwData = 3;
    ToolbarButtons[3].iString = 0;
    
    ToolbarButtons[4].iBitmap = iBitmapIndex++;
    ToolbarButtons[4].idCommand = ID_GO_BEGINNING;
    ToolbarButtons[4].fsState = TBSTATE_ENABLED;
    ToolbarButtons[4].fsStyle = TBSTYLE_BUTTON;
    ToolbarButtons[4].dwData = 4;
    ToolbarButtons[4].iString = 0;
    
    ToolbarButtons[5].iBitmap = iBitmapIndex++;
    ToolbarButtons[5].idCommand = ID_GO_END;
    ToolbarButtons[5].fsState = TBSTATE_ENABLED;
    ToolbarButtons[5].fsStyle = TBSTYLE_BUTTON;
    ToolbarButtons[5].dwData = 5;
    ToolbarButtons[5].iString = 0;
    
    ToolbarButtons[6].iBitmap = iBitmapIndex++;
    ToolbarButtons[6].idCommand = ID_PREVKEYFRAME;
    ToolbarButtons[6].fsState = TBSTATE_ENABLED;
    ToolbarButtons[6].fsStyle = TBSTYLE_BUTTON;
    ToolbarButtons[6].dwData = 6;
    ToolbarButtons[6].iString = 0;
    
    ToolbarButtons[7].iBitmap = iBitmapIndex++;
    ToolbarButtons[7].idCommand = ID_NEXTKEYFRAME;
    ToolbarButtons[7].fsState = TBSTATE_ENABLED;
    ToolbarButtons[7].fsStyle = TBSTYLE_BUTTON;
    ToolbarButtons[7].dwData = 7;
    ToolbarButtons[7].iString = 0;

    LOGFONT lfLabelFont;
    lfLabelFont.lfHeight = 14;
    lfLabelFont.lfWidth = 0;
    lfLabelFont.lfEscapement = 0;
    lfLabelFont.lfOrientation = 0;
    lfLabelFont.lfWeight = FW_DONTCARE;
    lfLabelFont.lfItalic = FALSE;
    lfLabelFont.lfUnderline = FALSE;
    lfLabelFont.lfStrikeOut = FALSE;
    lfLabelFont.lfCharSet = DEFAULT_CHARSET;
    lfLabelFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lfLabelFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lfLabelFont.lfQuality = DEFAULT_QUALITY;
    lfLabelFont.lfPitchAndFamily = FF_DONTCARE | DEFAULT_PITCH;
    wcscpy_s(lfLabelFont.lfFaceName, 32, L"Arial");
    m_hStaticFont = CreateFontIndirect(&lfLabelFont);

    if(FALSE == SendMessage(TB_ADDBUTTONS, 8, (LPARAM) (LPTBBUTTON) ToolbarButtons))
    {
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        CAtlString str;
        str.Format(L"Failed to add toolbar buttons.  Reason: %x", hr);

        MessageBox(str, L"Error", MB_OK);
    }
    
    SendMessage(TB_AUTOSIZE, 0, 0);
    
    RECT rectSampleTimeLabel;
    rectSampleTimeLabel.left = 200;
    rectSampleTimeLabel.top = 5;
    rectSampleTimeLabel.right = 400;
    rectSampleTimeLabel.bottom = rectClient.bottom;
    m_SampleTimeLabel.Create(m_hWnd, rectSampleTimeLabel, L"", WS_CHILD | WS_VISIBLE);
    m_SampleTimeLabel.SetFont(m_hStaticFont);
    
    return 0;
}

void CTransportToolbar::SetSampleTimeLabelText(LPCWSTR szText)
{
    RECT rectClient;
    GetClientRect(&rectClient);
    
    m_SampleTimeLabel.SetWindowPos(HWND_TOP, 0, 0, 200, rectClient.bottom, SWP_NOMOVE);
    m_SampleTimeLabel.SetWindowText(szText);
}