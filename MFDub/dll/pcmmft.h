#pragma once

#include "basemft.h"
#include "resource.h"
#include "mfveutil.h"
#include "pcmtransform.h"

// 1474a093-8d56-4e3e-a67a-dc43b0db808a
DEFINE_GUID(CLSID_CVolumeCompressionMFT, 0x1474a093, 0x8d56, 0x4e3e, 0xa6, 0x7a, 0xdc, 0x43, 0xb0, 0xdb, 0x80, 0x8a);

class CPcmMFT
    : public AMFTransform
{
public:
    CPcmMFT(CPCMSampleTransform* pImageTransform);
    virtual ~CPcmMFT();

    virtual HRESULT STDMETHODCALLTYPE GetStreamIDs(DWORD dwInputIDArraySize, DWORD *pdwInputIDs, DWORD dwOutputIDArraySize, DWORD *pdwOutputIDs);
    virtual HRESULT STDMETHODCALLTYPE GetInputStatus(DWORD dwInputStreamID, DWORD *pdwFlags);
    virtual HRESULT STDMETHODCALLTYPE GetOutputStatus(DWORD *pdwFlags);
    virtual HRESULT STDMETHODCALLTYPE ProcessMessage(MFT_MESSAGE_TYPE eMessage, ULONG_PTR ulParam);
    virtual HRESULT STDMETHODCALLTYPE ProcessInput(DWORD dwInputStreamID, IMFSample* pSample, DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE ProcessOutput(DWORD dwFlags, DWORD cOutputBufferCount, MFT_OUTPUT_DATA_BUFFER* pOutputSamples, DWORD *pdwStatus);

private:
    CComPtr<IMFSample> m_spCurrentSample;
    CPCMSampleTransform* m_pSampleTransform;
};

class CVolumeCompressionMFT
    : public CComObjectRoot
    , public CComCoClass<CVolumeCompressionMFT, &CLSID_CVolumeCompressionMFT>
    , public CPcmMFT
    , public IMFTConfiguration
{
public:
    CVolumeCompressionMFT();
    ~CVolumeCompressionMFT();

    BEGIN_COM_MAP(CVolumeCompressionMFT)
        COM_INTERFACE_ENTRY(IMFTransform)
        COM_INTERFACE_ENTRY(IMFTConfiguration)
    END_COM_MAP()

    DECLARE_REGISTRY_RESOURCEID(IDR_VOLCOMP)
    DECLARE_NOT_AGGREGATABLE(CVolumeCompressionMFT)
    DECLARE_CLASSFACTORY()

    HRESULT STDMETHODCALLTYPE GetAttributes(__out IMFAttributes** ppAttributes);

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

private:
    CVolumeCompressionTransform m_VolumeCompressionTransform;
    CComPtr<IMFAttributes> m_spAttributes;
};

class CVolumeCompressionDialog
    : public CDialogImpl<CVolumeCompressionDialog>
{
public:
    enum { IDD = IDD_VOLCOMP };
    
    CVolumeCompressionDialog(IMFSample* pExampleSample, float flFactor);
    ~CVolumeCompressionDialog();
    
    float GetCompressionFactor() { return _flFactor; }

protected:
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    BEGIN_MSG_MAP( CVolumeCompressionDialog )
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        
        COMMAND_HANDLER(IDOK, 0, OnOK)
        COMMAND_HANDLER(IDCANCEL, 0, OnCancel)
    END_MSG_MAP()
    
private:
    float _flFactor;
    CComPtr<IMFSample> _spExampleSample;
    bool _fInitialized;
};