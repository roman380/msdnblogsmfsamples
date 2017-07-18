#pragma once

#include "basemft.h"
#include "resource.h"
#include "mfveutil.h"

// {F8A7E6A5-1E0B-4ec8-ADB6-23A44D65707D}
DEFINE_GUID(CLSID_CEdgeFinderMFT, 0xf8a7e6a5, 0x1e0b, 0x4ec8, 0xad, 0xb6, 0x23, 0xa4, 0x4d, 0x65, 0x70, 0x7d);

class CEdgeFinderMFT
    : public CComObjectRoot
    , public CComCoClass<CEdgeFinderMFT, &CLSID_CEdgeFinderMFT>
    , public AMFTransform
{
public:
    CEdgeFinderMFT();
    ~CEdgeFinderMFT();

    BEGIN_COM_MAP(CEdgeFinderMFT)
        COM_INTERFACE_ENTRY(IMFTransform)
    END_COM_MAP()

    DECLARE_REGISTRY_RESOURCEID(IDR_EDGEFINDER)
    DECLARE_NOT_AGGREGATABLE(CEdgeFinderMFT)
    DECLARE_CLASSFACTORY()

    HRESULT STDMETHODCALLTYPE GetStreamIDs(DWORD dwInputIDArraySize, DWORD *pdwInputIDs, DWORD dwOutputIDArraySize, DWORD *pdwOutputIDs);
    HRESULT STDMETHODCALLTYPE GetInputStatus(DWORD dwInputStreamID, DWORD *pdwFlags);
    HRESULT STDMETHODCALLTYPE GetOutputStatus(DWORD *pdwFlags);
    HRESULT STDMETHODCALLTYPE ProcessMessage(MFT_MESSAGE_TYPE eMessage, ULONG_PTR ulParam);
    HRESULT STDMETHODCALLTYPE ProcessInput(DWORD dwInputStreamID, IMFSample* pSample, DWORD dwFlags);
    HRESULT STDMETHODCALLTYPE ProcessOutput(DWORD dwFlags, DWORD cOutputBufferCount, MFT_OUTPUT_DATA_BUFFER* pOutputSamples, DWORD *pdwStatus);

protected:
    HRESULT FindEdges(IMFSample* pInputSample, IMFSample* pOutputSample);

private:
    CComPtr<IMFSample> m_spCurrentSample;
};
