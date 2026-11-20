// ============================================================================
//  Editor.cpp  -  Bracket checker (stack based, ignores strings and comments)
//
//  OWNER: Archit (Team Lead) - editor logic shared by the main window
// ============================================================================
#include "Editor.h"

#include <stack>

bool Editor::areBracketsBalanced(const std::string& code) const {
    std::stack<char> st;
    const size_t n = code.size();
    for (size_t i = 0; i < n; ++i) {
        char c = code[i];
        if (c == '/' && i + 1 < n && code[i + 1] == '/') {
            while (i < n && code[i] != '\n') ++i;
            continue;
        }
        if (c == '/' && i + 1 < n && code[i + 1] == '*') {
            i += 2;
            while (i + 1 < n && !(code[i] == '*' && code[i + 1] == '/')) ++i;
            ++i;
            continue;
        }
        if (c == '"' || c == '\'') {
            char q = c;
            ++i;
            while (i < n && code[i] != q && code[i] != '\n') {
                if (code[i] == '\\') ++i;
                ++i;
            }
            continue;
        }
        if (c == '(' || c == '{' || c == '[') st.push(c);
        else if (c == ')' || c == '}' || c == ']') {
            if (st.empty()) return false;
            char top = st.top();
            if ((c == ')' && top != '(') || (c == '}' && top != '{') || (c == ']' && top != '['))
                return false;
            st.pop();
        }
    }
    return st.empty();
}
