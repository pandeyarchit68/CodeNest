// ============================================================================
//  main.cpp  -  CodeNest: main window (MyFrame), application (MyApp), layout
//
//  OWNER: Archit (Team Lead)
//
//  Responsible for: GUI structure and layout, integration of all modules,
//  the document system (tabs), file handling coordination, editor behaviour
//  (auto-indent, auto-closing brackets, smart Enter), menus and shortcuts.
//
//  Modules used:
//    Theme.h              (Anushka)   colours + theme switching
//    SyntaxHighlight.*    (Anushka)   C syntax colouring
//    UIControls.*         (Vanshika)  FlatButton, TabBar, Gutter, ActivityBar
//    FileExplorer.*       (Vanshika)  folder tree panel (show / hide with the icon)
//    RunSystem.*          (Yuvraj)    external Command Prompt compile & run
//    Editor.*, FileManager.*, Doc.h   bracket checker, file I/O, document struct
// ============================================================================

#include <wx/wx.h>
#include <wx/accel.h>
#include <wx/config.h>
#include <wx/dirdlg.h>
#include <wx/filename.h>

#include <algorithm>
#include <cwctype>
#include <memory>
#include <regex>
#include <string>
#include <vector>

#include "Doc.h"
#include "Editor.h"
#include "FileExplorer.h"
#include "FileManager.h"
#include "RunSystem.h"
#include "SyntaxHighlight.h"
#include "Theme.h"
#include "UIControls.h"

// ----------------------------------------------------------------------------
//  Main window
// ----------------------------------------------------------------------------
enum {
    ID_RUN = wxID_HIGHEST + 1,
    ID_ZOOM_IN,
    ID_ZOOM_OUT,
    ID_ZOOM_RESET,
    ID_NEXT_TAB,
    ID_PREV_TAB,
    ID_POLL_TIMER,
    ID_HIGHLIGHT_TIMER,
    ID_THEME_BTN,
    ID_TOGGLE_EXPLORER,
    ID_OPEN_FOLDER,
    ID_THEME_FIRST = wxID_HIGHEST + 100      // one id per theme: ID_THEME_FIRST + index
};

class MyFrame : public wxFrame {
public:
    MyFrame()
        : wxFrame(nullptr, wxID_ANY, "CodeNest", wxDefaultPosition, wxSize(1120, 740)),
        m_pollTimer(this, ID_POLL_TIMER),
        m_hlTimer(this, ID_HIGHLIGHT_TIMER) {

        // the theme and explorer state chosen last time (stored in the registry / config file)
        {
            long saved = 0;
            wxConfigBase::Get()->Read("theme", &saved, 0L);
            Theme::Set((int)saved);

            long vis = 1;
            wxConfigBase::Get()->Read("explorerVisible", &vis, 1L);
            m_explorerVisible = (vis != 0);
        }

        SetBackgroundColour(Theme::Bg());
        wxFont uiFont(wxFontInfo(10).FaceName("Segoe UI"));

        BuildMenu();

        m_root = new wxPanel(this);
        m_root->SetBackgroundColour(Theme::Bg());
        wxBoxSizer* rootSizer = new wxBoxSizer(wxVERTICAL);

        // ---------- header bar ----------
        m_header = new wxPanel(m_root);
        wxPanel* header = m_header;
        header->SetBackgroundColour(Theme::Chrome());
        wxBoxSizer* hs = new wxBoxSizer(wxHORIZONTAL);

        wxStaticText* logo = new wxStaticText(header, wxID_ANY, "CodeNest");
        m_logo = logo;
        logo->SetFont(wxFont(wxFontInfo(14).FaceName("Segoe UI").Bold()));
        logo->SetForegroundColour(Theme::Logo());
        logo->SetBackgroundColour(Theme::Chrome());

        wxStaticText* sub = new wxStaticText(header, wxID_ANY, "C IDE");
        m_sub = sub;
        sub->SetFont(uiFont);
        sub->SetForegroundColour(Theme::TextDim());
        sub->SetBackgroundColour(Theme::Chrome());

        FlatButton* themeBtn = new FlatButton(header, ID_THEME_BTN, "Theme");
        themeBtn->SetToolTip("Choose a colour theme");
        FlatButton* newBtn = new FlatButton(header, wxID_NEW, "New");
        FlatButton* openBtn = new FlatButton(header, wxID_OPEN, "Open");
        FlatButton* saveBtn = new FlatButton(header, wxID_SAVE, "Save");
        FlatButton* runBtn = new FlatButton(header, ID_RUN, "Run", FlatButton::Primary, true);
        newBtn->SetToolTip("New tab (Ctrl+N)");
        openBtn->SetToolTip("Open file(s) (Ctrl+O)");
        saveBtn->SetToolTip("Save file (Ctrl+S)");
        runBtn->SetToolTip("Compile and run in a Command Prompt window (F5)");

        hs->AddSpacer(FromDIP(16));
        hs->Add(logo, 0, wxALIGN_CENTER_VERTICAL);
        hs->AddSpacer(FromDIP(8));
        hs->Add(sub, 0, wxALIGN_CENTER_VERTICAL | wxTOP, FromDIP(4));
        hs->AddStretchSpacer();
        hs->Add(themeBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(18));
        hs->Add(newBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(4));
        hs->Add(openBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(4));
        hs->Add(saveBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(18));
        hs->Add(runBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(16));

        wxBoxSizer* hv = new wxBoxSizer(wxVERTICAL);
        hv->Add(hs, 1, wxEXPAND | wxTOP | wxBOTTOM, FromDIP(9));
        header->SetSizer(hv);

        rootSizer->Add(header, 0, wxEXPAND);
        rootSizer->Add(new AccentStrip(m_root), 0, wxEXPAND);

        // ---------- body: [ activity bar | explorer | tabs + editors ] ----------
        m_body = new wxPanel(m_root);
        m_body->SetBackgroundColour(Theme::Bg());
        m_bodySizer = new wxBoxSizer(wxHORIZONTAL);

        m_activity = new ActivityBar(m_body);
        m_activity->SetActive(m_explorerVisible);
        m_activity->onToggle = [this]() { SetExplorerVisible(!m_explorerVisible); };

        m_explorer = new FileExplorer(m_body);
        m_explorer->onOpenFolderRequest = [this]() { ChooseFolder(); };
        m_explorer->onOpenFile = [this](const wxString& p) { OpenFromExplorer(p); };

        m_editorArea = new wxPanel(m_body);
        m_editorArea->SetBackgroundColour(Theme::Bg());
        wxBoxSizer* areaSizer = new wxBoxSizer(wxVERTICAL);

        // ---------- tab bar ----------
        m_tabBar = new TabBar(m_editorArea);
        m_tabBar->onSelect = [this](int i) { SwitchTo(i); };
        m_tabBar->onClose = [this](int i) { CallAfter([this, i]() { if (i >= 0 && i < (int)m_docs.size()) CloseDoc(i); }); };
        m_tabBar->onNew = [this]() { NewDoc(SampleCode(), wxString()); };
        areaSizer->Add(m_tabBar, 0, wxEXPAND);

        // ---------- pages (one per document) ----------
        m_pages = new wxPanel(m_editorArea);
        m_pages->SetBackgroundColour(Theme::Bg());
        m_pagesSizer = new wxBoxSizer(wxVERTICAL);
        m_pages->SetSizer(m_pagesSizer);
        areaSizer->Add(m_pages, 1, wxEXPAND);
        m_editorArea->SetSizer(areaSizer);

        m_bodySizer->Add(m_activity, 0, wxEXPAND);
        m_bodySizer->Add(m_explorer, 0, wxEXPAND);
        m_bodySizer->Add(m_editorArea, 1, wxEXPAND);
        m_bodySizer->Show(m_explorer, m_explorerVisible);
        m_body->SetSizer(m_bodySizer);
        rootSizer->Add(m_body, 1, wxEXPAND);

        // ---------- status bar ----------
        m_status = new wxPanel(m_root);
        m_status->SetBackgroundColour(Theme::Accent());
        wxBoxSizer* ss = new wxBoxSizer(wxHORIZONTAL);
        m_statusLeft = new wxStaticText(m_status, wxID_ANY, "Ready");
        m_statusRight = new wxStaticText(m_status, wxID_ANY, "Ln 1, Col 1");
        for (wxStaticText* t : { m_statusLeft, m_statusRight }) {
            t->SetFont(wxFont(wxFontInfo(9).FaceName("Segoe UI")));
            t->SetForegroundColour(Theme::OnAccent());
            t->SetBackgroundColour(Theme::Accent());
        }
        ss->AddSpacer(FromDIP(12));
        ss->Add(m_statusLeft, 0, wxALIGN_CENTER_VERTICAL);
        ss->AddStretchSpacer();
        ss->Add(m_statusRight, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(16));
        wxBoxSizer* sv = new wxBoxSizer(wxVERTICAL);
        sv->Add(ss, 1, wxEXPAND | wxTOP | wxBOTTOM, FromDIP(3));
        m_status->SetSizer(sv);
        rootSizer->Add(m_status, 0, wxEXPAND);

        m_root->SetSizer(rootSizer);
        wxBoxSizer* frameSizer = new wxBoxSizer(wxVERTICAL);
        frameSizer->Add(m_root, 1, wxEXPAND);
        SetSizer(frameSizer);

        SetMinSize(wxSize(FromDIP(820), FromDIP(520)));
        SetClientSize(FromDIP(wxSize(1100, 700)));
        Centre();

        m_explorer->ApplyTheme();

        // reopen the folder of the last session (if it still exists)
        {
            wxString last;
            wxConfigBase::Get()->Read("lastFolder", &last);
            if (!last.empty() && wxDirExists(last)) m_explorer->OpenFolder(last);
        }

        // ---------- first document ----------
        NewDoc(SampleCode(), wxString());

#ifdef __WXMSW__
        ApplyTitleBarTheme((HWND)GetHWND(), Theme::IsDark(), Theme::Chrome(), Theme::Text());
#endif

        // ---------- events ----------
        Bind(wxEVT_BUTTON, &MyFrame::OnNew, this, wxID_NEW);
        Bind(wxEVT_BUTTON, &MyFrame::OnOpen, this, wxID_OPEN);
        Bind(wxEVT_BUTTON, &MyFrame::OnSave, this, wxID_SAVE);
        Bind(wxEVT_BUTTON, &MyFrame::OnRun, this, ID_RUN);
        Bind(wxEVT_BUTTON, &MyFrame::OnThemeButton, this, ID_THEME_BTN);
        Bind(wxEVT_MENU, &MyFrame::OnThemeCmd, this, ID_THEME_FIRST, ID_THEME_FIRST + 31);

        Bind(wxEVT_MENU, &MyFrame::OnNew, this, wxID_NEW);
        Bind(wxEVT_MENU, &MyFrame::OnOpen, this, wxID_OPEN);
        Bind(wxEVT_MENU, &MyFrame::OnOpenFolder, this, ID_OPEN_FOLDER);
        Bind(wxEVT_MENU, &MyFrame::OnSave, this, wxID_SAVE);
        Bind(wxEVT_MENU, &MyFrame::OnSaveAs, this, wxID_SAVEAS);
        Bind(wxEVT_MENU, &MyFrame::OnCloseTab, this, wxID_CLOSE);
        Bind(wxEVT_MENU, &MyFrame::OnExit, this, wxID_EXIT);
        Bind(wxEVT_MENU, &MyFrame::OnAbout, this, wxID_ABOUT);
        Bind(wxEVT_MENU, &MyFrame::OnRun, this, ID_RUN);
        Bind(wxEVT_MENU, [this](wxCommandEvent&) { SetExplorerVisible(!m_explorerVisible); }, ID_TOGGLE_EXPLORER);
        Bind(wxEVT_MENU, [this](wxCommandEvent&) { Zoom(+1); }, ID_ZOOM_IN);
        Bind(wxEVT_MENU, [this](wxCommandEvent&) { Zoom(-1); }, ID_ZOOM_OUT);
        Bind(wxEVT_MENU, [this](wxCommandEvent&) { m_fontSize = 11; ApplyFontsAll(); }, ID_ZOOM_RESET);
        Bind(wxEVT_MENU, [this](wxCommandEvent&) { CycleTab(+1); }, ID_NEXT_TAB);
        Bind(wxEVT_MENU, [this](wxCommandEvent&) { CycleTab(-1); }, ID_PREV_TAB);

        Bind(wxEVT_CLOSE_WINDOW, &MyFrame::OnClose, this);
        Bind(wxEVT_TIMER, &MyFrame::OnPoll, this, ID_POLL_TIMER);
        Bind(wxEVT_TIMER, [this](wxTimerEvent&) { if (Doc* d = Cur()) Highlight(*d); }, ID_HIGHLIGHT_TIMER);

        wxAcceleratorEntry entries[1];
        entries[0].Set(wxACCEL_CTRL, (int)'R', ID_RUN);
        SetAcceleratorTable(wxAcceleratorTable(1, entries));

        // old run folders from earlier sessions are no longer needed
        RunSystem::CleanTempRoot();

        m_pollTimer.Start(80);
    }

private:
    // ----- widgets -----
    wxPanel* m_root = nullptr;
    wxPanel* m_header = nullptr;
    wxStaticText* m_logo = nullptr;
    wxStaticText* m_sub = nullptr;
    std::vector<wxPanel*> m_lines;       // thin separator lines (recoloured when the theme changes)
    wxPanel* m_body = nullptr;
    wxPanel* m_editorArea = nullptr;
    wxPanel* m_pages = nullptr;
    wxPanel* m_status = nullptr;
    wxBoxSizer* m_bodySizer = nullptr;
    wxBoxSizer* m_pagesSizer = nullptr;
    ActivityBar* m_activity = nullptr;
    FileExplorer* m_explorer = nullptr;
    TabBar* m_tabBar = nullptr;
    wxStaticText* m_statusLeft = nullptr;
    wxStaticText* m_statusRight = nullptr;

    // ----- state -----
    wxTimer m_pollTimer;
    wxTimer m_hlTimer;
    std::vector<std::unique_ptr<Doc>> m_docs;
    int m_cur = -1;
    int m_untitledCounter = 0;
    int m_runCounter = 0;
    int m_fontSize = 11;
    int m_lastFirst = -1, m_lastLines = -1;
    long m_lastPos = -1;
    bool m_explorerVisible = true;  // file explorer panel shown / hidden
    bool m_stateBusy = false;       // status bar shows the orange 'busy' colour
    bool m_synthEnter = false;      // true while we send a real Enter key to the editor ourselves

    Editor editor;
    FileManager fm;

    static wxString SampleCode() {
        return "#include <stdio.h>\n\nint main() {\n    printf(\"Hello from CodeNest!\\n\");\n    return 0;\n}\n";
    }

    wxPanel* MakeLine(wxWindow* parent) {
        wxPanel* line = new wxPanel(parent);
        line->SetBackgroundColour(Theme::Border());
        line->SetMinSize(wxSize(-1, 1));
        m_lines.push_back(line);
        return line;
    }

    Doc* Cur() const {
        return (m_cur >= 0 && m_cur < (int)m_docs.size()) ? m_docs[m_cur].get() : nullptr;
    }

    void BuildMenu() {
        wxMenu* fileMenu = new wxMenu();
        fileMenu->Append(wxID_NEW, "&New Tab\tCtrl+N");
        fileMenu->Append(wxID_OPEN, "&Open...\tCtrl+O");
        fileMenu->Append(ID_OPEN_FOLDER, "Open &Folder...\tCtrl+Shift+O");
        fileMenu->Append(wxID_SAVE, "&Save\tCtrl+S");
        fileMenu->Append(wxID_SAVEAS, "Save &As...\tCtrl+Shift+S");
        fileMenu->Append(wxID_CLOSE, "&Close Tab\tCtrl+W");
        fileMenu->AppendSeparator();
        fileMenu->Append(wxID_EXIT, "E&xit\tCtrl+Q");

        wxMenu* editMenu = new wxMenu();
        editMenu->Append(wxID_UNDO, "&Undo\tCtrl+Z");
        editMenu->Append(wxID_REDO, "&Redo\tCtrl+Y");
        editMenu->AppendSeparator();
        editMenu->Append(wxID_CUT, "Cu&t\tCtrl+X");
        editMenu->Append(wxID_COPY, "&Copy\tCtrl+C");
        editMenu->Append(wxID_PASTE, "&Paste\tCtrl+V");
        editMenu->AppendSeparator();
        editMenu->Append(wxID_SELECTALL, "Select &All\tCtrl+A");

        wxMenu* runMenu = new wxMenu();
        runMenu->Append(ID_RUN, "&Compile && Run\tF5");

        wxMenu* viewMenu = new wxMenu();
        wxMenuItem* expItem = viewMenu->AppendCheckItem(ID_TOGGLE_EXPLORER, "&Explorer\tCtrl+B");
        expItem->Check(m_explorerVisible);
        viewMenu->AppendSeparator();
        viewMenu->Append(ID_ZOOM_IN, "Zoom &In\tCtrl++");
        viewMenu->Append(ID_ZOOM_OUT, "Zoom &Out\tCtrl+-");
        viewMenu->Append(ID_ZOOM_RESET, "&Reset Zoom\tCtrl+0");
        viewMenu->AppendSeparator();
        viewMenu->Append(ID_NEXT_TAB, "&Next Tab\tCtrl+Tab");
        viewMenu->Append(ID_PREV_TAB, "&Previous Tab\tCtrl+Shift+Tab");
        viewMenu->AppendSeparator();
        wxMenu* themeMenu = new wxMenu();
        for (int i = 0; i < Theme::Count(); ++i) {
            wxMenuItem* it = themeMenu->AppendRadioItem(ID_THEME_FIRST + i, Theme::NameOf(i));
            if (i == Theme::Index()) it->Check(true);
        }
        viewMenu->AppendSubMenu(themeMenu, "&Theme");

        wxMenu* helpMenu = new wxMenu();
        helpMenu->Append(wxID_ABOUT, "&About CodeNest");

        wxMenuBar* bar = new wxMenuBar();
        bar->Append(fileMenu, "&File");
        bar->Append(editMenu, "&Edit");
        bar->Append(runMenu, "&Run");
        bar->Append(viewMenu, "&View");
        bar->Append(helpMenu, "&Help");
        SetMenuBar(bar);
    }

    // ------------------------------------------------------------------
    //  File explorer panel (show / hide, open folder, open file)
    // ------------------------------------------------------------------
    void SetExplorerVisible(bool visible) {
        m_explorerVisible = visible;
        m_bodySizer->Show(m_explorer, visible);
        m_body->Layout();
        m_activity->SetActive(visible);
        if (wxMenuBar* mb = GetMenuBar()) mb->Check(ID_TOGGLE_EXPLORER, visible);

        wxConfigBase* cfg = wxConfigBase::Get();
        cfg->Write("explorerVisible", visible ? 1L : 0L);
        cfg->Flush();

        if (Doc* d = Cur()) d->editor->SetFocus();
    }

    void ChooseFolder() {
        wxDirDialog dlg(this, "Open Folder", m_explorer->GetRootPath(),
            wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
        if (dlg.ShowModal() == wxID_CANCEL) return;

        const wxString dir = dlg.GetPath();
        m_explorer->OpenFolder(dir);
        if (!m_explorerVisible) SetExplorerVisible(true);

        wxConfigBase* cfg = wxConfigBase::Get();
        cfg->Write("lastFolder", dir);
        cfg->Flush();
        SetState("Folder opened");
    }

    void OnOpenFolder(wxCommandEvent&) { ChooseFolder(); }

    void OpenFromExplorer(const wxString& path) {
        if (wxFileName(path).GetSize().GetValue() > 5ULL * 1024 * 1024) {
            wxMessageBox("This file is too large to open in the editor:\n" + path,
                "CodeNest", wxOK | wxICON_INFORMATION, this);
            return;
        }
        OpenPath(path);
    }

    // ------------------------------------------------------------------
    //  Documents / tabs
    // ------------------------------------------------------------------
    Doc* NewDoc(const wxString& text, const wxString& path) {
        std::unique_ptr<Doc> up(new Doc());
        Doc* d = up.get();
        d->path = path;
        d->untitledNo = path.empty() ? ++m_untitledCounter : 0;

        d->page = new wxPanel(m_pages);
        d->page->SetBackgroundColour(Theme::Bg());

        d->editor = new wxTextCtrl(d->page, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
            wxTE_MULTILINE | wxTE_RICH2 | wxHSCROLL | wxTE_DONTWRAP | wxTE_PROCESS_TAB | wxBORDER_NONE);
        d->editor->SetBackgroundColour(Theme::Bg());
        d->editor->SetForegroundColour(Theme::Text());

        d->gutter = new Gutter(d->page, d->editor);

        wxBoxSizer* es = new wxBoxSizer(wxHORIZONTAL);
        es->Add(d->gutter, 0, wxEXPAND);
        es->AddSpacer(FromDIP(6));
        es->Add(d->editor, 1, wxEXPAND);
        d->page->SetSizer(es);

        m_pagesSizer->Add(d->page, 1, wxEXPAND);
        d->page->Hide();

        ApplyFontsTo(*d);
        d->editor->ChangeValue(text);
        Highlight(*d);

#ifdef __WXMSW__
        ApplyControlTheme((HWND)d->editor->GetHWND(), Theme::IsDark());
#endif

        d->editor->Bind(wxEVT_CHAR, [this, d](wxKeyEvent& e) { OnEditorChar(*d, e); });
        d->editor->Bind(wxEVT_TEXT, [this, d](wxCommandEvent& e) { OnEditorText(*d, e); });
        d->editor->Bind(wxEVT_MOUSEWHEEL, [this](wxMouseEvent& e) {
            if (e.ControlDown()) Zoom(e.GetWheelRotation() > 0 ? +1 : -1);
            else e.Skip();
            });

        m_docs.push_back(std::move(up));
        SwitchTo((int)m_docs.size() - 1);
        return d;
    }

    void SwitchTo(int i) {
        if (i < 0 || i >= (int)m_docs.size()) return;
        if (Doc* old = Cur()) old->page->Hide();
        m_cur = i;
        Doc* d = Cur();
        d->page->Show();
        m_pages->Layout();
        d->editor->SetFocus();
        m_lastFirst = -1; m_lastLines = -1; m_lastPos = -1;     // force the status bar to refresh
        UpdateTitle();
    }

    void CycleTab(int dir) {
        const int n = (int)m_docs.size();
        if (n < 2) return;
        SwitchTo(((m_cur + dir) % n + n) % n);
    }

    bool CloseDoc(int i) {
        if (i < 0 || i >= (int)m_docs.size()) return false;
        if (!ConfirmDiscard(*m_docs[i])) return false;

        const bool wasCurrent = (i == m_cur);
        Doc* d = m_docs[i].get();
        d->page->Hide();
        m_pagesSizer->Detach(d->page);
        d->page->Destroy();
        m_docs.erase(m_docs.begin() + i);

        if (m_docs.empty()) {
            m_cur = -1;
            NewDoc(SampleCode(), wxString());
            return true;
        }
        if (wasCurrent) {
            m_cur = -1;
            SwitchTo(std::min(i, (int)m_docs.size() - 1));
        }
        else {
            if (i < m_cur) --m_cur;
            RefreshTabs();
        }
        return true;
    }

    int FindDocByPath(const wxString& path) const {
        for (size_t i = 0; i < m_docs.size(); ++i)
            if (!m_docs[i]->path.empty() && m_docs[i]->path.IsSameAs(path, false)) return (int)i;
        return -1;
    }

    void RefreshTabs() {
        std::vector<TabBar::Tab> tabs;
        for (const auto& d : m_docs) {
            TabBar::Tab t;
            t.label = d->Name();
            t.modified = d->modified;
            tabs.push_back(t);
        }
        m_tabBar->SetTabs(tabs, m_cur);
    }

    void UpdateTitle() {
        Doc* d = Cur();
        if (!d) return;
        SetTitle(d->Name() + (d->modified ? "*" : "") + " - CodeNest");
        RefreshTabs();
    }

    // ------------------------------------------------------------------
    //  Fonts / zoom
    // ------------------------------------------------------------------
    void ApplyFontsTo(Doc& d) {
        wxFont mono(wxFontInfo(m_fontSize).FaceName("Consolas"));
        d.editor->SetFont(mono);
        d.gutter->SetFont(mono);
        d.editor->SetDefaultStyle(wxTextAttr(Theme::Text()));
        UpdateGutterWidth(d);
        d.gutter->Refresh();
    }

    void ApplyFontsAll() {
        for (auto& d : m_docs) {
            ApplyFontsTo(*d);
            Highlight(*d);
        }
    }

    void Zoom(int delta) {
        int s = std::clamp(m_fontSize + delta, 8, 30);
        if (s == m_fontSize) return;
        m_fontSize = s;
        ApplyFontsAll();
    }

    void UpdateGutterWidth(Doc& d) {
        int w = d.gutter->PreferredWidth();
        if (d.gutter->GetMinSize().x != w) {
            d.gutter->SetMinSize(wxSize(w, -1));
            d.page->Layout();
        }
    }

    // ------------------------------------------------------------------
    //  Status bar
    // ------------------------------------------------------------------
    // busy = orange bar (used for warnings); otherwise the accent colour of the theme
    void SetState(const wxString& text, bool busy = false) {
        m_stateBusy = busy;
        m_statusLeft->SetLabel(text);
        ApplyStatusColours();
    }

    void ApplyStatusColours() {
        const wxColour bg = m_stateBusy ? Theme::Busy() : Theme::Accent();
        const wxColour fg = m_stateBusy ? wxColour(255, 255, 255) : Theme::OnAccent();
        m_status->SetBackgroundColour(bg);
        for (wxStaticText* t : { m_statusLeft, m_statusRight }) {
            t->SetBackgroundColour(bg);
            t->SetForegroundColour(fg);
        }
        m_status->Layout();
        m_status->Refresh();
    }

    // ------------------------------------------------------------------
    //  Themes
    // ------------------------------------------------------------------
    void ApplyTheme() {
        SetBackgroundColour(Theme::Bg());
        m_root->SetBackgroundColour(Theme::Bg());
        m_header->SetBackgroundColour(Theme::Chrome());
        m_logo->SetForegroundColour(Theme::Logo());
        m_logo->SetBackgroundColour(Theme::Chrome());
        m_sub->SetForegroundColour(Theme::TextDim());
        m_sub->SetBackgroundColour(Theme::Chrome());
        for (wxPanel* l : m_lines) l->SetBackgroundColour(Theme::Border());
        m_body->SetBackgroundColour(Theme::Bg());
        m_editorArea->SetBackgroundColour(Theme::Bg());
        m_pages->SetBackgroundColour(Theme::Bg());

        for (auto& d : m_docs) {
            d->page->SetBackgroundColour(Theme::Bg());
            d->editor->SetBackgroundColour(Theme::Bg());
            d->editor->SetForegroundColour(Theme::Text());
            d->editor->SetDefaultStyle(wxTextAttr(Theme::Text()));
#ifdef __WXMSW__
            ApplyControlTheme((HWND)d->editor->GetHWND(), Theme::IsDark());
#endif
            Highlight(*d);                       // re-colour the code with the new syntax colours
            d->gutter->Refresh();
        }

        m_explorer->ApplyTheme();
        m_activity->Refresh();

        ApplyStatusColours();
#ifdef __WXMSW__
        ApplyTitleBarTheme((HWND)GetHWND(), Theme::IsDark(), Theme::Chrome(), Theme::Text());
#endif
        m_tabBar->Refresh();
        m_root->Refresh(true);
        Refresh(true);
    }

    void SetTheme(int idx) {
        if (idx < 0 || idx >= Theme::Count() || idx == Theme::Index()) return;
        Theme::Set(idx);
        ApplyTheme();
        if (wxMenuBar* mb = GetMenuBar()) mb->Check(ID_THEME_FIRST + idx, true);
        wxConfigBase* cfg = wxConfigBase::Get();
        cfg->Write("theme", (long)idx);
        cfg->Flush();
        SetState("Theme: " + wxString(Theme::NameOf(idx)));
    }

    void OnThemeCmd(wxCommandEvent& e) { SetTheme(e.GetId() - ID_THEME_FIRST); }

    void OnThemeButton(wxCommandEvent&) {
        wxMenu menu;
        for (int i = 0; i < Theme::Count(); ++i) {
            wxMenuItem* it = menu.AppendRadioItem(ID_THEME_FIRST + i, Theme::NameOf(i));
            it->Check(i == Theme::Index());
        }
        wxWindow* btn = FindWindow(ID_THEME_BTN);
        wxPoint pt = btn ? ScreenToClient(btn->ClientToScreen(wxPoint(0, btn->GetSize().y))) : wxPoint(0, 0);
        PopupMenu(&menu, pt);
    }

    void OnPoll(wxTimerEvent&) {
        Doc* d = Cur();
        if (!d) return;
        int first = d->gutter->FirstVisibleLine();
        int lines = d->editor->GetNumberOfLines();
        long pos = d->editor->GetInsertionPoint();
        if (first == m_lastFirst && lines == m_lastLines && pos == m_lastPos) return;

        bool linesChanged = (lines != m_lastLines);
        m_lastFirst = first; m_lastLines = lines; m_lastPos = pos;

        long col = 0, line = 0;
        d->editor->PositionToXY(pos, &col, &line);
        d->gutter->SetCurrentLine((int)line);
        m_statusRight->SetLabel(wxString::Format("Ln %ld, Col %ld", line + 1, col + 1));
        m_status->Layout();
        if (linesChanged) UpdateGutterWidth(*d);
        d->gutter->Refresh();
    }

    // ------------------------------------------------------------------
    //  Syntax highlighting (the colouring itself lives in SyntaxHighlight.cpp)
    // ------------------------------------------------------------------
    void Highlight(Doc& d) {
        if (d.styling) return;
        d.styling = true;
        SyntaxHighlight::Apply(d.editor);
        d.gutter->Refresh();
        d.styling = false;
    }

    // ------------------------------------------------------------------
    //  Editor behaviour
    // ------------------------------------------------------------------
    static wchar_t CharAt(wxTextCtrl* ed, long pos) {
        if (pos < 0 || pos >= ed->GetLastPosition()) return 0;
        wxString s = ed->GetRange(pos, pos + 1);
        return s.empty() ? 0 : (wchar_t)s[0].GetValue();
    }

    static wchar_t CloserFor(wchar_t open) {
        switch (open) {
        case L'(': return L')';
        case L'[': return L']';
        case L'{': return L'}';
        default:   return 0;
        }
    }

    static bool IsCloser(wchar_t c) { return c == L')' || c == L']' || c == L'}'; }

    // "class Foo", "struct Foo", "union Foo", "enum Foo", "typedef struct [Foo]"
    struct TypeHeader {
        bool match = false;
        bool isTypedef = false;
        wxString name;
    };

    static TypeHeader ParseTypeHeader(const wxString& text) {
        static const std::wregex re(
            LR"(^\s*(typedef\s+)?(class|struct|union|enum)(?:\s+([A-Za-z_][A-Za-z0-9_]*))?\s*$)");
        TypeHeader h;
        std::wstring s = text.ToStdWstring();
        std::wsmatch m;
        if (std::regex_match(s, m, re)) {
            h.match = true;
            h.isTypedef = m[1].matched;
            if (m[3].matched) h.name = wxString(m[3].str());
        }
        return h;
    }

    // text that follows the closing brace:  ";"  or  " Name;"  for typedefs
    static wxString TypeTail(const TypeHeader& h) {
        if (!h.isTypedef) return ";";
        return h.name.empty() ? wxString(" ;") : (" " + h.name + ";");
    }

    bool DocAlive(const Doc* d) const {
        for (const auto& p : m_docs) if (p.get() == d) return true;
        return false;
    }

    // Inserts exactly ONE line break, the same way a real Enter key press does.
    // (WriteText("\n") can produce two breaks in a rich-edit control, which looked like Enter being pressed twice.)
    void InsertNewline(wxTextCtrl* ed) {
#ifdef __WXMSW__
        m_synthEnter = true;                                         // OnEditorChar must not react to this key
        ::SendMessage((HWND)ed->GetHWND(), WM_CHAR, (WPARAM)'\r', 0);
        m_synthEnter = false;
#else
        ed->WriteText("\n");
#endif
    }

    // Writes the parts one after another with a single line break between each two parts.
    void InsertBreaks(wxTextCtrl* ed, const std::vector<wxString>& parts) {
        for (size_t i = 0; i < parts.size(); ++i) {
            if (i > 0) InsertNewline(ed);
            if (!parts[i].empty()) ed->WriteText(parts[i]);
        }
    }

    void OnEditorText(Doc& d, wxCommandEvent& e) {
        e.Skip();
        if (d.styling) return;
        if (!d.modified) { d.modified = true; UpdateTitle(); }
        m_hlTimer.Start(150, wxTIMER_ONE_SHOT);   // re-highlight shortly after typing stops
    }

    void OnEditorChar(Doc& d, wxKeyEvent& event) {
        wxTextCtrl* ed = d.editor;

        // the Enter key we send ourselves in InsertNewline() goes straight to the control
        if (m_synthEnter) { event.Skip(); return; }

        // Ctrl+X / Alt+X shortcuts pass through (AltGr = Ctrl+Alt is allowed for typing brackets)
        if (event.ControlDown() != event.AltDown()) { event.Skip(); return; }

        long from = 0, to = 0;
        ed->GetSelection(&from, &to);
        const bool hasSel = (from != to);
        const int code = event.GetKeyCode();
        const wchar_t ch = (wchar_t)event.GetUnicodeKey();

        // Tab -> 4 spaces
        if (code == WXK_TAB && !event.ShiftDown()) {
            ed->WriteText("    ");
            return;
        }

        // Backspace between an empty pair () [] {} "" '' removes both
        if (code == WXK_BACK && !hasSel) {
            long pos = ed->GetInsertionPoint();
            wchar_t prev = CharAt(ed, pos - 1), next = CharAt(ed, pos);
            if (prev && next && ((CloserFor(prev) == next) ||
                ((prev == L'"' || prev == L'\'') && prev == next))) {
                ed->Remove(pos - 1, pos + 1);
                return;
            }
            event.Skip();
            return;
        }

        // Enter
        if ((code == WXK_RETURN || code == WXK_NUMPAD_ENTER) && !hasSel) {
            long pos = ed->GetInsertionPoint();
            long col = 0, line = 0;
            ed->PositionToXY(pos, &col, &line);
            wxString lineText = ed->GetLineText(line);

            wxString indent;
            for (size_t i = 0; i < lineText.length(); ++i) {
                wxUniChar c = lineText[i];
                if (c == ' ' || c == '\t') indent += c; else break;
            }
            wxString beforeRaw = lineText.Left(col);
            wxString after = lineText.Mid(col);
            wxString before = beforeRaw;
            before.Trim();                      // right trim
            wxString afterTrim = after;
            afterTrim.Trim(false);
            afterTrim.Trim();
            wxString inner = indent + "    ";

            // (1) "class Name" / "struct Name" / "typedef struct Name" + Enter  ->  { ... };
            if (afterTrim.empty()) {
                TypeHeader h = ParseTypeHeader(before);
                if (h.match && (h.isTypedef || !h.name.empty())) {
                    long trailing = (long)(beforeRaw.length() - before.length());
                    if (trailing > 0) { ed->Remove(pos - trailing, pos); pos -= trailing; }
                    ed->SetInsertionPoint(pos);
                    InsertBreaks(ed, { wxString(" {"), inner, indent + "}" + TypeTail(h) });
                    ed->SetInsertionPoint(ed->XYToPosition((long)inner.length(), line + 1));   // inside the braces
                    return;
                }
            }

            // (2) Enter right after a '{'
            const bool openBrace = !before.empty() && before.Last() == '{';
            if (openBrace) {
                if (!after.empty() && after[0] == '}') {          // between { and }
                    TypeHeader h = ParseTypeHeader(before.Left(before.length() - 1));
                    const bool needTail = h.match && afterTrim == "}";
                    InsertBreaks(ed, { wxString(), inner, indent });
                    if (needTail) {                               // add the ';' (or " Name;") after the }
                        long q = ed->GetInsertionPoint();         // right before the existing '}'
                        ed->SetInsertionPoint(q + 1);
                        ed->WriteText(TypeTail(h));
                    }
                    ed->SetInsertionPoint(ed->XYToPosition((long)inner.length(), line + 1));
                }
                else {
                    InsertBreaks(ed, { wxString(), inner });
                }
                return;
            }

            // (3) normal Enter: the control inserts the line break itself, then we add the indentation
            event.Skip();
            if (!indent.empty()) {
                Doc* dp = &d;
                CallAfter([this, dp, indent]() { if (DocAlive(dp)) dp->editor->WriteText(indent); });
            }
            return;
        }

        // Opening bracket -> auto close
        if (ch == L'(' || ch == L'[' || ch == L'{') {
            if (hasSel) { event.Skip(); return; }
            long pos = ed->GetInsertionPoint();
            wchar_t nx = CharAt(ed, pos);
            if (nx == 0 || iswspace(nx) || IsCloser(nx)) {
                wxString pair;
                pair += ch;
                pair += CloserFor(ch);
                ed->WriteText(pair);
                ed->SetInsertionPoint(pos + 1);
                return;
            }
            event.Skip();
            return;
        }

        // Closing bracket -> type over an auto-inserted one
        if (IsCloser(ch) && !hasSel) {
            long pos = ed->GetInsertionPoint();
            if (CharAt(ed, pos) == ch) { ed->SetInsertionPoint(pos + 1); return; }
            event.Skip();
            return;
        }

        // Quotes
        if (ch == L'"' || ch == L'\'') {
            if (hasSel) { event.Skip(); return; }
            long pos = ed->GetInsertionPoint();
            wchar_t nx = CharAt(ed, pos), pv = CharAt(ed, pos - 1);
            if (nx == ch) { ed->SetInsertionPoint(pos + 1); return; }
            if (iswalnum(pv) || pv == L'\\') { event.Skip(); return; }
            if (nx == 0 || iswspace(nx) || IsCloser(nx)) {
                wxString pair;
                pair += ch;
                pair += ch;
                ed->WriteText(pair);
                ed->SetInsertionPoint(pos + 1);
                return;
            }
        }

        event.Skip();
    }

    // ------------------------------------------------------------------
    //  Run: the work is done by RunSystem (external Command Prompt window)
    // ------------------------------------------------------------------
    void OnRun(wxCommandEvent&) {
        Doc* d = Cur();
        if (!d) return;

        std::string code = std::string(d->editor->GetValue().utf8_str());
        const bool balanced = editor.areBracketsBalanced(code);

        if (!RunSystem::CompileAndRun(this, d->Name(), d->path, code, ++m_runCounter))
            return;

        if (!balanced) SetState("Launched - warning: brackets are not balanced", true);
        else SetState("Launched in Command Prompt");
    }

    // ------------------------------------------------------------------
    //  File handling
    // ------------------------------------------------------------------
    bool ConfirmDiscard(Doc& d) {
        if (!d.modified) return true;
        wxMessageDialog dlg(this, "Save changes to " + d.Name() + "?", "CodeNest",
            wxYES_NO | wxCANCEL | wxICON_QUESTION);
        dlg.SetYesNoCancelLabels("Save", "Don't Save", "Cancel");
        int r = dlg.ShowModal();
        if (r == wxID_CANCEL) return false;
        if (r == wxID_YES) return SaveDoc(d, false);
        return true;
    }

    bool SaveDoc(Doc& d, bool saveAs) {
        wxString path = d.path;
        if (saveAs || path.empty()) {
            wxFileDialog dlg(this, "Save C File", "", d.Name(),
                "C files (*.c)|*.c|All files (*.*)|*.*",
                wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
            if (dlg.ShowModal() == wxID_CANCEL) return false;
            path = dlg.GetPath();
        }
        std::string content = std::string(d.editor->GetValue().utf8_str());
        if (!fm.save(path, content)) {
            wxMessageBox("Could not write the file:\n" + path, "CodeNest", wxOK | wxICON_ERROR, this);
            return false;
        }
        d.path = path;
        d.modified = false;
        UpdateTitle();
        SetState("Saved");
        return true;
    }

    static wxString BytesToString(const std::string& s) {
        if (s.empty()) return wxString();
        wxString u = wxString::FromUTF8(s.c_str(), s.size());
        if (!u.empty()) return u;
        wxString l(s.c_str(), wxConvLocal);
        if (!l.empty()) return l;
        return wxString(s.c_str(), wxConvISO8859_1);
    }

    void OpenPath(const wxString& path) {
        int existing = FindDocByPath(path);
        if (existing >= 0) { SwitchTo(existing); return; }

        std::string content;
        if (!fm.load(path, content)) {
            wxMessageBox("Could not read the file:\n" + path, "CodeNest", wxOK | wxICON_ERROR, this);
            return;
        }
        if (content.size() >= 3 && (unsigned char)content[0] == 0xEF &&
            (unsigned char)content[1] == 0xBB && (unsigned char)content[2] == 0xBF)
            content.erase(0, 3);   // strip UTF-8 BOM
        // CRLF -> LF (the editor control keeps its own line ends)
        content.erase(std::remove(content.begin(), content.end(), '\r'), content.end());
        wxString text = BytesToString(content);

        Doc* cur = Cur();
        if (cur && cur->path.empty() && !cur->modified) {      // reuse an untouched empty tab
            cur->path = path;
            cur->editor->ChangeValue(text);
            Highlight(*cur);
            cur->editor->SetInsertionPoint(0);
            UpdateTitle();
        }
        else {
            Doc* d = NewDoc(text, path);
            d->editor->SetInsertionPoint(0);
        }
    }

    void OnNew(wxCommandEvent&) { NewDoc(SampleCode(), wxString()); }

    void OnOpen(wxCommandEvent&) {
        wxFileDialog dlg(this, "Open C File(s)", "", "",
            "C files (*.c;*.h)|*.c;*.h|All files (*.*)|*.*",
            wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
        if (dlg.ShowModal() == wxID_CANCEL) return;
        wxArrayString paths;
        dlg.GetPaths(paths);
        for (size_t i = 0; i < paths.GetCount(); ++i) OpenPath(paths[i]);
    }

    void OnSave(wxCommandEvent&) { if (Doc* d = Cur()) SaveDoc(*d, false); }
    void OnSaveAs(wxCommandEvent&) { if (Doc* d = Cur()) SaveDoc(*d, true); }
    void OnCloseTab(wxCommandEvent&) { CloseDoc(m_cur); }
    void OnExit(wxCommandEvent&) { Close(); }

    void OnAbout(wxCommandEvent&) {
        wxMessageBox("CodeNest\nA student-friendly C IDE\n\n"
            "F5 / Ctrl+R  -  compile and run (opens a Command Prompt window)\n"
            "Ctrl+N / Ctrl+W / Ctrl+Tab  -  new / close / switch tab\n"
            "Ctrl+B  -  show / hide the Explorer panel\n"
            "Ctrl+Shift+O  -  open a folder in the Explorer\n"
            "Ctrl + / Ctrl -  -  zoom\n\n"
            "Type  struct Name  (or class / union / enum)  and press Enter to get { };\n\n"
            "Requires gcc (MinGW) in your PATH to run programs.",
            "About CodeNest", wxOK | wxICON_INFORMATION, this);
    }

    void OnClose(wxCloseEvent& e) {
        if (e.CanVeto()) {
            for (size_t i = 0; i < m_docs.size(); ++i) {
                if (!m_docs[i]->modified) continue;
                SwitchTo((int)i);
                if (!ConfirmDiscard(*m_docs[i])) { e.Veto(); return; }
            }
        }
        m_pollTimer.Stop();
        m_hlTimer.Stop();
        Destroy();
    }
};

class MyApp : public wxApp {
public:
    bool OnInit() override {
        SetAppName("CodeNest");          // names the settings entry used by wxConfig
        SetVendorName("CodeNest");
        MyFrame* frame = new MyFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(MyApp);
