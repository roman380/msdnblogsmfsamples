#pragma once

class CTransformApplier
{
public:
    CTransformApplier();
    virtual ~CTransformApplier();
    
    HRESULT AddTransform(IMFTransform* pTransform, CAtlString strName, CLSID clsidTransform, HWND hWndParent, IMFSample* pExampleSample);
    HRESULT ProcessSample(IMFSample* pInputSample, IMFSample** ppOutSample);
    virtual void Reset();

    void RemoveTransformAtIndex(size_t index);
    void MoveTransform(size_t indexCurrent, size_t indexNew);
    
    size_t GetTransformCount() const;
    HRESULT CloneTransform(size_t index, IMFTransform** ppTransform);
    CAtlString GetTransformName(size_t index) const;
    CLSID GetTransformCLSID(size_t index) const;
    
    virtual void SetInputType(IMFMediaType* pInputType);

protected:
    HRESULT ApplyTransform(IMFTransform* pTransform, IMFSample* pInputSample, IMFSample** ppOutSample);
    HRESULT TryConfigureMFT(IMFTransform* pTransform, HWND hWndParent, IMFSample* pExampleSample, IMFMediaType* pSampleType);
    HRESULT RefreshMediaTypes();

    virtual HRESULT CreateMediaTypeFromAvailable(IMFMediaType* pUpType, IMFMediaType* pAvailableType, IMFMediaType** ppMediaType) = 0;
    virtual HRESULT NotifyNewOutputType(IMFMediaType* pOutputType) = 0;
    virtual HRESULT GetOutputSampleSize(IMFTransform* pTransform, IMFSample* pInputSample, UINT32& cbSample) = 0;
    
private:
     CInterfaceArray<IMFTransform> m_arrTransformQueue;
     CAtlArray<CAtlString> m_arrTransformNames;
     CAtlArray<CLSID> m_arrCLSIDs;
     CComPtr<IMFMediaType> m_spInputType;
};

class CVideoTransformApplier
    : public CTransformApplier
{
public:
    CVideoTransformApplier();
    ~CVideoTransformApplier();

    void Reset();
    void SetInputType(IMFMediaType* pInputType);
    UINT32 GetOutputWidth() { return m_unOutputWidth; }
    UINT32 GetOutputHeight() { return m_unOutputHeight; }

protected:
    HRESULT CreateMediaTypeFromAvailable(IMFMediaType* pUpType, IMFMediaType* pAvailableType, IMFMediaType** ppMediaType);
    HRESULT NotifyNewOutputType(IMFMediaType* pOutputType);
    HRESULT GetOutputSampleSize(IMFTransform* pTransform, IMFSample* pInputSample, UINT32& cbSample);

private:
    UINT32 m_unOutputWidth;
    UINT32 m_unOutputHeight;
};

class CAudioTransformApplier
    : public CTransformApplier
{
protected:
    HRESULT CreateMediaTypeFromAvailable(IMFMediaType* pUpType, IMFMediaType* pAvailableType, IMFMediaType** ppMediaType);
    HRESULT NotifyNewOutputType(IMFMediaType* pOutputType);
    HRESULT GetOutputSampleSize(IMFTransform* pTranform, IMFSample* pInputSample, UINT32& cbSample);
};