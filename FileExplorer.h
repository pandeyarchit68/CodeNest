#ifndef CODENEST_FILEEXPLORER_H
#define CODENEST_FILEEXPLORER_H

#include <functional>

#include <wx/panel.h>
#include <wx/string.h>
#include <wx/treebase.h>

class wxBoxSizer;
class wxStaticText;
class wxTreeCtrl;
class FlatButton;

class FileExplorer : public wxPanel {
public:
    std::function<void(const wxString&)> onOpenFile;   
    std::function<void()> onOpenFolderRequest;      

    explicit FileExplorer(wxWindow* parent);

    void OpenFolder(const wxString& dir);

    wxString GetRootPath() const { return m_root; }

    void ApplyTheme();

private:
    wxString m_root;
    wxBoxSizer* m_col = nullptr;
    wxStaticText* m_title = nullptr;
    wxStaticText* m_hint = nullptr;
    FlatButton* m_openBtn = nullptr;
    wxTreeCtrl* m_tree = nullptr;
    wxPanel* m_border = nullptr;

    void Populate(const wxTreeItemId& item);
    void ShowTree(bool show);
};

#endif 
