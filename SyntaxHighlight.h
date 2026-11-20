// ============================================================================
//  SyntaxHighlight.h  -  C syntax highlighting for a text control
//
//  OWNER: Anushka
// ============================================================================
#ifndef CODENEST_SYNTAXHIGHLIGHT_H
#define CODENEST_SYNTAXHIGHLIGHT_H

class wxTextCtrl;

namespace SyntaxHighlight {
    // Re-colours the whole text of the control using the current theme.
    // (The caller is responsible for the "styling" guard flag of the document.)
    void Apply(wxTextCtrl* editor);
}

#endif // CODENEST_SYNTAXHIGHLIGHT_H
