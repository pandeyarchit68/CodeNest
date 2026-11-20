#include "RunSystem.h"
#include "FileManager.h"

#include <wx/wx.h>
#include <wx/filename.h>

#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace {

fs::path TempRoot() {
    return fs::path(wxFileName::GetTempDir().ToStdWstring()) / L"CodeNest";
}

// characters that would break an echo / title line in a batch file are dropped
std::wstring BatText(const std::wstring& s) {
    std::wstring o;
    for (wchar_t c : s)
        if (c != L'&' && c != L'|' && c != L'<' && c != L'>' && c != L'^' &&
            c != L'%' && c != L'"' && c != L'!' && c != L'(' && c != L')')
            o += c;
    return o;
}


std::wstring BatPath(const std::wstring& s) {
    std::wstring o;
    for (wchar_t c : s) { if (c == L'%') o += L"%%"; else o += c; }
    return o;
}

bool LaunchBatch(const std::wstring& batPath, const std::wstring& workDir) {
#ifdef _WIN32
    std::wstring cmd = L"cmd.exe /S /C \"\"" + batPath + L"\"\"";
    std::vector<wchar_t> buf(cmd.begin(), cmd.end());
    buf.push_back(0);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    BOOL ok = CreateProcessW(nullptr, buf.data(), nullptr, nullptr, FALSE,
        CREATE_NEW_CONSOLE, nullptr,
        workDir.empty() ? nullptr : workDir.c_str(), &si, &pi);
    if (!ok) return false;
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
#else
    (void)batPath; (void)workDir;
    return false;
#endif
}

} 

void RunSystem::CleanTempRoot() {
    std::error_code ec;
    fs::remove_all(TempRoot(), ec);
}

bool RunSystem::CompileAndRun(wxWindow* parent, const wxString& docName, const wxString& docPath,
    const std::string& code, int runNumber) {

#ifndef _WIN32
    wxMessageBox("Running programs is only implemented for Windows.", "CodeNest", wxOK | wxICON_INFORMATION, parent);
    return false;
#endif

    FileManager fm;


    long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    fs::path runDir = TempRoot() / (L"run_" + std::to_wstring(ms) + L"_" + std::to_wstring(runNumber));
    std::error_code ec;
    fs::create_directories(runDir, ec);
    if (ec) {
        wxMessageBox("Could not create the temporary folder:\n" + wxString(runDir.wstring()),
            "CodeNest", wxOK | wxICON_ERROR, parent);
        return false;
    }

    const std::wstring srcName = docName.ToStdWstring();          
    const std::wstring exeName = L"codenest_program.exe";
    const fs::path srcPath = runDir / srcName;

    if (!fm.save(wxString(srcPath.wstring()), code)) {
        wxMessageBox("Could not write the temporary source file:\n" + wxString(srcPath.wstring()),
            "CodeNest", wxOK | wxICON_ERROR, parent);
        return false;
    }


    std::wstring workDir = docPath.empty() ? wxGetHomeDir().ToStdWstring()
        : wxFileName(docPath).GetPath().ToStdWstring();
    if (workDir.empty()) workDir = runDir.wstring();

    std::wstring bat;
    auto line = [&](const std::wstring& s) { bat += s; bat += L"\r\n"; };
    line(L"@echo off");
    line(L"chcp 65001 >nul");
    line(L"title CodeNest - " + BatText(srcName));
    line(L"cd /d \"" + BatPath(runDir.wstring()) + L"\"");
    line(L"echo Compiling " + BatText(srcName) + L" ...");
    line(L"echo.");
    line(L"gcc \"" + BatPath(srcName) + L"\" -o \"" + exeName + L"\"");
    line(L"if errorlevel 1 goto failed");
    line(L"echo Build succeeded. Running...");
    line(L"echo ----------------------------------------");
    line(L"cd /d \"" + BatPath(workDir) + L"\"");
    line(L"\"" + BatPath((runDir / exeName).wstring()) + L"\"");
    line(L"set CN_EXIT=%errorlevel%");
    line(L"echo.");
    line(L"echo ----------------------------------------");
    line(L"echo Program finished with exit code %CN_EXIT%");
    line(L"goto finish");
    line(L":failed");
    line(L"echo.");
    line(L"echo Build failed. Fix the errors above and run again.");
    line(L"echo If gcc is not recognized, install MinGW and add its bin folder to your PATH.");
    line(L":finish");
    line(L"echo.");
    line(L"echo Press any key to close this window . . .");
    line(L"pause >nul");

    const fs::path batPath = runDir / L"run.bat";
    if (!fm.save(wxString(batPath.wstring()), std::string(wxString(bat).utf8_str()))) {
        wxMessageBox("Could not write the temporary run script.", "CodeNest", wxOK | wxICON_ERROR, parent);
        return false;
    }

    if (!LaunchBatch(batPath.wstring(), workDir)) {
        wxMessageBox("Could not open a Command Prompt window.", "CodeNest", wxOK | wxICON_ERROR, parent);
        return false;
    }

    return true;
}
