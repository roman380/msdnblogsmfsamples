#pragma once

class CMainToolbar : public CWindowImpl<CMainToolbar>
{
public:
    CMainToolbar();
    ~CMainToolbar();

    void EnableButtonByCommand(int iID, BOOL fEnable);
    
    DECLARE_WND_SUPERCLASS(NULL, TOOLBARCLASSNAME)
protected:
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    BEGIN_MSG_MAP(CMainToolbar)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
    END_MSG_MAP()

private:
};