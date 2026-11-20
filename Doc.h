// ============================================================================
//  Doc.h  -  One open document (a tab): page, editor, gutter, path, state
//
//  OWNER: Archit (Team Lead) - document system
// ============================================================================
#ifndef CODENEST_DOC_H
#define CODENEST_DOC_H

#include <wx/filename.h>
#include <wx/string.h>

class wxPanel;
class wxTextCtrl;
class Gutter;

struct Doc {
    wxPanel* page = nullptr;
    wxTextCtrl* editor = nullptr;
    Gutter* gutter = nullptr;
    wxString path;
    int untitledNo = 0;
    bool modified = false;
    bool styling = false;

    wxString Name() const {
        if (path.empty()) return wxString::Format("untitled-%d.c", untitledNo);
        return wxFileName(path).GetFullName();
    }
};

#endif // CODENEST_DOC_H
