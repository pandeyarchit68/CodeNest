// ============================================================================
//  SyntaxHighlight.cpp  -  C syntax highlighting for a text control
//
//  OWNER: Anushka
// ============================================================================
#include "SyntaxHighlight.h"
#include "Theme.h"

#include <wx/wx.h>

#include <cwctype>
#include <set>
#include <string>
#include <vector>

namespace {

bool IsLineStart(const std::wstring& t, size_t i) {
    while (i > 0) {
        wchar_t c = t[i - 1];
        if (c == L'\n') return true;
        if (c != L' ' && c != L'\t') return false;
        --i;
    }
    return true;
}

} // namespace

void SyntaxHighlight::Apply(wxTextCtrl* editor) {
    static const std::set<std::wstring> keywords = {
        L"auto", L"const", L"enum", L"extern", L"inline", L"register", L"restrict", L"sizeof",
        L"static", L"struct", L"typedef", L"union", L"volatile", L"NULL", L"true", L"false", L"class"
    };
    static const std::set<std::wstring> control = {
        L"if", L"else", L"for", L"while", L"do", L"switch", L"case", L"default", L"break",
        L"continue", L"return", L"goto"
    };
    static const std::set<std::wstring> types = {
        L"int", L"char", L"float", L"double", L"void", L"long", L"short", L"signed", L"unsigned",
        L"size_t", L"FILE", L"bool", L"int8_t", L"int16_t", L"int32_t", L"int64_t",
        L"uint8_t", L"uint16_t", L"uint32_t", L"uint64_t"
    };

    std::wstring t = editor->GetValue().ToStdWstring();
    const size_t n = t.size();

    struct Span { long s, e; wxColour c; };
    std::vector<Span> spans;
    auto add = [&](size_t s, size_t e, const wxColour& c) {
        if (e > s) spans.push_back({ (long)s, (long)e, c });
        };

    if (n < 300000) {
        size_t i = 0;
        while (i < n) {
            wchar_t c = t[i];
            if (c == L'/' && i + 1 < n && t[i + 1] == L'/') {
                size_t e = t.find(L'\n', i);
                if (e == std::wstring::npos) e = n;
                add(i, e, Theme::Comment());
                i = e;
            }
            else if (c == L'/' && i + 1 < n && t[i + 1] == L'*') {
                size_t e = t.find(L"*/", i + 2);
                e = (e == std::wstring::npos) ? n : e + 2;
                add(i, e, Theme::Comment());
                i = e;
            }
            else if (c == L'"' || c == L'\'') {
                size_t j = i + 1;
                while (j < n && t[j] != c && t[j] != L'\n') {
                    if (t[j] == L'\\' && j + 1 < n) ++j;
                    ++j;
                }
                if (j < n && t[j] == c) ++j;
                add(i, j, Theme::String());
                i = j;
            }
            else if (c == L'#' && IsLineStart(t, i)) {
                size_t j = i + 1;
                while (j < n && (t[j] == L' ' || t[j] == L'\t')) ++j;
                size_t w = j;
                while (w < n && iswalpha(t[w])) ++w;
                std::wstring directive = t.substr(j, w - j);
                add(i, w, Theme::Control());
                i = w;
                if (directive == L"include") {
                    size_t k = i;
                    while (k < n && (t[k] == L' ' || t[k] == L'\t')) ++k;
                    if (k < n && t[k] == L'<') {
                        size_t e = t.find(L'>', k);
                        size_t nl = t.find(L'\n', k);
                        if (e != std::wstring::npos && (nl == std::wstring::npos || e < nl)) {
                            add(k, e + 1, Theme::String());
                            i = e + 1;
                        }
                    }
                }
            }
            else if (iswdigit(c) || (c == L'.' && i + 1 < n && iswdigit(t[i + 1]))) {
                size_t j = i;
                while (j < n && (iswalnum(t[j]) || t[j] == L'.')) ++j;
                add(i, j, Theme::Number());
                i = j;
            }
            else if (iswalpha(c) || c == L'_') {
                size_t j = i;
                while (j < n && (iswalnum(t[j]) || t[j] == L'_')) ++j;
                std::wstring word = t.substr(i, j - i);
                if (control.count(word)) add(i, j, Theme::Control());
                else if (keywords.count(word)) add(i, j, Theme::Keyword());
                else if (types.count(word)) add(i, j, Theme::Type());
                else {
                    size_t k = j;
                    while (k < n && (t[k] == L' ' || t[k] == L'\t')) ++k;
                    if (k < n && t[k] == L'(') add(i, j, Theme::Function());
                }
                i = j;
            }
            else {
                ++i;
            }
        }
    }

    editor->Freeze();
    editor->SetStyle(0, (long)n, wxTextAttr(Theme::Text()));
    for (const Span& sp : spans)
        editor->SetStyle(sp.s, sp.e, wxTextAttr(sp.c));
    editor->SetDefaultStyle(wxTextAttr(Theme::Text()));
    editor->Thaw();
}
