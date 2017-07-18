// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE
//
// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "stdafx.h"

#include "topologybuilder.h"

CTopologyBuilder::CTopologyBuilder()
    : m_fAudioStream(false)
    , m_fVideoStream(false)
{
}

CTopologyBuilder::~CTopologyBuilder()
{
}

// LoadSource
// Resolve a source for the media file at strSourceURL.
HRESULT CTopologyBuilder::LoadSource(CAtlString strSourceURL)
{
    HRESULT hr;
    CComPtr<IMFSourceResolver> spResolver;
    CComPtr<IUnknown> spSrcUnk;
    CComPtr<IMFPresentationDescriptor> spPD;

    CHECK_HR( hr = MFCreateTopology(&m_spTopology) );
        
    MF_OBJECT_TYPE ObjectType;
    CHECK_HR( hr = MFCreateSourceResolver(&spResolver) );
    CHECK_HR( hr = spResolver->CreateObjectFromURL(strSourceURL, 0, NULL, &ObjectType, &spSrcUnk) );
    CHECK_HR( hr = spSrcUnk->QueryInterface(IID_PPV_ARGS(&m_spSource)) );
    
    CHECK_HR( hr = m_spSource->CreatePresentationDescriptor(&spPD) );

    DWORD dwSDCount;
    CHECK_HR( hr = spPD->GetStreamDescriptorCount(&dwSDCount) );

    for(DWORD i = 0; i < dwSDCount; i++)
    {
        CHECK_HR( hr = spPD->SelectStream(i) );

        CComPtr<IMFStreamDescriptor> spSD;
        BOOL fSelected;
        CHECK_HR( hr = spPD->GetStreamDescriptorByIndex(i, &fSelected, &spSD) );

        // Each source node needs to be set up with MF_TOPONODE_SOURCE,
        // MF_TOPONODE_PRESENTATION_DESCRIPTOR, and MF_TOPONODE_STREAM_DESCRIPTOR
        // for the MF pipeline to unambiguously identify what stream is associated
        // with the topology node.
        CComPtr<IMFTopologyNode> spSourceStreamNode;
        CHECK_HR( hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &spSourceStreamNode) );
        CHECK_HR( hr = spSourceStreamNode->SetUnknown(MF_TOPONODE_SOURCE, m_spSource) );
        CHECK_HR( hr = spSourceStreamNode->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, spPD) );
        CHECK_HR( hr = spSourceStreamNode->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, spSD) );
        CHECK_HR( hr = m_spTopology->AddNode(spSourceStreamNode) );
        
        CComPtr<IMFMediaTypeHandler> spMediaTypeHandler;
        CHECK_HR( hr = spSD->GetMediaTypeHandler(&spMediaTypeHandler) );

        CComPtr<IMFMediaType> spMediaType;
        CHECK_HR( hr = spMediaTypeHandler->GetCurrentMediaType(&spMediaType) );

        GUID gidMajorType;
        CHECK_HR( hr = spMediaType->GetMajorType(&gidMajorType) );
        if(gidMajorType == MFMediaType_Video)
        {
            m_spLastVideoNode = spSourceStreamNode;
            m_fVideoStream = true;
        }
        else if(gidMajorType == MFMediaType_Audio)
        {
            m_spLastAudioNode = spSourceStreamNode;
            m_fAudioStream = true;
        }
    }
    
done:
    return hr;
}

// AddAudioTransform
// Add an audio transform to the topology.
HRESULT CTopologyBuilder::AddAudioTransform(IUnknown* pTransform)
{
    HRESULT hr = S_OK;
    
    CComPtr<IMFTopologyNode> spNode;
    CHECK_HR( hr = MFCreateTopologyNode(MF_TOPOLOGY_TRANSFORM_NODE, &spNode) );
    CHECK_HR( hr = spNode->SetObject(pTransform) );
    CHECK_HR( hr = m_spTopology->AddNode(spNode) );
    
    CHECK_HR( hr = m_spLastAudioNode->ConnectOutput(0, spNode, 0) );
    
    m_spLastAudioNode = spNode;
    
    if(m_spAudioStreamSinkNode.p)
    {
        CHECK_HR( hr = m_spLastAudioNode->ConnectOutput(0, m_spAudioStreamSinkNode, 0) );
    }
    
done:
    return hr;  
}

// AddVideoTransform
// Add a video transform to the topology.
HRESULT CTopologyBuilder::AddVideoTransform(IUnknown* pTransform)
{
    HRESULT hr = S_OK;
    
    CComPtr<IMFTopologyNode> spNode;
    CHECK_HR( hr = MFCreateTopologyNode(MF_TOPOLOGY_TRANSFORM_NODE, &spNode) );
    CHECK_HR( hr = spNode->SetObject(pTransform) );
    CHECK_HR( hr = m_spTopology->AddNode(spNode) );
    
    CHECK_HR( hr = m_spLastVideoNode->ConnectOutput(0, spNode, 0) );
    
    m_spLastVideoNode = spNode;
    
    if(m_spVideoStreamSinkNode.p)
    {
        CHECK_HR( hr = m_spLastVideoNode->ConnectOutput(0, m_spVideoStreamSinkNode, 0) );
    }
    
done:
    return hr;
}

// SetAudioStreamSink
// Set the sink for the audio stream.
HRESULT CTopologyBuilder::SetAudioStreamSink(IUnknown* pStreamSink, UINT32 unStreamID)
{
    HRESULT hr = S_OK;
    
    CComPtr<IMFTopologyNode> spNode;
    CHECK_HR( hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &spNode) );
    CHECK_HR( hr = spNode->SetObject(pStreamSink) )

    // Setting the stream ID here is important if the sink could be an
    // activate.  The MF media session uses the stream ID to identify
    // which of the stream sinks correspond to this node after activating
    // the activate object.
    CHECK_HR( hr = spNode->SetUINT32(MF_TOPONODE_STREAMID, unStreamID) );
    CHECK_HR( hr = m_spTopology->AddNode(spNode) );
    
    CHECK_HR( hr = m_spLastAudioNode->ConnectOutput(0, spNode, 0) );

    m_spAudioStreamSinkNode = spNode;
    
done:
    return hr;
}

// SetVideoStreamSink
// Set the sink for the video stream.
HRESULT CTopologyBuilder::SetVideoStreamSink(IUnknown* pStreamSink, UINT32 unStreamID)
{
    HRESULT hr = S_OK;
    
    CComPtr<IMFTopologyNode> spNode;
    CHECK_HR( hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &spNode) );
    CHECK_HR( hr = spNode->SetObject(pStreamSink) );

    // Setting the stream ID here is important if the sink could be an
    // activate.  The MF media session uses the stream ID to identify
    // which of the stream sinks correspond to this node after activating
    // the activate object.
    CHECK_HR( hr = spNode->SetUINT32(MF_TOPONODE_STREAMID, unStreamID) );
    CHECK_HR( hr = m_spTopology->AddNode(spNode) );
    
    CHECK_HR( hr = m_spLastVideoNode->ConnectOutput(0, spNode, 0) );

    m_spVideoStreamSinkNode = spNode;
    
done:
    return hr;
}
    
// GetTopology
// Return a pointer to the built topology in *ppTopology.
HRESULT CTopologyBuilder::GetTopology(IMFTopology** ppTopology)
{
    *ppTopology = m_spTopology;
    (*ppTopology)->AddRef();
    
    return S_OK;
}

// GetSource
// Return a pointer to the topology source in *ppSource.
HRESULT CTopologyBuilder::GetSource(IMFMediaSource** ppSource)
{
    *ppSource = m_spSource;
    (*ppSource)->AddRef();
    
    return S_OK;
}
