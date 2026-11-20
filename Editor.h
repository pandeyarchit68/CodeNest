// ============================================================================
//  Editor.h  -  Bracket checker (stack based, ignores strings and comments)
//
//  OWNER: Archit (Team Lead) - editor logic shared by the main window
// ============================================================================
#ifndef CODENEST_EDITOR_H
#define CODENEST_EDITOR_H

#include <string>

class Editor {
public:
    bool areBracketsBalanced(const std::string& code) const;
};

#endif // CODENEST_EDITOR_H
