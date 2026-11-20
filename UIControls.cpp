// ============================================================================
//  UIControls.cpp  -  FlatButton, TabBar, Gutter, AccentStrip, ActivityBar
//
//  OWNER: Vanshika
// ============================================================================
#include "UIControls.h"
#include "Theme.h"

#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <wx/dcgraph.h>

#include <algorithm>
#include <string>

// ============================================================================
//  FlatButton
// ============================================================================
FlatButton::FlatButton(wxWindow* parent, wxWindowID id, const wxString& label, Kind kind, bool playIcon)
    : wxControl(parent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
    m_label(label), m_kind(kind), m_play(playIcon) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetFont(wxFont(wxFontInfo(10).FaceName("Segoe UI")));
    SetCursor(wxCursor(wxCURSOR_HAND));
    Bind(wxEVT_PAINT, &FlatButton::OnPaint, this);
    Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent&) { m_hover = true; Refresh(); });
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { m_hover = false; Refresh(); });
    Bind(wxEVT_LEFT_DOWN, &FlatButton::OnDown, this);
    Bind(wxEVT_LEFT_UP, &FlatButton::OnUp, this);
    Bind(wxEVT_LEFT_DCLICK, &FlatButton::OnDown, this);
    Bind(wxEVT_MOUSE_CAPTURE_LOST, [this](wxMouseCaptureLostEvent&) { m_down = false; Refresh(); });
    SetInitialSize();
}

wxSize FlatButton::DoGetBestSize() const {
    int w = 0, h = 0;
    GetTextExtent(m_label, &w, &h);
    int iconW = m_play ? FromDIP(18) : 0;
    return wxSize(w + iconW + FromDIP(28), FromDIP(32));
}

void FlatButton::OnDown(wxMouseEvent&) {
    m_down = true;
    if (!HasCapture()) CaptureMouse();
    Refresh();
}

void FlatButton::OnUp(wxMouseEvent& e) {
    bool fire = m_down && GetClientRect().Contains(e.GetPosition());
    m_down = false;
    if (HasCapture()) ReleaseMouse();
    Refresh();
    if (fire) {
        wxCommandEvent ev(wxEVT_BUTTON, GetId());
        ev.SetEventObject(this);
        ProcessWindowEvent(ev);
    }
}

void FlatButton::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC pdc(this);
    wxColour parentBg = GetParent()->GetBackgroundColour();
    pdc.SetBackground(wxBrush(parentBg));
    pdc.Clear();

    wxGCDC dc(pdc);
    wxRect r = GetClientRect();

    wxColour fill, fg;
    if (m_kind == Primary) {
        fill = m_down ? Theme::RunDown() : (m_hover ? Theme::RunHover() : Theme::Run());
        fg = *wxWHITE;
    }
    else {
        fill = m_down ? Theme::BtnDown() : (m_hover ? Theme::BtnHover() : parentBg);
        fg = Theme::Text();
    }

    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(fill));
    dc.DrawRoundedRectangle(r.x, r.y, r.width, r.height, FromDIP(6));

    dc.SetFont(GetFont());
    dc.SetTextForeground(fg);
    int tw = 0, th = 0;
    dc.GetTextExtent(m_label, &tw, &th);
    int iconW = m_play ? FromDIP(18) : 0;
    int x = (r.width - (tw + iconW)) / 2;
    int y = (r.height - th) / 2;

    if (m_play) {
        int s = FromDIP(10);
        int iy = (r.height - s) / 2;
        wxPoint tri[3] = { wxPoint(x, iy), wxPoint(x, iy + s), wxPoint(x + s - FromDIP(1), iy + s / 2) };
        dc.SetBrush(wxBrush(fg));
        dc.DrawPolygon(3, tri);
        x += iconW;
    }
    dc.DrawText(m_label, x, y);
}

// ============================================================================
//  TabBar
// ============================================================================
TabBar::TabBar(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetFont(wxFont(wxFontInfo(10).FaceName("Segoe UI")));
    SetMinSize(wxSize(-1, FromDIP(36)));
    Bind(wxEVT_PAINT, &TabBar::OnPaint, this);
    Bind(wxEVT_LEFT_DOWN, &TabBar::OnLeftDown, this);
    Bind(wxEVT_MIDDLE_DOWN, &TabBar::OnMiddleDown, this);
    Bind(wxEVT_MOTION, &TabBar::OnMotion, this);
    Bind(wxEVT_LEAVE_WINDOW, &TabBar::OnLeave, this);
    Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { Refresh(); e.Skip(); });
}

void TabBar::SetTabs(const std::vector<Tab>& tabs, int selected) {
    m_tabs = tabs;
    m_sel = selected;
    Refresh();
}

TabBar::Geometry TabBar::Compute() const {
    Geometry g;
    const wxSize cs = GetClientSize();
    const int left = FromDIP(8), gap = FromDIP(1);
    const int closeW = FromDIP(28), padX = FromDIP(14);
    const int minW = FromDIP(70), maxW = FromDIP(230), plusSize = FromDIP(26);

    std::vector<int> w;
    int total = 0;
    for (const Tab& t : m_tabs) {
        int tw = 0, th = 0;
        GetTextExtent(t.label, &tw, &th);
        int ww = std::min(std::max(tw + closeW + padX, FromDIP(110)), maxW);
        w.push_back(ww);
        total += ww + gap;
    }
    const int avail = cs.x - left - plusSize - FromDIP(16);
    if (!w.empty() && total > avail) {
        int each = std::max(minW, avail / (int)w.size() - gap);
        for (int& x : w) x = std::min(x, each);
    }
    int x = left;
    const int top = FromDIP(4);
    for (int ww : w) {
        g.tabs.push_back(wxRect(x, top, ww, cs.y - top));
        x += ww + gap;
    }
    g.plus = wxRect(x + FromDIP(6), (cs.y - plusSize) / 2 + FromDIP(2), plusSize, plusSize);
    return g;
}

wxRect TabBar::CloseRect(const wxRect& tab) const {
    const int s = FromDIP(18);
    return wxRect(tab.x + tab.width - FromDIP(8) - s, tab.y + (tab.height - s) / 2 + FromDIP(1), s, s);
}

// returns tab index (or -1), sets onClose when the pointer is over the x button, plus when over "+"
int TabBar::HitTest(const wxPoint& p, bool& onClose, bool& onPlus) const {
    onClose = false;
    onPlus = false;
    Geometry g = Compute();
    if (g.plus.Contains(p)) { onPlus = true; return -1; }
    for (size_t i = 0; i < g.tabs.size(); ++i) {
        if (g.tabs[i].Contains(p)) {
            onClose = CloseRect(g.tabs[i]).Inflate(FromDIP(2)).Contains(p);
            return (int)i;
        }
    }
    return -1;
}

void TabBar::OnLeftDown(wxMouseEvent& e) {
    bool c = false, plus = false;
    int i = HitTest(e.GetPosition(), c, plus);
    if (plus) { if (onNew) onNew(); return; }
    if (i < 0) return;
    if (c) { if (onClose) onClose(i); }
    else if (onSelect) onSelect(i);
}

void TabBar::OnMiddleDown(wxMouseEvent& e) {
    bool c = false, plus = false;
    int i = HitTest(e.GetPosition(), c, plus);
    if (i >= 0 && onClose) onClose(i);
}

void TabBar::OnMotion(wxMouseEvent& e) {
    bool c = false, plus = false;
    int i = HitTest(e.GetPosition(), c, plus);
    if (i != m_hoverTab || c != m_hoverClose || plus != m_hoverPlus) {
        m_hoverTab = i; m_hoverClose = c; m_hoverPlus = plus;
        Refresh();
    }
}

void TabBar::OnLeave(wxMouseEvent&) {
    m_hoverTab = -1; m_hoverClose = false; m_hoverPlus = false;
    Refresh();
}

void TabBar::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC pdc(this);
    pdc.SetBackground(wxBrush(Theme::Chrome()));
    pdc.Clear();
    wxGCDC dc(pdc);
    dc.SetFont(GetFont());

    Geometry g = Compute();
    const int padX = FromDIP(14);

    for (size_t i = 0; i < g.tabs.size(); ++i) {
        const wxRect& r = g.tabs[i];
        const Tab& t = m_tabs[i];
        const bool sel = ((int)i == m_sel);
        const bool hov = ((int)i == m_hoverTab);

        wxColour fill = sel ? Theme::Bg() : (hov ? Theme::TabHover() : Theme::TabIdle());
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(wxBrush(fill));
        dc.DrawRectangle(r);
        if (sel) {
            dc.SetBrush(wxBrush(Theme::Accent()));
            dc.DrawRectangle(r.x, r.y, r.width, FromDIP(2));
        }

        wxRect cr = CloseRect(r);

        // label (clipped so it never runs into the close button)
        dc.SetClippingRegion(wxRect(r.x, r.y, r.width - cr.width - FromDIP(10), r.height));
        dc.SetTextForeground(sel ? Theme::Text() : Theme::TextDim());
        int tw = 0, th = 0;
        dc.GetTextExtent(t.label, &tw, &th);
        dc.DrawText(t.label, r.x + padX, r.y + (r.height - th) / 2 + FromDIP(1));
        dc.DestroyClippingRegion();

        // dirty dot / close button
        if (t.modified && !hov) {
            dc.SetBrush(wxBrush(sel ? Theme::Text() : Theme::TextDim()));
            dc.DrawCircle(cr.x + cr.width / 2, cr.y + cr.height / 2, FromDIP(4));
        }
        else if (sel || hov) {
            if (hov && m_hoverClose) {
                dc.SetBrush(wxBrush(Theme::BtnDown()));
                dc.DrawRoundedRectangle(cr, FromDIP(4));
            }
            dc.SetPen(wxPen(sel || (hov && m_hoverClose) ? Theme::Text() : Theme::TextDim(), std::max(1, FromDIP(1))));
            const int in = FromDIP(5);
            dc.DrawLine(cr.x + in, cr.y + in, cr.x + cr.width - in, cr.y + cr.height - in);
            dc.DrawLine(cr.x + cr.width - in, cr.y + in, cr.x + in, cr.y + cr.height - in);
        }
    }

    // "+" button
    {
        const wxRect& r = g.plus;
        if (m_hoverPlus) {
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.SetBrush(wxBrush(Theme::BtnHover()));
            dc.DrawRoundedRectangle(r, FromDIP(5));
        }
        dc.SetPen(wxPen(m_hoverPlus ? Theme::Text() : Theme::TextDim(), std::max(1, FromDIP(1))));
        const int cx = r.x + r.width / 2, cy = r.y + r.height / 2, a = FromDIP(5);
        dc.DrawLine(cx - a, cy, cx + a + 1, cy);
        dc.DrawLine(cx, cy - a, cx, cy + a + 1);
    }

    // bottom border
    dc.SetPen(wxPen(Theme::Border()));
    const wxSize cs = GetClientSize();
    dc.DrawLine(0, cs.y - 1, cs.x, cs.y - 1);
}

// ============================================================================
//  Gutter
// ============================================================================
Gutter::Gutter(wxWindow* parent, wxTextCtrl* editor)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE), m_ed(editor) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, &Gutter::OnPaint, this);
}

int Gutter::FirstVisibleLine() const {
#ifdef __WXMSW__
    return (int)::SendMessage((HWND)m_ed->GetHWND(), EM_GETFIRSTVISIBLELINE, 0, 0);
#else
    return 0;
#endif
}

int Gutter::PreferredWidth() const {
    int digits = (int)std::to_string(std::max(1, m_ed->GetNumberOfLines())).size();
    digits = std::max(digits, 3);
    int cw = 0, ch = 0;
    GetTextExtent("0", &cw, &ch);
    return digits * cw + FromDIP(26);
}

void Gutter::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(Theme::GutterBg()));
    dc.Clear();
    dc.SetFont(GetFont());

    wxSize cs = GetClientSize();
    const int total = m_ed->GetNumberOfLines();
    const int first = FirstVisibleLine();

    for (int l = first; l < total; ++l) {
        long pos = m_ed->XYToPosition(0, l);
        if (pos < 0) break;
        wxPoint pt = m_ed->PositionToCoords(pos);
        if (pt == wxDefaultPosition) continue;
        if (pt.y >= cs.y) break;

        wxString s = wxString::Format("%d", l + 1);
        dc.SetTextForeground(l == m_cur ? Theme::Text() : Theme::TextDim());
        int tw = 0, th = 0;
        dc.GetTextExtent(s, &tw, &th);
        dc.DrawText(s, cs.x - tw - FromDIP(12), pt.y);
    }
    dc.SetPen(wxPen(Theme::GutterLine()));
    dc.DrawLine(cs.x - 1, 0, cs.x - 1, cs.y);
}

// ============================================================================
//  AccentStrip
// ============================================================================
AccentStrip::AccentStrip(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(-1, FromDIP(4)));
    Bind(wxEVT_PAINT, &AccentStrip::OnPaint, this);
}

void AccentStrip::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    const wxSize cs = GetClientSize();
    if (Theme::IsRainbow()) {
        static const int cols[7][3] = { {255, 120, 150}, {255, 170, 90}, {255, 225, 100}, {130, 215, 130},
                                        {110, 190, 255}, {160, 140, 255}, {255, 140, 200} };
        const int n = 6;
        for (int i = 0; i < n; ++i) {
            int x0 = cs.x * i / n, x1 = cs.x * (i + 1) / n;
            wxRect r(x0, 0, std::max(1, x1 - x0), cs.y);
            dc.GradientFillLinear(r,
                wxColour((unsigned char)cols[i][0], (unsigned char)cols[i][1], (unsigned char)cols[i][2]),
                wxColour((unsigned char)cols[i + 1][0], (unsigned char)cols[i + 1][1], (unsigned char)cols[i + 1][2]),
                wxEAST);
        }
    }
    else {
        dc.SetBackground(wxBrush(Theme::Chrome()));
        dc.Clear();
        dc.SetPen(wxPen(Theme::Border()));
        dc.DrawLine(0, cs.y - 1, cs.x, cs.y - 1);
    }
}

// ============================================================================
//  ActivityBar  (Explorer icon: click = show / hide the explorer panel)
// ============================================================================
ActivityBar::ActivityBar(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(FromDIP(48), -1));
    SetCursor(wxCursor(wxCURSOR_HAND));
    SetToolTip("Explorer (Ctrl+B)");
    Bind(wxEVT_PAINT, &ActivityBar::OnPaint, this);
    Bind(wxEVT_LEFT_DOWN, &ActivityBar::OnLeftDown, this);
    Bind(wxEVT_MOTION, &ActivityBar::OnMotion, this);
    Bind(wxEVT_LEAVE_WINDOW, &ActivityBar::OnLeave, this);
}

wxRect ActivityBar::ButtonRect() const {
    return wxRect(0, 0, GetClientSize().x, FromDIP(48));
}

void ActivityBar::OnLeftDown(wxMouseEvent& e) {
    if (ButtonRect().Contains(e.GetPosition()) && onToggle) onToggle();
}

void ActivityBar::OnMotion(wxMouseEvent& e) {
    bool h = ButtonRect().Contains(e.GetPosition());
    if (h != m_hover) { m_hover = h; Refresh(); }
}

void ActivityBar::OnLeave(wxMouseEvent&) {
    if (m_hover) { m_hover = false; Refresh(); }
}

void ActivityBar::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC pdc(this);
    pdc.SetBackground(wxBrush(Theme::Chrome()));
    pdc.Clear();
    wxGCDC dc(pdc);

    const wxSize cs = GetClientSize();
    const wxRect br = ButtonRect();

    // active marker on the left edge
    if (m_active) {
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(wxBrush(Theme::Accent()));
        dc.DrawRectangle(0, br.y, FromDIP(2), br.height);
    }

    // "Explorer" icon: two overlapping pages
    const wxColour col = (m_active || m_hover) ? Theme::Text() : Theme::TextDim();
    dc.SetPen(wxPen(col, std::max(1, FromDIP(1))));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);

    const int size = FromDIP(24);
    const int ix = br.x + (br.width - size) / 2;
    const int iy = br.y + (br.height - size) / 2;
    auto d = [&](int v) { return FromDIP(v); };

    // page in the back (top right), with a folded corner
    wxPoint back[6] = {
        wxPoint(ix + d(8),  iy + d(6)),  wxPoint(ix + d(8),  iy + d(0)),
        wxPoint(ix + d(17), iy + d(0)),  wxPoint(ix + d(22), iy + d(5)),
        wxPoint(ix + d(22), iy + d(17)), wxPoint(ix + d(14), iy + d(17))
    };
    dc.DrawLines(6, back);
    wxPoint fold[3] = {
        wxPoint(ix + d(17), iy + d(0)), wxPoint(ix + d(17), iy + d(5)), wxPoint(ix + d(22), iy + d(5))
    };
    dc.DrawLines(3, fold);

    // page in the front (bottom left)
    dc.DrawRectangle(ix + d(2), iy + d(6), d(12), d(16));

    // right border
    dc.SetPen(wxPen(Theme::Border()));
    dc.DrawLine(cs.x - 1, 0, cs.x - 1, cs.y);
}
