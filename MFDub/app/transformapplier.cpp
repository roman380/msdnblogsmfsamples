// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE
//
// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "stdafx.h"

#include "transformapplier.h"
#include "mfveutil.h"
#include "mferror.h"
#include "intsafe.h"

CTransformApplier::CTransformApplier()
{
}

CTransformApplier::~CTransformApplier()
{
}

// AddTransform
// Add a new transform to the transform chain.
HRESULT CTransformApplier::AddTransform(IMFTransform* pTransform, CAtlString strName, CLSID clsidTransform, HWND hWndParent, IMFSample* pExampleSample)
{
    HRESULT hr;
    CComPtr<IMFMediaType> spUpType;
    CComPtr<IMFMediaType> spMediaTypeAv;
    CComPtr<IMFMediaType> spMediaType;
    
    // If this is the first transform in the chain, the input type to
    // the transform is the entire chain's input type.  Otherwise,
    // the transform's input type is the output type of the previous
    // transform.
    if(m_arrTransformQueue.IsEmpty())
    {
        spUpType = m_spInputType;
    }
    else
    {
        CHECK_HR( hr = m_arrTransformQueue[m_arrTransformQueue.GetCount() - 1]->GetOutputCurrentType(0, &spUpType) );
    }

    CHECK_HR( hr = TryConfigureMFT(pTransform, hWndParent, pExampleSample, spUpType) );
 
    // Initialize input type on transform, setting input frame size to the size of 
    // the last transform in the queue.
    // This is a very basic form of type resolution that works for MFDub purposes.  A
    // generalized topology loader would need to be concerned about mismatches in media
    // types requiring a converter, resolving partial media types into complete media
    // types, and a host of other issues.
    CHECK_HR( hr = pTransform->GetInputAvailableType(0, 0, &spMediaTypeAv) );
    CHECK_HR( hr = CreateMediaTypeFromAvailable(spUpType, spMediaTypeAv, &spMediaType) );
    CHECK_HR( hr = pTransform->SetInputType(0, spMediaType, 0) );
    
    spMediaTypeAv.Release();
    spUpType.Attach(spMediaType.Detach());
    CHECK_HR( hr = pTransform->GetOutputAvailableType(0, 0, &spMediaTypeAv) );
    CHECK_HR( hr = CreateMediaTypeFromAvailable(spUpType, spMediaTypeAv, &spMediaType) );
    CHECK_HR( hr = pTransform->SetOutputType(0, spMediaType, 0) );
    CHECK_HR( hr = NotifyNewOutputType(spMediaType) );
    
    m_arrTransformQueue.Add(pTransform);
    m_arrTransformNames.Add(strName);
    m_arrCLSIDs.Add(clsidTransform);
   
done:
    return hr;
}

// ProcessSample
// Feed a sample through the transform chain and return the outputted
// sample in *ppOutSample.
HRESULT CTransformApplier::ProcessSample(IMFSample* pInputSample, IMFSample** ppOutSample)
{
    HRESULT hr = S_OK;
    
    // Apply each transform in order.
    CComPtr<IMFSample> spCurrentSample = pInputSample;
    for(size_t i = 0; i < m_arrTransformQueue.GetCount(); i++)
    {
        IMFSample* pOutSample;
        CHECK_HR( hr = ApplyTransform(m_arrTransformQueue[i], spCurrentSample, &pOutSample) );
        spCurrentSample = pOutSample;
        pOutSample->Release();
    }
    
    *ppOutSample = spCurrentSample;
    (*ppOutSample)->AddRef();
   
done:
    return hr;
}

// Reset
// Remove all transforms, restoring the transform chain to its default state.
void CTransformApplier::Reset()
{
    m_arrTransformQueue.RemoveAll();
    m_arrTransformNames.RemoveAll();
    m_arrCLSIDs.RemoveAll();
}

// GetTransformCount
// Return the number of transforms in the transform chain.
size_t CTransformApplier::GetTransformCount() const
{
    return m_arrTransformNames.GetCount();
}

// CloneTransform
// Create a copy of the transform at the specified index, returning it
// in *ppTransform.
HRESULT CTransformApplier::CloneTransform(size_t index, IMFTransform** ppTransform)
{
    HRESULT hr;
    CComPtr<IMFTransform> spTransform;
    CComPtr<IMFTConfiguration> spMFTConfiguration;
    
    // IMFTConfiguration is an MFDub-defined interface.  This interface defines a specific MFT cloning call
    // that clones the MFT configuration as well as the MFT itself.  If this interface is not defined,
    // then just creating another MFT with the same CLSID will have to do.
    hr = m_arrTransformQueue.GetAt(index)->QueryInterface(IID_PPV_ARGS(&spMFTConfiguration));
    if(E_NOINTERFACE == hr)
    {
        CHECK_HR( hr = CoCreateInstance(GetTransformCLSID(index), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&spTransform)) );
    }
    else if(S_OK == hr)
    {
        CComPtr<IMFTConfiguration> spClonedMFTConfig;
        CHECK_HR( hr = spMFTConfiguration->CloneMFT(&spClonedMFTConfig) );
        CHECK_HR( hr = spClonedMFTConfig->QueryInterface(IID_PPV_ARGS(&spTransform)) );
    }
    
    *ppTransform = spTransform.p;
    (*ppTransform)->AddRef();

done:
    return hr;
}

// GetTransformName
// Return the friendly name of the transform at index.
CAtlString CTransformApplier::GetTransformName(size_t index) const
{
    return m_arrTransformNames.GetAt(index);
}

// GetTransformCLSID
// Return the CLSID of the transform at index.
CLSID CTransformApplier::GetTransformCLSID(size_t index) const
{
    return m_arrCLSIDs.GetAt(index);
}

// RemoveTransformAtIndex
// Remove the transform at index from the transform chain.  This
// requires resolving the media types again.
void CTransformApplier::RemoveTransformAtIndex(size_t index)
{
    m_arrTransformQueue.RemoveAt(index);
    m_arrTransformNames.RemoveAt(index);
    m_arrCLSIDs.RemoveAt(index);
    
    RefreshMediaTypes();
}

// MoveTransform
// Move the transform in the transform chain at position indexCurrent
// to new position indexNew.  This requires resolving the media types
// again.
void CTransformApplier::MoveTransform(size_t indexCurrent, size_t indexNew)
{
    CComPtr<IMFTransform> spTransform = m_arrTransformQueue.GetAt(indexCurrent);
    CAtlString strTransformName = m_arrTransformNames.GetAt(indexCurrent);
    CLSID clsidTransform = m_arrCLSIDs.GetAt(indexCurrent);
    
    m_arrTransformQueue.RemoveAt(indexCurrent);
    m_arrTransformNames.RemoveAt(indexCurrent);
    m_arrCLSIDs.RemoveAt(indexCurrent);
    
    m_arrTransformQueue.InsertAt(indexNew, spTransform);
    m_arrTransformNames.InsertAt(indexNew, strTransformName);
    m_arrCLSIDs.InsertAt(indexNew, clsidTransform);
    
    RefreshMediaTypes();
}

// SetInputType
// Change the input type to the transform chain.  This requires
// resolving the media types again.
void CTransformApplier::SetInputType(IMFMediaType* pInputType)
{
    m_spInputType = pInputType;

    RefreshMediaTypes();
}

// ApplyTransform
// Transform the sample pInputSample using transform pTransform, storing the output in *ppOutSample.
HRESULT CTransformApplier::ApplyTransform(IMFTransform* pTransform, IMFSample* pInputSample, IMFSample** ppOutSample)
{
    HRESULT hr = S_OK;
    CComPtr<IMFMediaBuffer> spBuffer;
    UINT32 cbSample;

    // MFDub only uses 1-in 1-out transforms, which makes processing samples
    // simple.  MFDub also expects that transforms do no buffering of samples
    // -- so every ProcessInput should make it possible to call ProcessOutput
    // successfully.  Dynamic format changes genenerate an error.  
    // Generalized sample processing through transforms is much more
    // complicated.
    CHECK_HR( hr = pTransform->ProcessInput(0, pInputSample, 0) );

    CHECK_HR( hr = GetOutputSampleSize(pTransform, pInputSample, cbSample) );
    
    MFT_OUTPUT_DATA_BUFFER OutputSamples;
    OutputSamples.dwStreamID = 0;
        
    CHECK_HR( hr = MFCreateSample(&(OutputSamples.pSample)) );
    CHECK_HR( hr = MFCreateMemoryBuffer(cbSample, &spBuffer) );
    CHECK_HR( hr = OutputSamples.pSample->AddBuffer(spBuffer) );

    OutputSamples.dwStatus = 0;
    OutputSamples.pEvents = NULL;

    DWORD dwStatus;
    CHECK_HR( hr = pTransform->ProcessOutput(0, 1, &OutputSamples, &dwStatus) );
    
    *ppOutSample = OutputSamples.pSample;
  
done:
    return hr;
}

// TryConfigureMFT
// See if an MFT has special configuration and allow this MFT to configure itself if so.
HRESULT CTransformApplier::TryConfigureMFT(IMFTransform* pTransform, HWND hWndParent, IMFSample* pExampleSample, IMFMediaType* pSampleType)
{
    HRESULT hr;
    CComPtr<IMFTConfiguration> spMFTConfiguration;
    
    hr = pTransform->QueryInterface(IID_PPV_ARGS(&spMFTConfiguration));
    if(SUCCEEDED(hr))
    {
        BOOL fRequiresConfiguration;
        CHECK_HR( hr = spMFTConfiguration->QueryRequiresConfiguration(&fRequiresConfiguration) );
        
        if(fRequiresConfiguration)
        {
            CHECK_HR( hr = spMFTConfiguration->Configure((LONG_PTR) hWndParent, pExampleSample, pSampleType) );
        }
    }
    else if(FAILED(hr) && hr != E_NOINTERFACE)
    {
        CHECK_HR( hr = hr );
    }
    
    hr = S_OK;
    
done:
    return hr;
}

// RefreshMediaTypes
// Go through the transform chain and resolve media types again.
HRESULT CTransformApplier::RefreshMediaTypes()
{
    HRESULT hr = S_OK;
    CComPtr<IMFMediaType> spUpType;

    spUpType = m_spInputType;

    for(size_t i = 0; i < m_arrTransformQueue.GetCount(); i++)
    {
        CComPtr<IMFMediaType> spMediaTypeAv;
        CComPtr<IMFMediaType> spMediaType;
        CComPtr<IMFTransform> spTransform;

        spTransform = m_arrTransformQueue.GetAt(i);
        
        CHECK_HR( hr = spTransform->GetInputAvailableType(0, 0, &spMediaTypeAv) );
        CHECK_HR( hr = CreateMediaTypeFromAvailable(spUpType, spMediaTypeAv, &spMediaType) );
        CHECK_HR( hr = spTransform->SetInputType(0, spMediaType, 0) );

        spMediaTypeAv.Release();
        spUpType.Attach(spMediaType.Detach());
        CHECK_HR( hr = spTransform->GetOutputAvailableType(0, 0, &spMediaTypeAv) );
        CHECK_HR( hr = CreateMediaTypeFromAvailable(spUpType, spMediaTypeAv, &spMediaType) );
        CHECK_HR( hr = spTransform->SetOutputType(0, spMediaType, 0) );

        spUpType = spMediaType;
    }
    
    CHECK_HR( hr = NotifyNewOutputType(spUpType) );
    
done:
    return hr;
}

////////////////////////////////////////////////////////////////////

CVideoTransformApplier::CVideoTransformApplier()
: m_unOutputWidth(0)
, m_unOutputHeight(0)
{
}

CVideoTransformApplier::~CVideoTransformApplier()
{
}

// SetInputType
// In addition to the functionality provided by CTransformApplier,
// update the output frame size whenever the input type changes.
void CVideoTransformApplier::SetInputType(IMFMediaType* pInputType)
{
    CTransformApplier::SetInputType(pInputType);

    if(0 == m_unOutputWidth && 0 == m_unOutputHeight )
    {
        MFGetAttributeSize(pInputType, MF_MT_FRAME_SIZE, &m_unOutputWidth, &m_unOutputHeight);
    }    
}

// Reset
// Reset the output frame size as well.
void CVideoTransformApplier::Reset()
{
    CTransformApplier::Reset();
    m_unOutputWidth = 0;
    m_unOutputHeight = 0;
}

// CreateMediaTypeFromAvailable
// Fill in the frame size attribute when needed
HRESULT CVideoTransformApplier::CreateMediaTypeFromAvailable(IMFMediaType* pUpType, IMFMediaType* pAvailableType, IMFMediaType** ppMediaType)
{
    HRESULT hr;
    UINT32 unSetWidth, unSetHeight;
    UINT32 unUpWidth = 0;
    UINT32 unUpHeight = 0;

    CHECK_HR( hr = MFCreateMediaType(ppMediaType) );
    CHECK_HR( hr = pAvailableType->CopyAllItems(*ppMediaType) );
   
    MFGetAttributeSize(pUpType, MF_MT_FRAME_SIZE, &unUpWidth, &unUpHeight);
    hr = MFGetAttributeSize(*ppMediaType, MF_MT_FRAME_SIZE, &unSetWidth, &unSetHeight);
   
    // If the MFT already specifies the output frame size, do not override it.  
    if(MF_E_ATTRIBUTENOTFOUND == hr)
    {
        if(0 != unUpWidth && 0 != unUpHeight)
        {
            CHECK_HR( hr = MFSetAttributeSize(*ppMediaType, MF_MT_FRAME_SIZE, unUpWidth, unUpHeight) );
        }
        else
        {
            CHECK_HR( hr = MFSetAttributeSize(*ppMediaType, MF_MT_FRAME_SIZE, m_unOutputWidth, m_unOutputHeight) );
        }
    }
    hr = S_OK;
    
done:
    return hr;
}

// NotifyNewOutputType
// Update the output frame size.
HRESULT CVideoTransformApplier::NotifyNewOutputType(IMFMediaType* pOutputType)
{
    return MFGetAttributeSize(pOutputType, MF_MT_FRAME_SIZE, &m_unOutputWidth, &m_unOutputHeight);
}

// GetOutputSampleSize
// Query the transform to determine the buffer byte size it requires for a ProcessOutput call.
HRESULT CVideoTransformApplier::GetOutputSampleSize(IMFTransform* pTransform, IMFSample* pInputSample, UINT32& cbSample)
{
    HRESULT hr;
    MFT_OUTPUT_STREAM_INFO StreamInfo;

    CHECK_HR( hr = pTransform->GetOutputStreamInfo(0, &StreamInfo) );

    cbSample = StreamInfo.cbSize;

done:
    return hr;
}

////////////////////////////////////////////////////////////////////

// CreateMediaTypeFromAvailable
// Fill in required audio attributes as needed.
HRESULT CAudioTransformApplier::CreateMediaTypeFromAvailable(IMFMediaType* pUpType, IMFMediaType* pAvailableType, IMFMediaType** ppMediaType)
{
    HRESULT hr;
    UINT32 unAttrVal;

    CHECK_HR( hr = MFCreateMediaType(ppMediaType) );
    CHECK_HR( hr = pAvailableType->CopyAllItems(*ppMediaType) );

    if( FAILED((*ppMediaType)->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &unAttrVal)) )
    {
        CHECK_HR( hr = pUpType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &unAttrVal) );
        CHECK_HR( hr = (*ppMediaType)->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, unAttrVal) );
    }

    if( FAILED((*ppMediaType)->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &unAttrVal)) )
    {
        CHECK_HR( hr = pUpType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &unAttrVal) );
        CHECK_HR( hr = (*ppMediaType)->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, unAttrVal) );
    }

    if( FAILED((*ppMediaType)->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &unAttrVal)) )
    {
        CHECK_HR( hr = pUpType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &unAttrVal) );
        CHECK_HR( hr = (*ppMediaType)->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, unAttrVal) );
    }

done:
    return hr;
}

// NotifyNewOutputType
// No additional processing is currently needed for audio transform chains.
HRESULT CAudioTransformApplier::NotifyNewOutputType(IMFMediaType* pOutputType)
{
    return S_OK;
}

// GetOutputSampleSize
// Output sample size calculation is more complicated for audio transforms, since the output size returned from
// GetOutputStreamInfo is a minimum sample size.  For 16-bit samples at 2 channels, that means 4 bytes.  Processing
// in 4-byte chunks is rather inefficient, so determine a more reasonable output size.
HRESULT CAudioTransformApplier::GetOutputSampleSize(IMFTransform* pTransform, IMFSample* pInputSample, UINT32& cbSample)
{
    HRESULT hr;
    CComPtr<IMFMediaType> spOutputType;
    MFT_OUTPUT_STREAM_INFO StreamInfo;
    UINT32 nAvgBytesPerSecond;
    UINT32 nBlockAlign;
    MFTIME hnsDuration;
    UINT32 cChannels;
    UINT32 nBitsPerSample;

    CHECK_HR( hr = pTransform->GetOutputStreamInfo(0, &StreamInfo) );

    if(0 == StreamInfo.cbAlignment)
    {
        StreamInfo.cbAlignment = 1;
    }

    CHECK_HR( hr = pTransform->GetOutputCurrentType(0, &spOutputType) );

    CHECK_HR( hr = spOutputType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &cChannels) );
    CHECK_HR( hr = spOutputType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &nBitsPerSample) );

    // If the average bytes per second has already been calculated, use it.  Otherwise, it
    // can be calculated from the sample rate, channel count, and bits per sample.
    if( FAILED(spOutputType->GetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, &nAvgBytesPerSecond)) )
    {
        UINT32 nSamplesPerSecond;
        
        CHECK_HR( hr = spOutputType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &nSamplesPerSecond) );

        CHECK_HR( hr = UInt32Mult(nSamplesPerSecond, cChannels, &nAvgBytesPerSecond) );
        CHECK_HR( hr = UInt32Mult(nAvgBytesPerSecond, nBitsPerSample / 8, &nAvgBytesPerSecond) );
    }

    // Processing the entire input sample is the goal.  Determine how big of a buffer
    // is necessary to generate the input sample's duration worth of data at the output
    // data rate.  Note that this only works for uncompressed audio.  For compressed
    // audio, there is not a consistent correspondance between time units and data units.
    CHECK_HR( hr = pInputSample->GetSampleDuration(&hnsDuration) );
    cbSample = static_cast<DWORD>(hnsDuration * (LONGLONG) nAvgBytesPerSecond / 10000000);

    // If the MFT wants an alinged buffer, increase the buffer size as necessary.
    if(0 != cbSample % StreamInfo.cbAlignment)
    {
        cbSample = (cbSample / StreamInfo.cbAlignment + 1) * StreamInfo.cbAlignment;
    }

    // The buffer needs to be divisible by the block alignment.  Increase the buffer
    // size as necessary if it is not.
    if( FAILED(spOutputType->GetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, &nBlockAlign)) )
    {
        nBlockAlign = nBitsPerSample / 8 * cChannels;
    }

    if(0 != cbSample % nBlockAlign)
    {
        cbSample = (cbSample / nBlockAlign + 1) * nBlockAlign;
    }

    // It could turn out that after all this, the MFT still wants a larger buffer.  Give
    // it what it wants.
    if(cbSample < StreamInfo.cbSize)
    {
        cbSample = StreamInfo.cbSize;
    }

done:
    return hr;
}