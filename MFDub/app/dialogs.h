#pragma once

#include "resource.h"
#include "mediatranscoder.h"

class CAddTransformDialog 
    : public CDialogImpl<CAddTransformDialog>
{
public:
    enum { IDD = IDD_ADDTRANSFORM };
    
    CAddTransformDialog(GUID gidCategory);
    
    CLSID GetChosenCLSID() const;
    CAtlStringW GetChosenName() const;

protected:
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnTransformSelectionChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    BEGIN_MSG_MAP( CAddTransformDialog )
       MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )
       MESSAGE_HANDLER( WM_CLOSE, OnClose )

       COMMAND_HANDLER(IDOK, 0, OnOK)
       COMMAND_HANDLER(IDCANCEL, 0, OnCancel)
       COMMAND_HANDLER(IDC_TRANSFORMLIST, LBN_SELCHANGE, OnTransformSelectionChange)
    END_MSG_MAP()

private:
    HWND m_hList;
    HWND m_hDesc;
    CAtlArray<CLSID> m_CLSIDs;
    CAtlArray<CAtlString> m_strNames;
    CAtlArray<CAtlString> m_strDescs;
    unsigned int m_nChosenIndex;
};

///////////////////

class CEncodeOptionsDialog
    : public CDialogImpl<CEncodeOptionsDialog>
{
public:
    enum { IDD = IDD_ENCODEOPTIONS };
    
    CEncodeOptionsDialog(int iQuality, double dbOriginalFrameRate);
    
    int GetChosenValue();
    bool IsFrameRateChanged() { return m_fFrameRateChanged; }
    double GetFrameRate() { return m_dbFrameRate; }
    
protected:
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCBRClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnVBRClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnValueChangeFinished(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnChangeFrameRateClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    BEGIN_MSG_MAP( CEncodeOptionsDialog )
       MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )
       MESSAGE_HANDLER( WM_CLOSE, OnClose )

       COMMAND_HANDLER(IDOK, 0, OnOK)
       COMMAND_HANDLER(IDCANCEL, 0, OnCancel)
       COMMAND_HANDLER(IDC_CBR, BN_CLICKED, OnCBRClicked)
       COMMAND_HANDLER(IDC_VALUE, EN_KILLFOCUS, OnValueChangeFinished)
       COMMAND_HANDLER(IDC_CHANGEFRAMERATE, BN_CLICKED, OnChangeFrameRateClicked)
    END_MSG_MAP()

    int GetValueAsInteger();
    double GetFrameRateAsDouble();
    
private:
    HWND m_hCBRButton;
    HWND m_hVBRButton;
    HWND m_hLabel;
    HWND m_hValue;
    HWND m_hFrameRate;
    int m_iValue;
    bool m_fFrameRateChanged;
    double m_dbFrameRate;
    double m_dbOriginalFrameRate;
};

//////////////////////

struct StringGuidPair
{
    GUID m_gidSubtype;
    LPCWSTR m_szName;
};

class CMetadataDialog
    : public CDialogImpl<CMetadataDialog>
{
public:
    enum { IDD = IDD_METADATA };
    
    CMetadataDialog(LPCWSTR szSourceURL, IMFSourceReader* pSourceReader, IMFMediaType* pAudioType, IMFMediaType* pVideoType, DWORD dwFrameCount, DWORD dwKeyframeCount);
    
protected:
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    BEGIN_MSG_MAP( CEncodeOptionsDialog )
       MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )
       MESSAGE_HANDLER( WM_CLOSE, OnClose )

       COMMAND_HANDLER(IDOK, 0, OnOK)
    END_MSG_MAP()

    void SetControlText(DWORD dwControlID, LPCWSTR szText);
    void SetControlUINT64(DWORD dwControlID, UINT64 unNum);
    void SetControlTime(DWORD dwControlID, UINT64 unNum);
    void SetControlUINT32(DWORD dwControlID, UINT32 unNum);
    void SetControlSizeText(DWORD dwControlID, UINT32 unWidth, UINT32 unHeight);
    void SetControlSubtype(DWORD dwControlID, GUID gidSubtype);
    void SetControlDouble(DWORD dwControlID, double val);
    
private:
    LPCWSTR m_szSourceURL;
    CComPtr<IMFSourceReader> m_spSourceReader;
    CComPtr<IMFMediaType> m_spAudioType;
    CComPtr<IMFMediaType> m_spVideoType;
    DWORD m_dwFrameCount;
    DWORD m_dwKeyframeCount;
    static const StringGuidPair ms_SubtypeToString[];
};
