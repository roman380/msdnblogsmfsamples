// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE
//
// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "stdafx.h"

#include "timebar.h"

#include <commctrl.h>
#include <assert.h>

CTimeBarControl::CTimeBarControl() 
    : m_scrollCallback(NULL)
    , m_fTracking(false)
{
}

HRESULT CTimeBarControl::Init(HWND hParentWnd, RECT& rect, bool fHoriz, bool fAutoTicks)
{
    HRESULT hr = S_OK;

    DWORD style = WS_CHILD | WS_VISIBLE;
    if(fHoriz)
    {
        style |= TBS_HORZ;
    }
    else 
    {
        style |= TBS_VERT;
    }

    if(fAutoTicks) 
    {
        style |= TBS_AUTOTICKS;
    }
    
    if(Create(hParentWnd, _U_RECT(rect), L"Slider", style, 0, 0U, NULL) == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    SendMessage(m_hWnd, TBM_SETPAGESIZE, 0, (LPARAM) 1);                  
done:
    return hr;
}

LONG CTimeBarControl::GetMaxPos() const
{
    return static_cast<LONG>(SendMessage(m_hWnd, TBM_GETRANGEMAX, 0, 0));
}

LONG CTimeBarControl::GetPos() const
{
    return static_cast<LONG>(SendMessage(m_hWnd, TBM_GETPOS, 0, 0));
}

void CTimeBarControl::SetPos(LONG lPos)
{
    SendMessage(m_hWnd, TBM_SETPOS, TRUE, lPos);
}

void CTimeBarControl::SetRange(LONG lMinValue, LONG lMaxValue)
{
    SendMessage(TBM_SETRANGEMIN, (WPARAM) FALSE, lMinValue);
    SendMessage(TBM_SETRANGEMAX, (WPARAM) TRUE, lMaxValue); 
    
    RECT rectClient;
    GetClientRect(&rectClient);
    SetTickFreq(static_cast<WORD>((lMaxValue - lMinValue) / (rectClient.right / 10)));
}

void CTimeBarControl::SetTickFreq(WORD wFreq)
{
    SendMessage(TBM_SETTICFREQ, wFreq, 0);
}

void CTimeBarControl::SetScrollCallback(HANDLESCROLLPROC scrollCallback)
{
    m_scrollCallback = scrollCallback;
}

bool CTimeBarControl::IsTracking()
{
    return m_fTracking;
}

void CTimeBarControl::SetPosByTime(MFTIME hnsTime, MFTIME hnsDuration)
{
    if(hnsDuration > 0)
    {
        SetPos(static_cast<LONG>(GetMaxPos() * hnsTime / hnsDuration));
    }
}

LRESULT CTimeBarControl::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
{
    HandleScroll(LOWORD(wParam), HIWORD(wParam));

    return 0;
}

LRESULT CTimeBarControl::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
{
    HandleScroll(LOWORD(wParam), HIWORD(wParam));

    return 0;  
}

void CTimeBarControl::HandleScroll(WORD wMsg, WORD wPos) 
{
    switch(wMsg)
    {
    case TB_PAGEDOWN: // fallthrough
    case TB_PAGEUP: 
    {
        LRESULT pos = SendMessage(m_hWnd, TBM_GETPOS, 0, 0);
        if(m_scrollCallback) m_scrollCallback(static_cast<WORD>(pos), true);
        break;
    }
    case TB_THUMBTRACK:
        m_fTracking = true;
        if(m_scrollCallback) m_scrollCallback(wPos, false);
        break;
    case TB_THUMBPOSITION:
        m_fTracking = false;
        if(m_scrollCallback) m_scrollCallback(wPos, true);
        break;
    }
}

LRESULT CTimeBarControl::OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
{
    POINT pt;
    pt.x = GET_X_LPARAM(lParam); 
    pt.y = GET_Y_LPARAM(lParam); 

    pt.x -= 10;
    
    if(pt.x < 0) return 0;

    RECT rect;
    GetClientRect(&rect);
    
    if(pt.x > rect.right - rect.left - 20) return 0;

    LRESULT pos = SendMessage(m_hWnd, TBM_GETRANGEMAX, 0, 0);

    m_LastClickPos = pos * pt.x / (rect.right - rect.left - 20);

    SetCapture();
    m_fTracking = true;
    return 0;
}

LRESULT CTimeBarControl::OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
{
    ReleaseCapture();
    m_fTracking = false;
    
    POINT pt;
    pt.x = GET_X_LPARAM(lParam); 
    pt.y = GET_Y_LPARAM(lParam); 

    pt.x -= 10;
    if(pt.x < 0) return 0;

    RECT rect;
    GetClientRect(&rect);

    if(pt.x > rect.right - rect.left - 20) return 0;
    
    LRESULT pos = SendMessage(m_hWnd, TBM_GETRANGEMAX, 0, 0);

    m_LastClickPos = pos * pt.x / (rect.right - rect.left - 20);

    SendMessage(m_hWnd, TBM_SETPOS, TRUE, m_LastClickPos);
    if(m_scrollCallback) m_scrollCallback(static_cast<WORD>(m_LastClickPos), true);
    
   return 0;
}

LRESULT CTimeBarControl::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
{
    if(m_fTracking)
    {
        POINT pt;
        pt.x = GET_X_LPARAM(lParam); 
        pt.y = GET_Y_LPARAM(lParam); 

        pt.x -= 10;
        
        if(pt.x < 0) return 0;
        
        RECT rect;
        GetClientRect(&rect);

        LRESULT pos = SendMessage(m_hWnd, TBM_GETRANGEMAX, 0, 0);

        m_LastClickPos = pos * pt.x / (rect.right - rect.left - 20);

        SendMessage(m_hWnd, TBM_SETPOS, TRUE, m_LastClickPos);
        if(m_scrollCallback) m_scrollCallback(static_cast<WORD>(m_LastClickPos), false);
    }
    
    return 0;
}

LRESULT CTimeBarControl::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RECT rectClient;
    GetClientRect(&rectClient);
    
    if(rectClient.right != 0)
    {
        LONG lMaxValue = static_cast<LONG>(SendMessage(TBM_GETRANGEMAX, 0, 0));
        LONG lMinValue = static_cast<LONG>(SendMessage(TBM_GETRANGEMIN, 0, 0));
        SetTickFreq(static_cast<WORD>((lMaxValue - lMinValue) / (rectClient.right / 10)));
    }
    
    bHandled = false;
    return 0;
}
