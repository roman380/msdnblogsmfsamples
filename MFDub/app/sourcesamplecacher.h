#pragma once

class CSourceStreamReader
{
public:
    CSourceStreamReader();
    ~CSourceStreamReader();
    
    HRESULT LoadStream(IMFMediaStream* pStream);
    HRESULT GetSample(size_t nSample, IMFSample** ppSampleOut);
    
protected:
    METHODASYNCCALLBACK(OnStreamEvent, CSourceStreamReader);
    void OnStreamEvent(IMFAsyncResult* pResult);
    
private:
    CComPtr<IMFMediaStream> m_spStream;
    CInterfaceArray<IMFSample> m_spSamples;
};

class CSourceSampleCacher
{
public:
    CSourceSampleCacher();
    ~CSourceSampleCacher();
    
    HRESULT LoadSource(IMFMediaSource* pSource);
    HRESULT GetSample(size_t nSample, IMFSample** ppSampleOut);
    
protected:
    METHODASYNCCALLBACK(OnSourceEvent, CSourceSampleCacher);
    void OnSourceEvent(IMFAsyncResult* pResult);
    
private:
    CComPtr<IMFMediaSource> m_spCurrentSource;
    CComPtr<IMFPresentationDescriptor> m_spPD;
    CInterfaceArray<IMFSample> m_spSamples;
}