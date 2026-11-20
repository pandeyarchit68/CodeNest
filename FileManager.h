// ============================================================================
//  FileManager.h  -  File save / load (unicode-safe paths)
//
//  OWNER: Archit (Team Lead) - file handling coordination
// ============================================================================
#ifndef CODENEST_FILEMANAGER_H
#define CODENEST_FILEMANAGER_H

#include <string>

#include <wx/string.h>

class FileManager {
public:
    bool save(const wxString& path, const std::string& content) const;
    bool load(const wxString& path, std::string& content) const;
};

#endif // CODENEST_FILEMANAGER_H
