// ============================================================================
//  FileExplorer.h  -  VS Code style file explorer panel (folder tree)
//
//  OWNER: Vanshika
//
//  Shows the contents of an opened folder as a tree. Folders are loaded
//  lazily when expanded. Double-click (or Enter) on a file asks the main
//  window to open it in a tab.
// ============================================================================
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
    std::function<void(const wxString&)> onOpenFile;   // a file was double-clicked
    std::function<void()> onOpenFolderRequest;         // the "Open Folder" button was pressed

    explicit FileExplorer(wxWindow* parent);

    // Shows the given folder as the root of the tree.
    void OpenFolder(const wxString& dir);

    wxString GetRootPath() const { return m_root; }

    // Re-applies the colours of the current theme.
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

#endif // CODENEST_FILEEXPLORER_H
