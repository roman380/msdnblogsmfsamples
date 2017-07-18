// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE
//
// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "stdafx.h"

#include "sampleresizer.h"

#include <initguid.h>
DEFINE_GUID(CLSID_CResizerDMO, 0x1EA1EA14, 0x48F4, 0x4054, 0xAD, 0x1A, 0xE8, 0xAE, 0xE1, 0x0A, 0xC8, 0x05);

CSampleResizer::CSampleResizer()
{
}

CSampleResizer::~CSampleResizer()
{
}

// SetInputSampleSize
// Set the size of input samples to be resized.
HRESULT CSampleResizer::SetInputSampleSize(UINT32 unWidth, UINT32 unHeight)
{
    HRESULT hr = S_OK;
    CComPtr<IMFMediaType> spInputType;
    
    if(NULL == m_spResizer.p) CHECK_HR( hr = CreateResizer() );
    
    CHECK_HR( hr = MFCreateMediaType(&spInputType) );
    
    CHECK_HR( hr = spInputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video) );
    CHECK_HR( hr = spInputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32) );
    CHECK_HR( hr = MFSetAttributeSize(spInputType, MF_MT_FRAME_SIZE, unWidth, unHeight) );
    
    CHECK_HR( hr = m_spResizer->SetInputType(0, spInputType, 0) );
    
done:
    return hr;
}

// SetOutputSampleSize
// Set the desired size of output samples.
HRESULT CSampleResizer::SetOutputSampleSize(UINT32 unWidth, UINT32 unHeight)
{
    HRESULT hr = S_OK;
    CComPtr<IMFMediaType> spOutputType;
    
    if(NULL == m_spResizer.p) CHECK_HR( hr = CreateResizer() );
    
    m_unOutputWidth = unWidth;
    m_unOutputHeight = unHeight;
    
    CHECK_HR( hr = MFCreateMediaType(&spOutputType) );
    
    CHECK_HR( hr = spOutputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video) );
    CHECK_HR( hr = spOutputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32) );
    CHECK_HR( hr = MFSetAttributeSize(spOutputType, MF_MT_FRAME_SIZE, unWidth, unHeight) );
    
    CHECK_HR( hr = m_spResizer->SetOutputType(0, spOutputType, 0) );
    
done:
    return hr;
}

// ResizeSample
// Resize the sample pSampleIn at the input size to *ppSampleOut at the output size.
HRESULT CSampleResizer::ResizeSample(IMFSample* pSampleIn, IMFSample** ppSampleOut)
{
    HRESULT hr = S_OK;
    CComPtr<IMFMediaBuffer> spBuffer;

    CHECK_HR( hr = m_spResizer->ProcessInput(0, pSampleIn, 0) );

    UINT32 unImageSize;
    CHECK_HR( hr = MFCalculateImageSize(MFVideoFormat_RGB32, m_unOutputWidth, m_unOutputHeight, &unImageSize) );
    
    MFT_OUTPUT_DATA_BUFFER OutputSamples;
    OutputSamples.dwStreamID = 0;
    
    CHECK_HR( hr = MFCreateSample(&(OutputSamples.pSample)) );
    CHECK_HR( hr = MFCreateMemoryBuffer(unImageSize, &spBuffer) );
    CHECK_HR( hr = OutputSamples.pSample->AddBuffer(spBuffer) );
    
    OutputSamples.dwStatus = 0;
    OutputSamples.pEvents = NULL;

    DWORD dwStatus;
    CHECK_HR( hr = m_spResizer->ProcessOutput(0, 1, &OutputSamples, &dwStatus) );
    
    *ppSampleOut = OutputSamples.pSample;
    
done:
    if(FAILED(hr))
    {
        m_spResizer->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0);
    }

    return hr;
}

// CreateResizer
// Create an instance of the resizer.
HRESULT CSampleResizer::CreateResizer()
{
    HRESULT hr = S_OK;

    // To do resizing, MFDub uses the resizer hybrid DMO/MFT.  When using the MF pipeline this
    // transform can get inserted automatically when there is a frame size mismatch.  MFDub uses
    // this transform outside of the pipeline -- all transforms should support this.
    CHECK_HR( hr = CoCreateInstance(CLSID_CResizerDMO, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_spResizer)) );
    
done:
    return hr;
}