#ifndef CODENEST_UICONTROLS_H
#define CODENEST_UICONTROLS_H

#include <functional>
#include <vector>

#include <wx/control.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/panel.h>
#include <wx/string.h>
#include <wx/textctrl.h>

class FlatButton : public wxControl {
public:
    enum Kind { Ghost, Primary };

    FlatButton(wxWindow* parent, wxWindowID id, const wxString& label, Kind kind = Ghost, bool playIcon = false);

    bool AcceptsFocus() const override { return false; }   

protected:
    wxSize DoGetBestSize() const override;

private:
    wxString m_label;
    Kind m_kind;
    bool m_play;
    bool m_hover = false;
    bool m_down = false;

    void OnDown(wxMouseEvent& e);
    void OnUp(wxMouseEvent& e);
    void OnPaint(wxPaintEvent& e);
};

class TabBar : public wxPanel {
public:
    struct Tab { wxString label; bool modified = false; };

    std::function<void(int)> onSelect;
    std::function<void(int)> onClose;
    std::function<void()> onNew;

    explicit TabBar(wxWindow* parent);

    bool AcceptsFocus() const override { return false; }

    void SetTabs(const std::vector<Tab>& tabs, int selected);

private:
    std::vector<Tab> m_tabs;
    int m_sel = 0;
    int m_hoverTab = -1;
    bool m_hoverClose = false;
    bool m_hoverPlus = false;

    struct Geometry { std::vector<wxRect> tabs; wxRect plus; };

    Geometry Compute() const;
    wxRect CloseRect(const wxRect& tab) const;
    int HitTest(const wxPoint& p, bool& onClose, bool& onPlus) const;

    void OnLeftDown(wxMouseEvent& e);
    void OnMiddleDown(wxMouseEvent& e);
    void OnMotion(wxMouseEvent& e);
    void OnLeave(wxMouseEvent& e);
    void OnPaint(wxPaintEvent& e);
};

class Gutter : public wxPanel {
public:
    Gutter(wxWindow* parent, wxTextCtrl* editor);

    int FirstVisibleLine() const;
    void SetCurrentLine(int l) { m_cur = l; }
    int PreferredWidth() const;

private:
    wxTextCtrl* m_ed;
    int m_cur = 0;

    void OnPaint(wxPaintEvent& e);
};

class AccentStrip : public wxPanel {
public:
    explicit AccentStrip(wxWindow* parent);

private:
    void OnPaint(wxPaintEvent& e);
};

class ActivityBar : public wxPanel {
public:
    std::function<void()> onToggle;

    explicit ActivityBar(wxWindow* parent);

    bool AcceptsFocus() const override { return false; }

    void SetActive(bool active) { m_active = active; Refresh(); }

private:
    bool m_active = true;
    bool m_hover = false;

    wxRect ButtonRect() const;

    void OnPaint(wxPaintEvent& e);
    void OnLeftDown(wxMouseEvent& e);
    void OnMotion(wxMouseEvent& e);
    void OnLeave(wxMouseEvent& e);
};

#endif 
