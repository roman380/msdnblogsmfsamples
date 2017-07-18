#pragma once

class CStatic 
    : public CWindowImpl<CStatic> 
{
public:

    DECLARE_WND_SUPERCLASS(NULL, WC_STATIC);
protected:
    BEGIN_MSG_MAP(CStatic)
    END_MSG_MAP()
};

class CTransportToolbar
    : public CWindowImpl<CTransportToolbar>
{
public:
    CTransportToolbar();
    ~CTransportToolbar();
    
    void EnableButtonByCommand(UINT unID, BOOL fEnable);
    void SetSampleTimeLabelText(LPCWSTR szText);
    
    DECLARE_WND_SUPERCLASS(NULL, TOOLBARCLASSNAME)
protected:
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
    BEGIN_MSG_MAP(CTransportToolbar)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
    END_MSG_MAP()
    
private:
    CStatic m_SampleTimeLabel;
    HFONT m_hStaticFont;
};