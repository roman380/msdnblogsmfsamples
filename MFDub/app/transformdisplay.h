#pragma once

#define WM_CHILDMOVEFINISHED (WM_USER + 1)

class CTransformApplier;
class CTransformDisplay;

class CTransformDisplayEventListener
{
public:
    virtual void HandleTransformRemoved() = 0;
    virtual void HandleTransformMoved() = 0;
};

class CTransformWindow
    : public CWindowImpl<CTransformWindow>
{
public:
    CTransformWindow(CTransformDisplay* pParent, CAtlString strTransformName, HFONT hLabelFont);
    ~CTransformWindow();
    
    CAtlString GetTransformName();
    bool IsMoving() { return m_fIsMoving; }
    bool IsInsideRect(RECT rectBounds);
    
protected:
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseLeave(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
    BEGIN_MSG_MAP(CTransformWindow)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
        MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
        MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(WM_MOUSELEAVE, OnMouseLeave)
    END_MSG_MAP()

    void OnFinalMessage(HWND hWnd);
    
private:
    CTransformDisplay* m_pParent;
    CAtlString m_strTransformName;
    HFONT m_hLabelFont;
    bool m_fIsMoving;
    DWORD m_dxPos;
    DWORD m_dyPos;

    bool m_fHighlight;
};

class CTransformDisplay
    : public CWindowImpl<CTransformDisplay>
{
public:
    CTransformDisplay(CTransformApplier* pTransformApplier);
    ~CTransformDisplay();
    
    void SetEventListener(CTransformDisplayEventListener* pListener);
    void Reset();
    
protected:
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnChildMoveFinished(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
    BEGIN_MSG_MAP(CTransformDisplay)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
        MESSAGE_HANDLER(WM_CHILDMOVEFINISHED, OnChildMoveFinished);
        MESSAGE_HANDLER(WM_MOVE, OnMove);
    END_MSG_MAP()

    void RepositionWindows();
    
private:
    CTransformApplier* m_pTransformApplier;
    HFONT m_hLabelFont;
    CAtlArray<CTransformWindow*> m_arrTransformWindows;
    CTransformDisplayEventListener* m_pEventListener;

    static const LONG m_kMargin = 5;
    static const LONG m_kdxTransformWindow = 180;
    static const LONG m_kdyTransformWindow = 15;
};
