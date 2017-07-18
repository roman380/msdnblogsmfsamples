#pragma once

typedef void (*HANDLESCROLLPROC)(WORD wPos, bool fEnd);

class CTimeBarControl : public CWindowImpl<CTimeBarControl>
{
public:
    CTimeBarControl();
    
    HRESULT Init(HWND hParentWnd, RECT& rect, bool fHoriz, bool fAutoTicks);

    LONG GetMaxPos() const;
    LONG GetPos() const;
    
    void SetPos(LONG lPos);
    void SetRange(LONG minValue, LONG maxValue);
    void SetTickFreq(WORD wFreq);
    void SetScrollCallback(HANDLESCROLLPROC scrollCallback);
    void HandleScroll(WORD wMsg, WORD wPos);
    bool IsTracking();
    
    void SetPosByTime(MFTIME hnsTime, MFTIME hnsDuration);
    
    DECLARE_WND_SUPERCLASS(NULL, TRACKBAR_CLASS)
protected:
    LRESULT OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    

    BEGIN_MSG_MAP(CTimeBarControl)
        MESSAGE_HANDLER(WM_HSCROLL, OnHScroll)
        MESSAGE_HANDLER(WM_VSCROLL, OnVScroll)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
        MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
        MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
    END_MSG_MAP()

protected:
    bool m_fTracking;
    HANDLESCROLLPROC m_scrollCallback;
    LRESULT m_LastClickPos;
};
