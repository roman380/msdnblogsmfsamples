// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE
//
// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "stdafx.h"

#include "transformdisplay.h"
#include "transformapplier.h"

CTransformWindow::CTransformWindow(CTransformDisplay* pParent, CAtlString strTransformName, HFONT hLabelFont)
    : m_pParent(pParent)
    , m_strTransformName(strTransformName)
    , m_hLabelFont(hLabelFont)
    , m_fIsMoving(false)
    , m_dxPos(0)
    , m_dyPos(0)
    , m_fHighlight(false)
{
}

CTransformWindow::~CTransformWindow()
{
}

CAtlString CTransformWindow::GetTransformName()
{
    return m_strTransformName;
}

bool CTransformWindow::IsInsideRect(RECT rectBounds)
{
    RECT rectWindow;
    GetWindowRect(&rectWindow);
    
    POINT ptLeftUp = {rectWindow.left, rectWindow.top};
    POINT ptLeftBot = {rectWindow.left, rectWindow.bottom};
    POINT ptRightUp = {rectWindow.right, rectWindow.top};
    POINT ptRightBot = {rectWindow.right, rectWindow.bottom};
    
    if( PtInRect(&rectBounds, ptLeftUp)
        || PtInRect(&rectBounds, ptLeftBot)
        || PtInRect(&rectBounds, ptRightUp)
        || PtInRect(&rectBounds, ptRightBot)
       )
    {
        return true;
    }
    
    return false;
}

LRESULT CTransformWindow::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return 0;
}

LRESULT CTransformWindow::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(&ps);
    HBRUSH hBrush;

    if(m_fHighlight)
    {
        hBrush = CreateSolidBrush(RGB(0, 0, 200));
    }
    else
    {
        hBrush = CreateSolidBrush(RGB(0, 0, 0));
    }

    HBRUSH hWhiteBrush = CreateSolidBrush(RGB(255, 255, 255));
    HFONT hOldFont = (HFONT) SelectObject(hDC, m_hLabelFont);
    
    DWORD x = 5;
    DWORD y = 5;
    RECT rectFrame;
    GetClientRect(&rectFrame);
    
    FillRect(hDC, &rectFrame, hWhiteBrush);
    FrameRect(hDC, &rectFrame, hBrush);
    
    rectFrame.left += 5;
    rectFrame.top += 2;
    rectFrame.right -= 2;
    rectFrame.bottom -= 2;    
    DrawText(hDC, m_strTransformName.GetString(), m_strTransformName.GetLength(), &rectFrame, DT_SINGLELINE);
   
    SelectObject(hDC, hOldFont);
    DeleteObject(hBrush);
    DeleteObject(hWhiteBrush);
    EndPaint(&ps);
    
    return 0;
}

LRESULT CTransformWindow::OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return 0;
}

LRESULT CTransformWindow::OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_fIsMoving = true;
    SetCapture();
    
    m_dxPos = GET_X_LPARAM(lParam);
    m_dyPos = GET_Y_LPARAM(lParam);
    
    return 0;
}

LRESULT CTransformWindow::OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    ReleaseCapture();
    m_fIsMoving = false;
    
    m_pParent->SendMessage(WM_CHILDMOVEFINISHED, 0, 0);
    
    return 0;
}

LRESULT CTransformWindow::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if(m_fIsMoving)
    {
        RECT rectWindow;
        GetWindowRect(&rectWindow);
        
        SetWindowPos(HWND_TOP, rectWindow.left + GET_X_LPARAM(lParam) - m_dxPos, rectWindow.top + GET_Y_LPARAM(lParam) - m_dyPos, 0, 0, SWP_NOSIZE);
        Invalidate();
    }
       
    if(!m_fHighlight)
    {
        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = m_hWnd;
        tme.dwHoverTime = 0;
        if(TrackMouseEvent(&tme))
        {
            m_fHighlight = true;
            InvalidateRect(NULL);
        }
    }

    return 0;
}

LRESULT CTransformWindow::OnMouseLeave(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if(m_fHighlight)
    {
        m_fHighlight = false;
        InvalidateRect(NULL);
    }

    return 0;
}

void CTransformWindow::OnFinalMessage(HWND hWnd)
{
    delete this;
}

///////////////////////////////////////

CTransformDisplay::CTransformDisplay(CTransformApplier* pTransformApplier)
    : m_pTransformApplier(pTransformApplier)
    , m_pEventListener(NULL)
{
}

CTransformDisplay::~CTransformDisplay()
{
    if(m_hLabelFont)
    {
        DeleteObject(m_hLabelFont);
    }
    
    for(size_t i = 0; i < m_arrTransformWindows.GetCount(); i++)
    {
       m_arrTransformWindows.GetAt(i)->DestroyWindow();
    }
}

void CTransformDisplay::SetEventListener(CTransformDisplayEventListener* pListener)
{
    m_pEventListener = pListener;
}

void CTransformDisplay::Reset()
{
    for(size_t i = 0; i < m_arrTransformWindows.GetCount(); i++)
    {
       m_arrTransformWindows.GetAt(i)->DestroyWindow();
    } 
}

LRESULT CTransformDisplay::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
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
    m_hLabelFont = CreateFontIndirect(&lfLabelFont);
    
    return 0;
}

LRESULT CTransformDisplay::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(&ps);
    
    RECT rectClient;
    GetClientRect(&rectClient);
    
    HBRUSH hWhiteBrush = static_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH));
    FillRect(hDC, &rectClient, hWhiteBrush);

    HBRUSH hBrush = static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));
    FrameRect(hDC, &rectClient, hBrush);
    
    int iTitleLength = GetWindowTextLength();
    LPWSTR szTitle = new WCHAR[iTitleLength + 1];
    GetWindowText(szTitle, iTitleLength + 1);

    RECT rectText = rectClient;
    rectText.left += m_kMargin;
    rectText.top += m_kMargin;
    rectText.right -= m_kMargin;
    DrawText(hDC, szTitle, iTitleLength, &rectText, DT_SINGLELINE);

    if(m_pTransformApplier->GetTransformCount() > m_arrTransformWindows.GetCount())
    {
        RECT rectNewWindow;
        GetWindowRect(&rectNewWindow);
        
        rectNewWindow.left += m_kMargin;
        rectNewWindow.top += static_cast<LONG>(m_kMargin + m_kMargin + m_kdyTransformWindow + (m_kdyTransformWindow + m_kMargin) * m_arrTransformWindows.GetCount());
        rectNewWindow.right = rectNewWindow.left + m_kdxTransformWindow;
        rectNewWindow.bottom = rectNewWindow.top + m_kdyTransformWindow;
        for(size_t i = m_arrTransformWindows.GetCount(); i < m_pTransformApplier->GetTransformCount(); i++)
        {
            CTransformWindow* pWindow = new CTransformWindow(this, m_pTransformApplier->GetTransformName(i), m_hLabelFont);
            pWindow->Create(m_hWnd, rectNewWindow, L"Transform", WS_POPUP | WS_VISIBLE, 0, 0U, NULL);
            m_arrTransformWindows.Add(pWindow);
            
            rectNewWindow.top += m_kdyTransformWindow + m_kMargin;
            rectNewWindow.bottom += m_kdyTransformWindow + m_kMargin;
        }
    }
    
    RepositionWindows();
    
    EndPaint(&ps);
    
    delete[] szTitle;

    return 0;
}

LRESULT CTransformDisplay::OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return 1;
}

LRESULT CTransformDisplay::OnChildMoveFinished(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RECT rectWindow;
    GetWindowRect(&rectWindow);
    
    //
    // If the window moved to a position outside of us, destroy it and the transform it corresponds to
    //
    for(size_t i = 0; i < m_arrTransformWindows.GetCount(); i++)
    {
        if(!m_arrTransformWindows.GetAt(i)->IsInsideRect(rectWindow) && !m_arrTransformWindows.GetAt(i)->IsMoving())
        {
            m_pTransformApplier->RemoveTransformAtIndex(i);
            m_arrTransformWindows.GetAt(i)->DestroyWindow();
            m_arrTransformWindows.RemoveAt(i);
            
            if(m_pEventListener) m_pEventListener->HandleTransformRemoved();
            break;
        }
    }
    
    //
    // If the window moved to a new position, give it a new index so indexes are in order
    //
    for(size_t i = 1; i < m_arrTransformWindows.GetCount(); i++)
    {
        CTransformWindow* pLastWindow = m_arrTransformWindows.GetAt(i - 1);
        CTransformWindow* pCurWindow = m_arrTransformWindows.GetAt(i);
        
        RECT rectLast;
        pLastWindow->GetWindowRect(&rectLast);
        
        RECT rectCur;
        pCurWindow->GetWindowRect(&rectCur);
        
        if(rectCur.top < rectLast.top)
        {
            for(size_t j = 0; j < m_arrTransformWindows.GetCount(); j++)
            {
                pLastWindow = m_arrTransformWindows.GetAt(j);
                pLastWindow->GetWindowRect(&rectLast);
                
                if(rectCur.top < rectLast.top)
                {
                    m_arrTransformWindows.RemoveAt(i);
                    m_arrTransformWindows.InsertAt(j, pCurWindow);
                    m_pTransformApplier->MoveTransform(i, j);
                    
                    if(m_pEventListener) m_pEventListener->HandleTransformMoved();
                    break;
                }
            }
            
            break;
        }
    }
    
    RepositionWindows();
    
    return 0;
}


LRESULT CTransformDisplay::OnMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RepositionWindows();
    
    return 0;
}

void CTransformDisplay::RepositionWindows()
{
    RECT rectWindow;
    GetWindowRect(&rectWindow);
    
    for(size_t i = 0; i < m_arrTransformWindows.GetCount(); i++)
    {
        if(!m_arrTransformWindows.GetAt(i)->IsMoving())
        {
            m_arrTransformWindows.GetAt(i)->SetWindowPos(HWND_TOP, rectWindow.left + m_kMargin, static_cast<int>(rectWindow.top + 
                i * (m_kMargin + m_kdyTransformWindow) + m_kdyTransformWindow + m_kMargin + m_kMargin), 0, 0, SWP_NOSIZE);
        }
    }
}