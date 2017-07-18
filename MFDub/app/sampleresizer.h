#pragma once

class CSampleResizer
{
public:
    CSampleResizer();
    ~CSampleResizer();
    
    HRESULT SetInputSampleSize(UINT32 unWidth, UINT32 unHeight);
    HRESULT SetOutputSampleSize(UINT32 unWidth, UINT32 unHeight);
    HRESULT ResizeSample(IMFSample* pSampleIn, IMFSample** ppSampleOut);

protected:
    HRESULT CreateResizer();
    
private:
    UINT32 m_unOutputWidth;
    UINT32 m_unOutputHeight;
    CComPtr<IMFTransform> m_spResizer;
};