// ============================================================================
//  FileManager.cpp  -  File save / load (unicode-safe paths)
//
//  OWNER: Archit (Team Lead) - file handling coordination
// ============================================================================
#include "FileManager.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

bool FileManager::save(const wxString& path, const std::string& content) const {
    std::ofstream file(fs::path(path.ToStdWstring()), std::ios::binary);
    if (!file) return false;
    file << content;
    file.close();
    return true;
}

bool FileManager::load(const wxString& path, std::string& content) const {
    std::ifstream file(fs::path(path.ToStdWstring()), std::ios::binary);
    if (!file) return false;
    std::stringstream buffer;
    buffer << file.rdbuf();
    content = buffer.str();
    return true;
}
