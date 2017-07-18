// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE
//
// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "stdafx.h"

#include "basemft.h"
#include "edgefindermft.h"
#include "rgbtypehandler.h"
#include "mferror.h"
#include "rgbutils.h"
#include <string.h>

CEdgeFinderMFT::CEdgeFinderMFT()
{
    Init(new CRGB1in1outAutoCopyTypeHandler());
}

CEdgeFinderMFT::~CEdgeFinderMFT()
{
    delete PTypeHandler();
}

HRESULT CEdgeFinderMFT::GetStreamIDs(DWORD dwInputIDArraySize, DWORD *pdwInputIDs, DWORD dwOutputIDArraySize, DWORD *pdwOutputIDs)
{
    if(dwInputIDArraySize < 1 || dwOutputIDArraySize < 1)
    {
        return E_INVALIDARG;
    }

    if(NULL == pdwInputIDs || NULL == pdwOutputIDs)
    {
        return E_POINTER;
    }

    *pdwInputIDs = 0;
    *pdwOutputIDs = 0;

    return S_OK;
}

HRESULT CEdgeFinderMFT::GetInputStatus(DWORD dwInputStreamID, DWORD *pdwFlags)
{
    if( dwInputStreamID > 1 )
    {
        return( MF_E_INVALIDSTREAMNUMBER );
    }

    if( NULL == pdwFlags )
    {
        return( E_INVALIDARG );
    }

    *pdwFlags = MFT_INPUT_STATUS_ACCEPT_DATA;

    return( S_OK );
}

HRESULT CEdgeFinderMFT::GetOutputStatus(DWORD *pdwFlags)
{
    return E_NOTIMPL;
}

HRESULT CEdgeFinderMFT::ProcessMessage(MFT_MESSAGE_TYPE eMessage, ULONG_PTR ulParam)
{

    return S_OK;
}

HRESULT CEdgeFinderMFT::ProcessInput(DWORD dwInputStreamID, IMFSample* pSample, DWORD dwFlags)
{
    if(dwInputStreamID > 0)
    {
        return MF_E_INVALIDSTREAMNUMBER;
    }

    if(NULL != m_spCurrentSample.p)
    {
        return MF_E_NOTACCEPTING;
    }

    if(NULL == pSample)
    {
        return E_POINTER;
    }

    m_spCurrentSample = pSample;

    return S_OK;
}

HRESULT CEdgeFinderMFT::ProcessOutput(DWORD dwFlags, DWORD cOutputBufferCount, MFT_OUTPUT_DATA_BUFFER* pOutputSamples, DWORD *pdwStatus)
{
    HRESULT hr = S_OK;

    if(NULL == m_spCurrentSample.p)
    {
        return MF_E_TRANSFORM_NEED_MORE_INPUT;
    }

    if(NULL != pOutputSamples->pSample)
    {
        FindEdges(m_spCurrentSample, pOutputSamples->pSample);
        m_spCurrentSample.Release();
    }

    return hr;
}

// FindEdges
// Perform edge detection using a Sobel operator.  
HRESULT CEdgeFinderMFT::FindEdges(IMFSample* pInputSample, IMFSample* pOutputSample)
{
    HRESULT hr = S_OK;
    CComPtr<IMFMediaType> spType;

    LONGLONG hnsTime;
    hr = pInputSample->GetSampleTime(&hnsTime);
    if (SUCCEEDED(hr)) {
        pOutputSample->SetSampleTime(hnsTime);
    }

    LONGLONG hnsDuration;
    hr = pInputSample->GetSampleDuration(&hnsDuration);
    if (SUCCEEDED(hr)) {
        pOutputSample->SetSampleDuration(hnsDuration);
    }
    hr = S_OK;

    CComPtr<IMFMediaBuffer> spInBuffer;
    BYTE* pbInBuffer;
    DWORD cbInLength;
    CComPtr<IMFMediaBuffer> spOutBuffer;
    BYTE* pbOutBuffer;
    DWORD cbOutLength;
        
    CHECK_HR( hr = pInputSample->GetBufferByIndex(0, &spInBuffer) );
    spInBuffer->Lock(&pbInBuffer, &cbInLength, NULL);
       
    CHECK_HR( hr = pOutputSample->GetBufferByIndex(0, &spOutBuffer) );
    spOutBuffer->Lock(&pbOutBuffer, &cbOutLength, NULL);

    DWORD dwMinLength = (cbOutLength > cbInLength) ? cbInLength : cbOutLength;
    spOutBuffer->SetCurrentLength(dwMinLength);

    PTypeHandler()->GetInputCurrentType(0, &spType);

    UINT32 unWidth, unHeight;
    MFGetAttributeSize(spType, MF_MT_FRAME_SIZE, &unWidth, &unHeight);

    // Go through the image row by row.  Keep track of the previous row,
    // the current row, and the next row, as all three will be needed
    // to properly apply the Sobel operator.
    DWORD cBytesWritten = 0;
    RGBQUAD* pRowAbove = (RGBQUAD*) pbInBuffer;
    RGBQUAD* pRowCurrent = ((RGBQUAD*) pbInBuffer) + unWidth;
    RGBQUAD* pRowBelow = ((RGBQUAD*) pbInBuffer) + unWidth + unWidth;
    RGBQUAD* pRowOut = ((RGBQUAD*) pbOutBuffer) + unWidth;
    for(UINT32 i = 1; i < unHeight - 1; i++)
    {
        for(UINT32 j = 1; j < unWidth - 1; j++)
        {
            INT32 iVertRed = 0, iVertGreen = 0, iVertBlue = 0;
            INT32 iHorzRed = 0, iHorzGreen = 0, iHorzBlue = 0;
            
            RGBQUAD* pPixel = pRowAbove + j - 1;
            
            iVertRed -= pPixel->rgbRed;
            iVertGreen -= pPixel->rgbGreen;
            iVertBlue -= pPixel->rgbBlue;
            
            iHorzRed += pPixel->rgbRed;
            iHorzGreen += pPixel->rgbGreen;
            iHorzBlue += pPixel->rgbBlue;
            
            pPixel = pRowAbove + j;
            
            iHorzRed += 2 * pPixel->rgbRed;
            iHorzGreen += 2 * pPixel->rgbGreen;
            iHorzBlue += 2 * pPixel->rgbBlue;
            
            pPixel = pRowAbove + j + 1;
            
            iVertRed += pPixel->rgbRed;
            iVertGreen += pPixel->rgbGreen;
            iVertBlue += pPixel->rgbBlue;
            
            iHorzRed += pPixel->rgbRed;
            iHorzGreen += pPixel->rgbGreen;
            iHorzBlue += pPixel->rgbBlue;
            
            pPixel = pRowCurrent + j - 1;
            
            iVertRed -= 2 * pPixel->rgbRed;
            iVertGreen -= 2 * pPixel->rgbGreen;
            iVertBlue -= 2 * pPixel->rgbBlue;
            
            pPixel = pRowCurrent + j + 1;
            
            iVertRed += 2 * pPixel->rgbRed;
            iVertGreen += 2 * pPixel->rgbGreen;
            iVertBlue += 2 * pPixel->rgbBlue;
            
            pPixel = pRowBelow + j - 1;
            
            iVertRed -= pPixel->rgbRed;
            iVertGreen -= pPixel->rgbGreen;
            iVertBlue -= pPixel->rgbBlue;
            
            iHorzRed -= pPixel->rgbRed;
            iHorzGreen -= pPixel->rgbGreen;
            iHorzBlue -= pPixel->rgbBlue;
            
            pPixel = pRowBelow + j;
            
            iHorzRed -= 2 * pPixel->rgbRed;
            iHorzGreen -= 2 * pPixel->rgbGreen;
            iHorzBlue -= 2 * pPixel->rgbBlue;
            
            pPixel = pRowBelow + j + 1;
            
            iVertRed += pPixel->rgbRed;
            iVertGreen += pPixel->rgbGreen;
            iVertBlue += pPixel->rgbBlue;
            
            iHorzRed -= pPixel->rgbRed;
            iHorzGreen -= pPixel->rgbGreen;
            iHorzBlue -= pPixel->rgbBlue;
            
            RGBQUAD* pPixelOut = pRowOut + j;
            pPixelOut->rgbRed = abs(iVertRed) + abs(iHorzRed);
            pPixelOut->rgbGreen = abs(iVertGreen) + abs(iHorzGreen);
            pPixelOut->rgbBlue = abs(iVertBlue) + abs(iHorzBlue);
        }

        pRowAbove = pRowCurrent;
        pRowCurrent = pRowBelow;
        pRowBelow += unWidth;

        pRowOut += unWidth;
        
        cBytesWritten += unWidth * 4;
        if(cBytesWritten >= dwMinLength) break;
    }

done:
    if(spInBuffer) spInBuffer->Unlock();
    if(spOutBuffer) spOutBuffer->Unlock();

    return hr;
}