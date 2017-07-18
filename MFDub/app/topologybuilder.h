#pragma once

class CTopologyBuilder
{
public:
    CTopologyBuilder();
    ~CTopologyBuilder();
    
    HRESULT LoadSource(CAtlString strSourceURL);
    HRESULT AddAudioTransform(IUnknown* pTransform);
    HRESULT AddVideoTransform(IUnknown* pTransform);
    HRESULT SetAudioStreamSink(IUnknown* pStreamSink, UINT32 unStreamID);
    HRESULT SetVideoStreamSink(IUnknown* pStreamSink, UINT32 unStreamID);
    
    HRESULT GetTopology(IMFTopology** ppTopology);
    HRESULT GetSource(IMFMediaSource** ppSource);
    bool HasAudioStream() { return m_fAudioStream; }
    bool HasVideoStream() { return m_fVideoStream; }
    
private:
    CComPtr<IMFMediaSource> m_spSource;
    CComPtr<IMFTopology> m_spTopology;
    CComPtr<IMFTopologyNode> m_spLastAudioNode;
    CComPtr<IMFTopologyNode> m_spLastVideoNode;
    CComPtr<IMFTopologyNode> m_spAudioStreamSinkNode;
    CComPtr<IMFTopologyNode> m_spVideoStreamSinkNode;
    bool m_fAudioStream;
    bool m_fVideoStream;
};