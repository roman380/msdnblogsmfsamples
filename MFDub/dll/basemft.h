#pragma once

class AMFTTypeHandler
{
public:
    virtual HRESULT GetStreamCount(DWORD* pcInputStreams, DWORD* pcOutputStreams) = 0;
    virtual HRESULT GetStreamLimits(DWORD* pdwInputMinimum, DWORD* pdwInputMaximum, DWORD* pdwOutputMinimum, DWORD* pdwOutputMaximum) = 0;
    virtual HRESULT GetInputStreamInfo(DWORD dwInputStreamID, MFT_INPUT_STREAM_INFO* pStreamInfo) = 0;
    virtual HRESULT GetOutputStreamInfo(DWORD dwOutputStreamID, MFT_OUTPUT_STREAM_INFO* pStreamInfo) = 0;
    virtual HRESULT GetInputAvailableType(DWORD dwInputStreamID, DWORD dwTypeIndex, IMFMediaType** ppType) = 0;
    virtual HRESULT GetOutputAvailableType(DWORD dwOutputStreamID, DWORD dwTypeIndex, IMFMediaType** ppType) = 0;
    virtual HRESULT SetInputType(DWORD dwInputStreamID, IMFMediaType* pType, DWORD dwFlags) = 0;
    virtual HRESULT SetOutputType(DWORD dwOutputStreamID, IMFMediaType* pType, DWORD dwFlags) = 0;
    virtual HRESULT GetInputCurrentType(DWORD dwInputStreamID, IMFMediaType** ppType) = 0;
    virtual HRESULT GetOutputCurrentType(DWORD dwOutputStreamID, IMFMediaType** ppType) = 0;

protected:
    virtual void OnInputTypeChanged()
    {
    }

    virtual void OnOutputTypeChanged()
    {
    }
};

class C1in1outTypeHandler
    : public AMFTTypeHandler
{
public:
    C1in1outTypeHandler();
    virtual ~C1in1outTypeHandler();

    virtual HRESULT GetStreamCount(DWORD* pcInputStreams, DWORD* pcOutputStreams);
    virtual HRESULT GetStreamLimits(DWORD* pdwInputMinimum, DWORD* pdwInputMaximum, DWORD* pdwOutputMinimum, DWORD* pdwOutputMaximum);
    virtual HRESULT GetInputStreamInfo(DWORD dwInputStreamIndex, MFT_INPUT_STREAM_INFO* pStreamInfo);
    virtual HRESULT GetOutputStreamInfo(DWORD dwOutputStreamIndex, MFT_OUTPUT_STREAM_INFO* pStreamInfo);
    virtual HRESULT GetInputAvailableType(DWORD dwInputStreamIndex, DWORD dwTypeIndex, IMFMediaType** ppType);
    virtual HRESULT GetOutputAvailableType(DWORD dwOutputStreamIndex, DWORD dwTypeIndex, IMFMediaType** ppType);
    virtual HRESULT SetInputType(DWORD dwInputStreamIndex, IMFMediaType* pType, DWORD dwFlags);
    virtual HRESULT SetOutputType(DWORD dwOutputStreamIndex, IMFMediaType* pType, DWORD dwFlags);
    virtual HRESULT GetInputCurrentType(DWORD dwInputStreamIndex, IMFMediaType** ppType);
    virtual HRESULT GetOutputCurrentType(DWORD dwOutputStreamIndex, IMFMediaType** ppType);

protected:
    HRESULT SetInputAvTypeCount(DWORD cInputAvTypes);
    void SetInputAvType(DWORD dwIndex, IMFMediaType* pType);
    HRESULT SetOutputAvTypeCount(DWORD cOutputAvTypes);
    void SetOutputAvType(DWORD dwIndex, IMFMediaType* pType);
    
    IMFMediaType** m_pInputAvTypes;
    DWORD m_cInputAvTypes;
    IMFMediaType** m_pOutAvTypes;
    DWORD m_cOutputAvTypes;
    CComPtr<IMFMediaType> m_spInputType;
    CComPtr<IMFMediaType> m_spOutputType;
    DWORD m_cbInputSize;
    DWORD m_cbOutputSize;
    DWORD m_dwInputStreamFlags;
    DWORD m_dwOutputStreamFlags;
};

class AMFTransform
    : public IMFTransform
{
public:
    void Init(AMFTTypeHandler* pTypeHandler);

    HRESULT STDMETHODCALLTYPE GetStreamCount(DWORD* pcInputStreams, DWORD* pcOutputStreams);
    HRESULT STDMETHODCALLTYPE GetStreamLimits(DWORD* pdwInputMinimum, DWORD* pdwInputMaximum, DWORD* pdwOutputMinimum, DWORD* pdwOutputMaximum);
    HRESULT STDMETHODCALLTYPE GetInputStreamInfo(DWORD dwInputStreamID, MFT_INPUT_STREAM_INFO* pStreamInfo);
    HRESULT STDMETHODCALLTYPE GetOutputStreamInfo(DWORD dwOutputStreamID, MFT_OUTPUT_STREAM_INFO* pStreamInfo);
    HRESULT STDMETHODCALLTYPE GetInputAvailableType(DWORD dwInputStreamID, DWORD dwTypeIndex, IMFMediaType** ppType);
    HRESULT STDMETHODCALLTYPE GetOutputAvailableType(DWORD dwOutputStreamID, DWORD dwTypeIndex, IMFMediaType** ppType);
    HRESULT STDMETHODCALLTYPE SetInputType(DWORD dwInputStreamID, IMFMediaType* pType, DWORD dwFlags);
    HRESULT STDMETHODCALLTYPE SetOutputType(DWORD dwOutputStreamID, IMFMediaType* pType, DWORD dwFlags);
    HRESULT STDMETHODCALLTYPE GetInputCurrentType(DWORD dwInputStreamID, IMFMediaType** ppType);
    HRESULT STDMETHODCALLTYPE GetOutputCurrentType(DWORD dwOutputStreamID, IMFMediaType** ppType);

    virtual HRESULT STDMETHODCALLTYPE GetStreamIDs(DWORD dwInputIDArraySize, DWORD *pdwInputIDs, DWORD dwOutputIDArraySize, DWORD *pdwOutputIDs) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetAttributes(IMFAttributes **pAttributes)
    {
        return E_NOTIMPL;
    }
        
    virtual HRESULT STDMETHODCALLTYPE GetInputStreamAttributes(DWORD dwInputStreamID, IMFAttributes **pAttributes)
    {
        return E_NOTIMPL;
    }
        
    virtual HRESULT STDMETHODCALLTYPE GetOutputStreamAttributes(DWORD dwOutputStreamID, IMFAttributes **pAttributes)
    {
        return E_NOTIMPL;
    }
        
    virtual HRESULT STDMETHODCALLTYPE DeleteInputStream(DWORD dwStreamID)
    {
        return E_NOTIMPL;
    }
        
    virtual HRESULT STDMETHODCALLTYPE AddInputStreams(DWORD cStreams, DWORD *adwStreamIDs)
    {
        return E_NOTIMPL;
    }
        
    virtual HRESULT STDMETHODCALLTYPE GetInputStatus(DWORD dwInputStreamID, DWORD *pdwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetOutputStatus(DWORD *pdwFlags) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetOutputBounds(LONGLONG hnsLowerBound, LONGLONG hnsUpperBound)
    {
        return E_NOTIMPL;
    }
        
    virtual HRESULT STDMETHODCALLTYPE ProcessEvent(DWORD dwInputStreamID, IMFMediaEvent *pEvent)
    {
        return E_NOTIMPL;
    }
        
    virtual HRESULT STDMETHODCALLTYPE ProcessMessage(MFT_MESSAGE_TYPE eMessage, ULONG_PTR ulParam) = 0;

    virtual STDMETHODIMP ProcessInput(DWORD dwInputStreamID, IMFSample* pSample, DWORD dwFlags) = 0;
    virtual STDMETHODIMP ProcessOutput(DWORD dwFlags, DWORD cOutputBufferCount, MFT_OUTPUT_DATA_BUFFER* pOutputSamples, DWORD *pdwStatus) = 0;

protected:
    AMFTTypeHandler* PTypeHandler()
    {
        return m_pTypeHandler;
    }

private:
    AMFTTypeHandler* m_pTypeHandler;
};