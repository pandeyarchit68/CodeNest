// ============================================================================
//  Theme.h  -  Colour palettes and theme switching
//
//  OWNER: Anushka
//
// ============================================================================
#ifndef CODENEST_THEME_H
#define CODENEST_THEME_H

#include <vector>

#include <wx/defs.h>
#include <wx/colour.h>

#ifdef __WXMSW__
#include <wx/msw/wrapwin.h>
#endif

struct Palette {
    const char* name = "";
    bool dark = true;          
    bool rainbow = false;      
    wxColour bg, chrome, border, text, textDim, accent, onAccent;
    wxColour logo;             
    wxColour sidebar;          
    wxColour tabIdle, tabHover, btnHover, btnDown, gutterBg, gutterLine;
    wxColour keyword, control, type, str, comment, number, function;
};

namespace Theme {
    inline wxColour C(int r, int g, int b) {
        return wxColour((unsigned char)r, (unsigned char)g, (unsigned char)b);
    }

    inline Palette Make(const char* name, bool dark,
        wxColour bg, wxColour chrome, wxColour border, wxColour text, wxColour textDim,
        wxColour accent, wxColour onAccent,
        wxColour tabIdle, wxColour tabHover, wxColour btnHover, wxColour btnDown,
        wxColour gutterBg, wxColour gutterLine,
        wxColour keyword, wxColour control, wxColour type, wxColour str,
        wxColour comment, wxColour number, wxColour function) {
        Palette p;
        p.name = name; p.dark = dark;
        p.bg = bg; p.chrome = chrome; p.border = border; p.text = text; p.textDim = textDim;
        p.accent = accent; p.onAccent = onAccent;
        p.logo = accent;                       
        p.sidebar = chrome;                    
        p.tabIdle = tabIdle; p.tabHover = tabHover; p.btnHover = btnHover; p.btnDown = btnDown;
        p.gutterBg = gutterBg; p.gutterLine = gutterLine;
        p.keyword = keyword; p.control = control; p.type = type; p.str = str;
        p.comment = comment; p.number = number; p.function = function;
        return p;
    }

    
    inline const std::vector<Palette>& All() {
        static const std::vector<Palette> all = []() {
            std::vector<Palette> v;

            v.push_back(Make("Dark", true,
                C(30, 30, 30), C(37, 37, 38), C(60, 60, 60), C(212, 212, 212), C(133, 133, 133),
                C(0, 122, 204), C(255, 255, 255),
                C(45, 45, 48), C(52, 52, 55), C(55, 55, 58), C(70, 70, 72),
                C(30, 30, 30), C(45, 45, 45),
                C(86, 156, 214), C(197, 134, 192), C(78, 201, 176), C(206, 145, 120),
                C(106, 153, 85), C(181, 206, 168), C(220, 220, 170)));

            v.push_back(Make("Light", false,
                C(255, 255, 255), C(243, 243, 243), C(221, 221, 221), C(31, 31, 31), C(150, 150, 150),
                C(0, 120, 212), C(255, 255, 255),
                C(236, 236, 236), C(228, 228, 228), C(228, 228, 228), C(210, 210, 210),
                C(255, 255, 255), C(235, 235, 235),
                C(0, 0, 255), C(175, 0, 219), C(38, 127, 153), C(163, 21, 21),
                C(0, 128, 0), C(9, 134, 88), C(121, 94, 38)));

            
            Palette sr = Make("Sunshine Rainbow", false,
                C(255, 251, 243), C(255, 214, 229), C(245, 186, 208), C(62, 54, 98), C(176, 142, 170),
                C(125, 200, 244), C(30, 52, 96),
                C(255, 229, 239), C(255, 205, 224), C(207, 232, 252), C(178, 217, 247),
                C(255, 247, 236), C(248, 228, 214),
                C(232, 62, 140), C(214, 51, 132), C(240, 98, 160), C(70, 156, 226),
                C(226, 172, 22), C(150, 96, 214), C(52, 120, 200));
            sr.logo = C(255, 176, 0);          // sunny yellow
            sr.sidebar = C(255, 246, 222);     // warm cream explorer panel
            sr.rainbow = true;                 // pastel rainbow stripe under the header
            v.push_back(sr);

            v.push_back(Make("Ocean", true,
                C(11, 29, 51), C(15, 38, 66), C(30, 64, 104), C(200, 220, 240), C(110, 140, 175),
                C(0, 150, 199), C(255, 255, 255),
                C(20, 48, 82), C(28, 60, 100), C(26, 58, 96), C(36, 78, 126),
                C(11, 29, 51), C(24, 50, 84),
                C(100, 181, 246), C(206, 147, 216), C(77, 208, 225), C(255, 171, 145),
                C(100, 150, 120), C(174, 213, 129), C(255, 213, 79)));

            v.push_back(Make("Forest", true,
                C(24, 32, 24), C(30, 42, 30), C(52, 72, 52), C(212, 224, 205), C(120, 145, 115),
                C(46, 125, 50), C(255, 255, 255),
                C(38, 52, 38), C(46, 64, 46), C(44, 62, 44), C(58, 82, 58),
                C(24, 32, 24), C(40, 56, 40),
                C(129, 199, 132), C(255, 183, 77), C(77, 182, 172), C(255, 204, 128),
                C(120, 144, 100), C(174, 213, 129), C(240, 230, 140)));

            v.push_back(Make("Dracula", true,
                C(40, 42, 54), C(33, 34, 44), C(68, 71, 90), C(248, 248, 242), C(130, 140, 175),
                C(189, 147, 249), C(30, 30, 40),
                C(52, 55, 70), C(62, 65, 84), C(62, 65, 84), C(78, 82, 104),
                C(40, 42, 54), C(56, 58, 74),
                C(189, 147, 249), C(255, 121, 198), C(139, 233, 253), C(241, 250, 140),
                C(98, 114, 164), C(255, 184, 108), C(80, 250, 123)));

            v.push_back(Make("Solarized Light", false,
                C(253, 246, 227), C(238, 232, 213), C(220, 212, 190), C(88, 110, 117), C(147, 161, 161),
                C(38, 139, 210), C(255, 255, 255),
                C(232, 225, 203), C(224, 216, 192), C(228, 221, 198), C(214, 206, 180),
                C(238, 232, 213), C(220, 212, 190),
                C(133, 153, 0), C(203, 75, 22), C(181, 137, 0), C(42, 161, 152),
                C(147, 161, 161), C(211, 54, 130), C(38, 139, 210)));

            v.push_back(Make("High Contrast", true,
                C(0, 0, 0), C(20, 20, 20), C(110, 110, 110), C(255, 255, 255), C(200, 200, 200),
                C(255, 214, 0), C(0, 0, 0),
                C(30, 30, 30), C(55, 55, 55), C(60, 60, 60), C(90, 90, 90),
                C(0, 0, 0), C(80, 80, 80),
                C(0, 191, 255), C(255, 105, 180), C(0, 255, 200), C(255, 165, 0),
                C(124, 224, 124), C(173, 255, 47), C(255, 255, 0)));

            return v;
            }();
        return all;
    }

    inline int& IndexRef() { static int i = 0; return i; }
    inline const Palette& P() { return All()[IndexRef()]; }
    inline int Count() { return (int)All().size(); }
    inline int Index() { return IndexRef(); }
    inline const char* NameOf(int i) { return All()[i].name; }
    inline void Set(int i) { if (i >= 0 && i < Count()) IndexRef() = i; }
    inline bool IsDark() { return P().dark; }
    inline bool IsRainbow() { return P().rainbow; }

    inline wxColour Bg() { return P().bg; }
    inline wxColour Chrome() { return P().chrome; }
    inline wxColour Border() { return P().border; }
    inline wxColour Text() { return P().text; }
    inline wxColour TextDim() { return P().textDim; }
    inline wxColour Accent() { return P().accent; }
    inline wxColour OnAccent() { return P().onAccent; }
    inline wxColour Logo() { return P().logo; }
    inline wxColour Sidebar() { return P().sidebar; }
    inline wxColour TabIdle() { return P().tabIdle; }
    inline wxColour TabHover() { return P().tabHover; }
    inline wxColour BtnHover() { return P().btnHover; }
    inline wxColour BtnDown() { return P().btnDown; }
    inline wxColour GutterBg() { return P().gutterBg; }
    inline wxColour GutterLine() { return P().gutterLine; }

    // the green Run button and the orange "busy" status look the same in every theme
    inline wxColour Run() { return wxColour(35, 134, 54); }
    inline wxColour RunHover() { return wxColour(46, 160, 67); }
    inline wxColour RunDown() { return wxColour(28, 108, 44); }
    inline wxColour Busy() { return wxColour(202, 81, 0); }

    inline wxColour Keyword() { return P().keyword; }
    inline wxColour Control() { return P().control; }
    inline wxColour Type() { return P().type; }
    inline wxColour String() { return P().str; }
    inline wxColour Comment() { return P().comment; }
    inline wxColour Number() { return P().number; }
    inline wxColour Function() { return P().function; }
}


#ifdef __WXMSW__

inline void ApplyTitleBarTheme(HWND hwnd, bool dark, const wxColour& caption, const wxColour& captionText) {
    HMODULE dwm = LoadLibraryW(L"dwmapi.dll");
    if (!dwm) return;
    typedef HRESULT(WINAPI* SetAttrFn)(HWND, DWORD, LPCVOID, DWORD);
    SetAttrFn fn = reinterpret_cast<SetAttrFn>(GetProcAddress(dwm, "DwmSetWindowAttribute"));
    if (fn) {
        BOOL on = dark ? TRUE : FALSE;
        fn(hwnd, 20, &on, sizeof(on));   
        fn(hwnd, 19, &on, sizeof(on));   
        COLORREF cap = RGB(caption.Red(), caption.Green(), caption.Blue());
        COLORREF txt = RGB(captionText.Red(), captionText.Green(), captionText.Blue());
        fn(hwnd, 35, &cap, sizeof(cap)); 
        fn(hwnd, 36, &txt, sizeof(txt)); 
        fn(hwnd, 34, &cap, sizeof(cap)); 
    }
    FreeLibrary(dwm);
}


inline void ApplyControlTheme(HWND hwnd, bool dark) {
    HMODULE ux = LoadLibraryW(L"uxtheme.dll");
    if (!ux) return;
    typedef HRESULT(WINAPI* SetThemeFn)(HWND, LPCWSTR, LPCWSTR);
    SetThemeFn fn = reinterpret_cast<SetThemeFn>(GetProcAddress(ux, "SetWindowTheme"));
    if (fn) fn(hwnd, dark ? L"DarkMode_Explorer" : L"Explorer", nullptr);
    FreeLibrary(ux);
}
#endif

#endif // CODENEST_THEME_H
