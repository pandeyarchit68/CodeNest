// ============================================================================
//  RunSystem.h  -  External compile & run (real Command Prompt window)
//
//  OWNER: Yuvraj
//
//  Writes the source into a fresh temporary folder, generates a batch file
//  (gcc + run + "Press any key"), and starts it in its own console window
//  with CreateProcess. There is no internal output panel.
// ============================================================================
#ifndef CODENEST_RUNSYSTEM_H
#define CODENEST_RUNSYSTEM_H

#include <string>

#include <wx/string.h>

class wxWindow;

namespace RunSystem {

    // Deletes the run folders left over from earlier sessions.
    void CleanTempRoot();

    // Compiles and runs the given source in a Command Prompt window.
    //   parent    - window used as parent for error message boxes
    //   docName   - file name shown in errors / window title (e.g. "main.c")
    //   docPath   - full path of the saved file (empty for an untitled tab);
    //               the program runs in that file's folder
    //   code      - UTF-8 source text
    //   runNumber - unique counter so that several runs never share a folder
    // Returns true when the console window was launched.
    bool CompileAndRun(wxWindow* parent, const wxString& docName, const wxString& docPath,
        const std::string& code, int runNumber);

}

#endif // CODENEST_RUNSYSTEM_H
