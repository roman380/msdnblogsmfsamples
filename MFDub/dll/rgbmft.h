#pragma once

#include "basemft.h"
#include "rgbtransform.h"
#include "resource.h"
#include "mfveutil.h"
#include "sampleoutputwindow.h"

// {4EF1157C-791E-4838-840A-C892110D756C}
DEFINE_GUID(CLSID_CHistogramEqualizationMFT, 0x4ef1157c, 0x791e, 0x4838, 0x84, 0xa, 0xc8, 0x92, 0x11, 0xd, 0x75, 0x6c);
// {B1CFA1FE-9A17-4a47-94E9-FF8207321116}
DEFINE_GUID(CLSID_CNoiseRemovalMFT, 0xb1cfa1fe, 0x9a17, 0x4a47, 0x94, 0xe9, 0xff, 0x82, 0x7, 0x32, 0x11, 0x16);
// {55CD9E29-45E9-4bf3-8503-50E3C8AC4B5B}
DEFINE_GUID(CLSID_CUnsharpMaskMFT, 0x55cd9e29, 0x45e9, 0x4bf3, 0x85, 0x3, 0x50, 0xe3, 0xc8, 0xac, 0x4b, 0x5b);
// {70B44AAE-A6BE-4cd3-A4B4-A7567220B061}
DEFINE_GUID(CLSID_CResizeCropMFT, 0x70b44aae, 0xa6be, 0x4cd3, 0xa4, 0xb4, 0xa7, 0x56, 0x72, 0x20, 0xb0, 0x61);

class CRgbMFT
    : public AMFTransform
{
public:
    CRgbMFT(CRGBImageTransform* pImageTransform);
    virtual ~CRgbMFT();

    virtual HRESULT STDMETHODCALLTYPE GetStreamIDs(DWORD dwInputIDArraySize, DWORD *pdwInputIDs, DWORD dwOutputIDArraySize, DWORD *pdwOutputIDs);
    virtual HRESULT STDMETHODCALLTYPE GetInputStatus(DWORD dwInputStreamID, DWORD *pdwFlags);
    virtual HRESULT STDMETHODCALLTYPE GetOutputStatus(DWORD *pdwFlags);
    virtual HRESULT STDMETHODCALLTYPE ProcessMessage(MFT_MESSAGE_TYPE eMessage, ULONG_PTR ulParam);
    virtual HRESULT STDMETHODCALLTYPE ProcessInput(DWORD dwInputStreamID, IMFSample* pSample, DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE ProcessOutput(DWORD dwFlags, DWORD cOutputBufferCount, MFT_OUTPUT_DATA_BUFFER* pOutputSamples, DWORD *pdwStatus);

private:
    CComPtr<IMFSample> m_spCurrentSample;
    CRGBImageTransform* m_pImageTransform;
};

////////////////////////////////////////////////

class CHistogramEqualizationMFT
    : public CComObjectRoot
    , public CComCoClass<CHistogramEqualizationMFT, &CLSID_CHistogramEqualizationMFT>
    , public CRgbMFT
{
public:
    CHistogramEqualizationMFT();
    ~CHistogramEqualizationMFT();

    BEGIN_COM_MAP(CHistogramEqualizationMFT)
        COM_INTERFACE_ENTRY(IMFTransform)
    END_COM_MAP()

    DECLARE_REGISTRY_RESOURCEID(IDR_HISTEQUAL)
    DECLARE_NOT_AGGREGATABLE(CHistogramEqualizationMFT)
    DECLARE_CLASSFACTORY()

private:
    CComPtr<IMFSample> m_spCurrentSample;
    CHistogramEqualizationTransform m_HistEqualTransform;
};

///////////////////////////////////////////

class CNoiseRemovalMFT
    : public CComObjectRoot
    , public CComCoClass<CNoiseRemovalMFT, &CLSID_CNoiseRemovalMFT>
    , public CRgbMFT
{
public:
    CNoiseRemovalMFT();
    ~CNoiseRemovalMFT();

    BEGIN_COM_MAP(CNoiseRemovalMFT)
        COM_INTERFACE_ENTRY(IMFTransform)
    END_COM_MAP()

    DECLARE_REGISTRY_RESOURCEID(IDR_NOISEREMOVAL)
    DECLARE_NOT_AGGREGATABLE(CNoiseRemovalMFT)
    DECLARE_CLASSFACTORY()

private:
    CComPtr<IMFSample> m_spCurrentSample;
    CNoiseRemovalTransform m_NoiseRemovalTransform;
};

///////////////////////////////////////////

class CUnsharpMaskMFT
    : public CComObjectRoot
    , public CComCoClass<CUnsharpMaskMFT, &CLSID_CUnsharpMaskMFT>
    , public CRgbMFT
    , public IMFTConfiguration
{
public:
    CUnsharpMaskMFT();
    ~CUnsharpMaskMFT();

    BEGIN_COM_MAP(CUnsharpMaskMFT)
        COM_INTERFACE_ENTRY(IMFTransform)
        COM_INTERFACE_ENTRY(IMFTConfiguration)
    END_COM_MAP()

    DECLARE_REGISTRY_RESOURCEID(IDR_UNSHARPMASK)
    DECLARE_NOT_AGGREGATABLE(CUnsharpMaskMFT)
    DECLARE_CLASSFACTORY()

    HRESULT STDMETHODCALLTYPE GetAttributes(IMFAttributes** ppAttributes);
        
    // IMFTConfiguration
    HRESULT STDMETHODCALLTYPE GetFriendlyName(__out LPWSTR* pszFriendlyName);
    HRESULT STDMETHODCALLTYPE QueryRequiresConfiguration(__out BOOL* pfRequiresConfiguration);
    HRESULT STDMETHODCALLTYPE Configure(LONG_PTR hWnd, __in IMFSample* pExampleSample, __in IMFMediaType* pSampleMediaType);
    HRESULT STDMETHODCALLTYPE CloneMFT(__out IMFTConfiguration** ppClonedTransform);
    HRESULT STDMETHODCALLTYPE GetParamCount(__out DWORD* pcParams);
    HRESULT STDMETHODCALLTYPE GetParam(DWORD dwIndex, __deref_out LPWSTR* pszName, __out MFVE_PARAM_TYPE* pParamType, __out GUID* pgidKey);
    HRESULT STDMETHODCALLTYPE GetParamConstraint(DWORD dwIndex, __out PROPVARIANT* pvarMin, __out PROPVARIANT* pvarMaxs, __out PROPVARIANT* pvarStep);
    HRESULT STDMETHODCALLTYPE GetParamValues(DWORD dwIndex, __out DWORD* pcValues, __out PROPVARIANT** ppvarValues);
    HRESULT STDMETHODCALLTYPE UpdateConfiguration();

protected:
    CUnsharpMaskMFT(float gamma);
    
private:
    CComPtr<IMFSample> m_spCurrentSample;
    CUnsharpTransform m_UnsharpTransform;
    CComPtr<IMFAttributes> m_spAttributes;
};

class CUnsharpMaskConfigurationDialog
    : public CDialogImpl<CUnsharpMaskConfigurationDialog>
{
public:
    enum { IDD = IDD_UNSHARPMASK };
    
    CUnsharpMaskConfigurationDialog(float StartGamma, IMFSample* pExampleSample, UINT32 unSampleWidth, UINT32 unSampleHeight);
    ~CUnsharpMaskConfigurationDialog();
    
    float GetChosenGamma();
    
protected:
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    float GetTextBoxGamma();
    HRESULT OutputExampleSample();
    
    BEGIN_MSG_MAP( CUnsharpMaskConfigurationDialog )
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_HSCROLL, OnHScroll)
        
        COMMAND_HANDLER(IDOK, 0, OnOK)
        COMMAND_HANDLER(IDCANCEL, 0, OnCancel)
    END_MSG_MAP()
    
private:
    float m_ChosenGamma;
    HWND m_hTextBox;
    HWND m_hSlider;
    CSampleOutputWindow* m_pSampleOutputWindow;
    CComPtr<IMFSample> m_spExampleSample;
    UINT32 m_unSampleWidth;
    UINT32 m_unSampleHeight;
    CUnsharpTransform m_Transform;
    
    const static float ms_MaxGamma;
    const static float ms_MinGamma;
};

/////////////////////////////////////////////////////////////////

class CResizeCropMFT
    : public CComObjectRoot
    , public CComCoClass<CResizeCropMFT, &CLSID_CResizeCropMFT>
    , public AMFTransform
    , public IMFTConfiguration
{
    BEGIN_COM_MAP(CResizeCropMFT)
        COM_INTERFACE_ENTRY(IMFTransform)
        COM_INTERFACE_ENTRY(IMFTConfiguration)
    END_COM_MAP()

    DECLARE_REGISTRY_RESOURCEID(IDR_RESIZECROP)
    DECLARE_NOT_AGGREGATABLE(CResizeCropMFT)
    DECLARE_CLASSFACTORY()
    
    CResizeCropMFT();
    virtual ~CResizeCropMFT();
    
    HRESULT STDMETHODCALLTYPE GetAttributes(IMFAttributes** ppAttributes);
    
    void HandleInputTypeSet();
    void HandleOutputTypeSet();
    
    // IMFTConfiguration
    HRESULT STDMETHODCALLTYPE GetFriendlyName(__out LPWSTR* pszFriendlyName);
    HRESULT STDMETHODCALLTYPE QueryRequiresConfiguration(__out BOOL* pfRequiresConfiguration);
    HRESULT STDMETHODCALLTYPE Configure(LONG_PTR hWnd, __in IMFSample* pExampleSample, __in IMFMediaType* pSampleMediaType);
    HRESULT STDMETHODCALLTYPE CloneMFT(__out IMFTConfiguration** ppClonedTransform);
    HRESULT STDMETHODCALLTYPE GetParamCount(__out DWORD* pcParams);
    HRESULT STDMETHODCALLTYPE GetParam(DWORD dwIndex, __deref_out LPWSTR* pszName, __out MFVE_PARAM_TYPE* pParamType, __out GUID* pgidKey);
    HRESULT STDMETHODCALLTYPE GetParamConstraint(DWORD dwIndex, __out PROPVARIANT* pvarMin, __out PROPVARIANT* pvarMaxs, __out PROPVARIANT* pvarStep);
    HRESULT STDMETHODCALLTYPE GetParamValues(DWORD dwIndex, __out DWORD* pcValues, __out PROPVARIANT** ppvarValues);
    
    virtual HRESULT STDMETHODCALLTYPE GetStreamIDs(DWORD dwInputIDArraySize, DWORD *pdwInputIDs, DWORD dwOutputIDArraySize, DWORD *pdwOutputIDs);
    virtual HRESULT STDMETHODCALLTYPE GetInputStatus(DWORD dwInputStreamID, DWORD *pdwFlags);
    virtual HRESULT STDMETHODCALLTYPE GetOutputStatus(DWORD *pdwFlags);
    virtual HRESULT STDMETHODCALLTYPE ProcessMessage(MFT_MESSAGE_TYPE eMessage, ULONG_PTR ulParam);
    virtual HRESULT STDMETHODCALLTYPE ProcessInput(DWORD dwInputStreamID, IMFSample* pSample, DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE ProcessOutput(DWORD dwFlags, DWORD cOutputBufferCount, MFT_OUTPUT_DATA_BUFFER* pOutputSamples, DWORD *pdwStatus);
    virtual HRESULT STDMETHODCALLTYPE UpdateConfiguration();

protected:
    CResizeCropMFT(UINT32 unOutputWidth, UINT32 unOutputHeight);
    HRESULT InitResizer(UINT32 unOutputWidth, UINT32 unOutputHeight, UINT32 unInputX, UINT32 unInputY, UINT32 unInputWidth, UINT32 unInputHeight);
    HRESULT SetCropParams(UINT32 unInputX, UINT32 unInputY, UINT32 unInputWidth, UINT32 unInputHeight);

private:
    CComPtr<IMFAttributes> m_spAttributes;
    UINT32 m_unSampleWidth;
    UINT32 m_unSampleHeight;
    CComPtr<IMFTransform> m_spResizer;
    bool m_fConfigured;
};

class CResizeCropDialog
    : public CDialogImpl<CResizeCropDialog>
{
public:
    enum { IDD = IDD_RESIZECROP };
    
    CResizeCropDialog(IMFSample* pExampleSample, UINT32 unSampleWidth, UINT32 unSampleHeight, UINT32 unInputLeft, UINT32 unInputTop, UINT32 unInputWidth, UINT32 unInputHeight);
    ~CResizeCropDialog();
    
    UINT32 GetOutputWidth() { return m_unOutputWidth; }
    UINT32 GetOutputHeight() { return m_unOutputHeight; }
    UINT32 GetInputX() { return m_unInputX; }
    UINT32 GetInputY() { return m_unInputY; }
    UINT32 GetInputWidth() { return m_unInputWidth; }
    UINT32 GetInputHeight() { return m_unInputHeight; }
    
protected:
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCropChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    UINT32 GetTextBoxValue(UINT32 unControlID);
    void SetTextBoxValue(UINT32 unControlID, UINT32 unValue);
    
    BEGIN_MSG_MAP( CResizeCropDialog )
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        
        COMMAND_HANDLER(IDOK, 0, OnOK)
        COMMAND_HANDLER(IDCANCEL, 0, OnCancel)
        COMMAND_HANDLER(IDC_SOURCELEFT, EN_CHANGE, OnCropChanged)
        COMMAND_HANDLER(IDC_SOURCETOP, EN_CHANGE, OnCropChanged)
        COMMAND_HANDLER(IDC_SOURCEWIDTH, EN_CHANGE, OnCropChanged)
        COMMAND_HANDLER(IDC_SOURCEHEIGHT, EN_CHANGE, OnCropChanged)
    END_MSG_MAP()
    
private:
    UINT32 m_unOutputWidth;
    UINT32 m_unOutputHeight;
    UINT32 m_unInputX;
    UINT32 m_unInputY;
    UINT32 m_unInputWidth;
    UINT32 m_unInputHeight;
    CComPtr<IMFSample> m_spExampleSample;
    UINT32 m_unSampleWidth;
    UINT32 m_unSampleHeight;
    CSampleOutputWindow* m_pSampleOutputWindow;
    bool m_fInitialized;
};
