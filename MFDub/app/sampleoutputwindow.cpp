// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE
//
// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "stdafx.h"

#include "sampleoutputwindow.h"

CSampleOutputWindow::CSampleOutputWindow()
    : m_pResizer(NULL)
    , m_pEventListener(NULL)
{
    m_pResizer = new CSampleResizer();
}

CSampleOutputWindow::~CSampleOutputWindow()
{
    delete m_pResizer;
}

// SetSampleSize
// Set the new video frame size that will be displayed in this window.  Update the
// resizer and potentially resize the window.
HRESULT CSampleOutputWindow::SetSampleSize(UINT32 unSampleWidth, UINT32 unSampleHeight, BOOL fResize)
{
    HRESULT hr = S_OK;
    
    m_unSampleWidth = unSampleWidth;
    m_unSampleHeight = unSampleHeight;

    if(fResize)
    {
        SetWindowPos(HWND_TOP, 0, 0, unSampleWidth, unSampleHeight, SWP_NOMOVE);
    }
    
    CHECK_HR( hr = m_pResizer->SetInputSampleSize(m_unSampleWidth, m_unSampleHeight) );

done:
    return hr;
}

// SetOutputSample
// Set the current sample to be displayed in this window.
HRESULT CSampleOutputWindow::SetOutputSample(IMFSample* pSample)
{
    HRESULT hr = S_OK;
    
    RECT rectClient;
    GetClientRect(&rectClient);
    
    m_spSample = pSample;
    
    // If the window size does not match the sample size, use the resizer to generate a new sample.
    if(m_unSampleWidth != rectClient.right - rectClient.left && m_unSampleHeight != rectClient.bottom - rectClient.top)
    {
        m_spResizedSample.Release();
        CHECK_HR( hr = m_pResizer->ResizeSample(pSample, &m_spResizedSample) );
    }
    else
    {
        m_spResizedSample = pSample;
    }

    HDC hDC = GetDC();
    DrawSample(hDC);
    ReleaseDC(hDC);

done:
    return hr;
}

// GetCurrentSample
// Return the currently displayed sample in *ppSample
HRESULT CSampleOutputWindow::GetCurrentSample(IMFSample** ppSample)
{
    *ppSample = m_spSample;
    (*ppSample)->AddRef();
    
    return S_OK;
}

// SetEventListener
// Set the handler that will receive resize notifications.
void CSampleOutputWindow::SetEventListener(CWindowEventListener* pEventListener)
{
    m_pEventListener = pEventListener;
}

// Reset
// Return to default size state.
void CSampleOutputWindow::Reset()
{
    delete m_pResizer;
    RECT rectClient;
    GetClientRect(&rectClient);
    m_pResizer = new CSampleResizer();
    m_pResizer->SetOutputSampleSize(rectClient.right, rectClient.bottom);
}

// OnPaint
// Handle the WM_PAINT message by redrawing the sample.
LRESULT CSampleOutputWindow::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(&ps);
    
    DrawSample(hDC);
    
    EndPaint(&ps);
    
    return 0;
}

// OnEraseBkgnd
// Ignore WM_ERASEBKGND.  Essentially, since the video sample takes up the whole window
// client area, it is the background.
LRESULT CSampleOutputWindow::OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    bHandled = true;
    
    return 1;
}

// OnWindowPosChanged
// Notify event handler of this change.
LRESULT CSampleOutputWindow::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    OnSize(uMsg, wParam, lParam, bHandled);
    if(m_pEventListener) m_pEventListener->HandleResize(this, reinterpret_cast<WINDOWPOS*>(lParam));
    return 0;
}

// OnWindowPosChanging
// Notify event handler of this change and allow it to update the window position.
LRESULT CSampleOutputWindow::OnWindowPosChanging(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if(m_pEventListener) m_pEventListener->CheckResize(this, reinterpret_cast<WINDOWPOS*>(lParam));

    return 0;
}

// OnSize
// Update the resizer to the new sample size, and regenerate the display sample.
LRESULT CSampleOutputWindow::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RECT rectClient;
    GetClientRect(&rectClient);

    // Resizer requires video height divisible by 2
    UINT32 unHeight = rectClient.bottom - rectClient.top;
    if(unHeight & 1)
    {
        unHeight--;
    }

    m_pResizer->SetOutputSampleSize(rectClient.right - rectClient.left, unHeight);
    
    if(m_spSample.p)
    {
        m_spResizedSample.Release();
        m_pResizer->ResizeSample(m_spSample, &m_spResizedSample);
    }
    
    return 0;
}

// DrawSample
// Draw the display sample to the window.
void CSampleOutputWindow::DrawSample(HDC hDC)
{
    HRESULT hr = S_OK;
    CComPtr<IMFMediaBuffer> spBuffer;

    RECT rectClientArea;
    GetClientRect(&rectClientArea);

    // If no sample has been provided yet, just fill the window with black.
    if(NULL == m_spResizedSample.p)
    {
        HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hDC, &rectClientArea, hBrush);
        DeleteObject(hBrush);

        goto done;
    }

    UINT32 unWindowWidth = rectClientArea.right - rectClientArea.left;

    // Resizer requires video height divisible by 2
    UINT32 unWindowHeight = rectClientArea.bottom - rectClientArea.top;
    if(unWindowHeight & 1)
    {
        unWindowHeight--;
    }

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
    return;
}