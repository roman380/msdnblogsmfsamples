// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE
//
// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "stdafx.h"

#include "sampleoutputwindow.h"

#define SHADE_PIXEL(x) \
    if(((x) & 0x000000FF) < 0x50) (x) = (x) | 0x00000050 & 0x00FFFF50; \
    if(((x) & 0x0000FF00) < 0x5000) (x) = (x) | 0x00005000 & 0x00FF50FF; \
    if(((x) & 0x00FF0000) < 0x500000) (x) = (x) | 0x00500000 & 0x0050FFFF;  \
    (x) = (x) - 0x00505050; 
    
CSampleOutputWindow::CSampleOutputWindow()
    : m_pResizer(NULL)
    , m_pEventListener(NULL)
    , m_unSelectionLeft(0)
    , m_unSelectionTop(0)
    , m_unSelectionRight(0)
    , m_unSelectionBottom(0)
{
    m_pResizer = new CSampleResizer();
}

CSampleOutputWindow::~CSampleOutputWindow()
{
    delete m_pResizer;
}

HRESULT CSampleOutputWindow::SetSampleSize(UINT32 unSampleWidth, UINT32 unSampleHeight)
{
    HRESULT hr = S_OK;
    
    m_unSampleWidth = unSampleWidth;
    m_unSampleHeight = unSampleHeight;

    CHECK_HR( hr = m_pResizer->SetInputSampleSize(m_unSampleWidth, m_unSampleHeight) );

done:
    return hr;
}

HRESULT CSampleOutputWindow::SetOutputSample(IMFSample* pSample)
{
    HRESULT hr = S_OK;
    
    RECT rectClient;
    GetClientRect(&rectClient);
    
    m_spSample = pSample;
    
    if(m_unSampleWidth != rectClient.right - rectClient.left && m_unSampleHeight != rectClient.bottom - rectClient.top)
    {
        m_spResizedSample.Release();
        CHECK_HR( hr = m_pResizer->ResizeSample(pSample, &m_spResizedSample) );
    }
    else
    {
        m_spResizedSample = pSample;
    }

    Invalidate();

done:
    return hr;
}

HRESULT CSampleOutputWindow::GetCurrentSample(IMFSample** ppSample)
{
    *ppSample = m_spSample;
    (*ppSample)->AddRef();
    
    return S_OK;
}

void CSampleOutputWindow::SetEventListener(CWindowEventListener* pEventListener)
{
    m_pEventListener = pEventListener;
}

void CSampleOutputWindow::Reset()
{
    delete m_pResizer;
    RECT rectClient;
    GetClientRect(&rectClient);
    m_pResizer = new CSampleResizer();
    m_pResizer->SetOutputSampleSize(rectClient.right, rectClient.bottom);
}

void CSampleOutputWindow::SetSelection(UINT32 unSelectionLeft, UINT32 unSelectionTop, UINT32 unSelectionRight, UINT32 unSelectionBottom)
{
    RECT rectClient;
    GetClientRect(&rectClient);
    
    m_unSelectionLeft = unSelectionLeft * rectClient.right / m_unSampleWidth;
    m_unSelectionTop = unSelectionTop * rectClient.bottom / m_unSampleHeight;
    m_unSelectionRight = unSelectionRight * rectClient.right / m_unSampleWidth;
    m_unSelectionBottom = unSelectionBottom * rectClient.bottom / m_unSampleHeight;
    
    UINT32 unWidth = (UINT32) rectClient.right;
    UINT32 unHeight = (UINT32) rectClient.bottom;
    
    if(m_unSelectionLeft > unWidth) m_unSelectionLeft = rectClient.right;
    if(m_unSelectionRight > unWidth) m_unSelectionRight = rectClient.right;
    if(m_unSelectionTop > unHeight) m_unSelectionTop = rectClient.bottom;
    if(m_unSelectionBottom > unHeight) m_unSelectionBottom = rectClient.bottom;
    
    m_spResizedSample.Release();
    m_pResizer->ResizeSample(m_spSample, &m_spResizedSample);
    DrawSelection(m_spResizedSample, rectClient.right, rectClient.bottom);
    
    Invalidate();
}

LRESULT CSampleOutputWindow::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HRESULT hr = S_OK;
    CComPtr<IMFMediaBuffer> spBuffer;

    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(&ps);

    RECT rectClientArea;
    GetClientRect(&rectClientArea);

    if(NULL == m_spResizedSample.p)
    {
        HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hDC, &rectClientArea, hBrush);
        DeleteObject(hBrush);

        goto done;
    }

    UINT32 unWindowWidth = rectClientArea.right - rectClientArea.left;
    UINT32 unWindowHeight = rectClientArea.bottom - rectClientArea.top;

    CHECK_HR( hr = m_spResizedSample->GetBufferByIndex(0, &spBuffer) );

    BYTE* pbBuffer;
    DWORD cbMaxLength;
    DWORD cbCurrentLength;
    CHECK_HR( hr = spBuffer->Lock(&pbBuffer, &cbMaxLength, &cbCurrentLength) );
    // No returns or goto cleanups from this point on; media buffer must be unlocked.
    
    BYTE* pbBufferCopy = new BYTE[cbCurrentLength];
    CopyMemory(pbBufferCopy, pbBuffer, cbCurrentLength);

    HDC hMemDC = CreateCompatibleDC(hDC);
    HBITMAP hMemBM = CreateBitmap(unWindowWidth, unWindowHeight, 1, 32, pbBufferCopy);
    SelectObject(hMemDC, hMemBM);

    CHECK_HR( hr = spBuffer->Unlock() );

    BitBlt(hDC, 0, 0, unWindowWidth, unWindowHeight, hMemDC, 0, 0, SRCCOPY);

    DeleteDC(hMemDC);
    DeleteObject(hMemBM);
    delete[] pbBufferCopy;
    
done:
    EndPaint(&ps);
    return 0;
}

LRESULT CSampleOutputWindow::OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    bHandled = true;
    
    return 1;
}

LRESULT CSampleOutputWindow::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    OnSize(uMsg, wParam, lParam, bHandled);
    return 0;
}

LRESULT CSampleOutputWindow::OnWindowPosChanging(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return 0;
}

LRESULT CSampleOutputWindow::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RECT rectClient;
    GetClientRect(&rectClient);
    m_pResizer->SetOutputSampleSize(rectClient.right - rectClient.left, rectClient.bottom - rectClient.top);
    
    if(m_pEventListener) m_pEventListener->HandleResize();
    
    if(m_spSample.p)
    {
        m_spResizedSample.Release();
        m_pResizer->ResizeSample(m_spSample, &m_spResizedSample);
    }
    
    return 0;
}

HRESULT CSampleOutputWindow::DrawSelection(IMFSample* pSample, UINT32 unWidth, UINT32 unHeight)
{
    HRESULT hr;
    CComPtr<IMFMediaBuffer> spBuffer;
    
    CHECK_HR( hr = pSample->GetBufferByIndex(0, &spBuffer) );

    BYTE* pbBuffer;
    DWORD cbMaxLength;
    DWORD cbCurrentLength;
    CHECK_HR( hr = spBuffer->Lock(&pbBuffer, &cbMaxLength, &cbCurrentLength) );
    
    UINT32* punBuffer = (UINT32*) pbBuffer;
    for(UINT32 i = 0; i < m_unSelectionTop; i++)
    {
        for(UINT32 j = 0; j < unWidth; j++)
        {
            SHADE_PIXEL(*punBuffer);
            punBuffer++;
        }
    }
    
    for(UINT32 i = m_unSelectionTop; i < m_unSelectionBottom; i++)
    {
        for(UINT32 j = 0; j < unWidth; j++)
        {
            if(j < m_unSelectionLeft || j > m_unSelectionRight)
            {
                SHADE_PIXEL(*punBuffer);
            }
            
            punBuffer++;
        }
    }
    
    for(UINT32 i = m_unSelectionBottom; i < unHeight; i++)
    {
        for(UINT32 j = 0; j < unWidth; j++)
        {
            SHADE_PIXEL(*punBuffer);
            punBuffer++;
        }
    }
    
    CHECK_HR( hr = spBuffer->Unlock() );
    
done:
    return hr;
}