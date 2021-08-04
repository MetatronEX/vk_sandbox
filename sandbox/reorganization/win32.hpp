#ifndef WIN32_HPP
#define WIN32_HPP

#pragma comment(linker, "/subsystem:windows")

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <ShellScalingAPI.h>

#include "common.hpp"

namespace win32
{
    struct window
    {
        HWND        hWnd;
        HINSTANCE   hInstance;

        uint32_t    screen_width;
        uint32_t    screen_height;
        
        const char* application_name;

        WNDPROC     fp_wndproc;

        bool        fullscreen;

        void        setup_window();
        void        setup_DPI_awareness();
    };
}

#endif