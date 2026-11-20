#ifndef CODENEST_RUNSYSTEM_H
#define CODENEST_RUNSYSTEM_H

#include <string>

#include <wx/string.h>

class wxWindow;

namespace RunSystem {

    
    void CleanTempRoot();

   
    bool CompileAndRun(wxWindow* parent, const wxString& docName, const wxString& docPath,
        const std::string& code, int runNumber);

}

#endif 
