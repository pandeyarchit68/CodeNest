#include "FileExplorer.h"
#include "Theme.h"
#include "UIControls.h"

#include <wx/wx.h>
#include <wx/artprov.h>
#include <wx/filename.h>
#include <wx/imaglist.h>
#include <wx/treectrl.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

namespace {

class PathData : public wxTreeItemData {
public:
    PathData(const wxString& p, bool dir) : path(p), isDir(dir) {}
    wxString path;
    bool isDir;
    bool loaded = false;      
};

wxBitmap IconOrBlank(const wxArtID& id) {
    wxBitmap bmp = wxArtProvider::GetBitmap(id, wxART_OTHER, wxSize(16, 16));
    if (!bmp.IsOk()) bmp = wxBitmap(wxImage(16, 16));
    return bmp;
}

} 

FileExplorer::FileExplorer(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE) {

    SetMinSize(wxSize(FromDIP(250), -1));

    m_title = new wxStaticText(this, wxID_ANY, "EXPLORER");
    m_title->SetFont(wxFont(wxFontInfo(9).FaceName("Segoe UI").Bold()));

    m_openBtn = new FlatButton(this, wxID_ANY, "Open Folder");
    m_openBtn->SetToolTip("Open a folder (Ctrl+Shift+O)");
    m_openBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (onOpenFolderRequest) onOpenFolderRequest();
        });

    m_hint = new wxStaticText(this, wxID_ANY,
        "You have not opened a folder yet.\nClick \"Open Folder\" above to\nbrowse your C files.");
    m_hint->SetFont(wxFont(wxFontInfo(10).FaceName("Segoe UI")));

    m_tree = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxTR_HAS_BUTTONS | wxTR_TWIST_BUTTONS | wxTR_NO_LINES | wxTR_FULL_ROW_HIGHLIGHT |
        wxTR_SINGLE | wxBORDER_NONE);
    m_tree->SetFont(wxFont(wxFontInfo(10).FaceName("Segoe UI")));

    wxImageList* images = new wxImageList(16, 16, true, 2);
    images->Add(IconOrBlank(wxART_FOLDER));
    images->Add(IconOrBlank(wxART_NORMAL_FILE));
    m_tree->AssignImageList(images);

    m_border = new wxPanel(this);
    m_border->SetMinSize(wxSize(1, -1));

    wxBoxSizer* head = new wxBoxSizer(wxHORIZONTAL);
    head->AddSpacer(FromDIP(14));
    head->Add(m_title, 1, wxALIGN_CENTER_VERTICAL);
    head->Add(m_openBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(8));

    m_col = new wxBoxSizer(wxVERTICAL);
    m_col->AddSpacer(FromDIP(8));
    m_col->Add(head, 0, wxEXPAND);
    m_col->AddSpacer(FromDIP(8));
    m_col->Add(m_hint, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(14));
    m_col->Add(m_tree, 1, wxEXPAND);

    wxBoxSizer* outer = new wxBoxSizer(wxHORIZONTAL);
    outer->Add(m_col, 1, wxEXPAND);
    outer->Add(m_border, 0, wxEXPAND);
    SetSizer(outer);

    ShowTree(false);
        
    m_tree->Bind(wxEVT_TREE_ITEM_EXPANDING, [this](wxTreeEvent& e) {
        Populate(e.GetItem());
        e.Skip();
        });

    m_tree->Bind(wxEVT_TREE_ITEM_ACTIVATED, [this](wxTreeEvent& e) {
        PathData* pd = static_cast<PathData*>(m_tree->GetItemData(e.GetItem()));
        if (pd && !pd->isDir && onOpenFile) onOpenFile(pd->path);
        });

    ApplyTheme();
}

void FileExplorer::ShowTree(bool show) {
    m_col->Show(m_tree, show);
    m_col->Show(m_hint, !show);
    Layout();
}

void FileExplorer::Populate(const wxTreeItemId& item) {
    if (!item.IsOk()) return;
    PathData* pd = static_cast<PathData*>(m_tree->GetItemData(item));
    if (!pd || !pd->isDir || pd->loaded) return;
    pd->loaded = true;
    m_tree->DeleteChildren(item);

    struct Entry { wxString name; wxString path; bool dir; };
    std::vector<Entry> list;

    std::error_code ec;
    fs::directory_iterator it(fs::path(pd->path.ToStdWstring()),
        fs::directory_options::skip_permission_denied, ec);
    fs::directory_iterator end;
    while (!ec && it != end) {
        std::error_code ec2;
        const bool dir = it->is_directory(ec2);
        list.push_back({ wxString(it->path().filename().wstring()),
                         wxString(it->path().wstring()), dir });
        it.increment(ec);
    }

    std::sort(list.begin(), list.end(), [](const Entry& a, const Entry& b) {
        if (a.dir != b.dir) return a.dir;
        return a.name.CmpNoCase(b.name) < 0;
        });

    for (const Entry& e : list) {
        const int icon = e.dir ? 0 : 1;
        wxTreeItemId child = m_tree->AppendItem(item, e.name, icon, icon, new PathData(e.path, e.dir));
        if (e.dir) m_tree->SetItemHasChildren(child, true);   // shows the arrow before the folder is read
    }
}

void FileExplorer::OpenFolder(const wxString& dir) {
    m_root = dir;
    m_tree->DeleteAllItems();

    wxArrayString dirs = wxFileName::DirName(dir).GetDirs();
    wxString name = dirs.IsEmpty() ? dir : dirs.Last();

    wxTreeItemId root = m_tree->AddRoot(name.Upper(), 0, 0, new PathData(dir, true));
    m_tree->SetItemBold(root, true);
    m_tree->SetItemHasChildren(root, true);
    Populate(root);
    m_tree->Expand(root);

    ShowTree(true);
}

void FileExplorer::ApplyTheme() {
    const wxColour bg = Theme::Sidebar();

    SetBackgroundColour(bg);
    m_title->SetBackgroundColour(bg);
    m_title->SetForegroundColour(Theme::TextDim());
    m_hint->SetBackgroundColour(bg);
    m_hint->SetForegroundColour(Theme::TextDim());
    m_border->SetBackgroundColour(Theme::Border());

    m_tree->SetBackgroundColour(bg);
    m_tree->SetForegroundColour(Theme::Text());
#ifdef __WXMSW__
    ApplyControlTheme((HWND)m_tree->GetHWND(), Theme::IsDark());
#endif

    m_openBtn->Refresh();
    m_tree->Refresh();
    Refresh();
}
