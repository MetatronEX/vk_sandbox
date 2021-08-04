#include "win32.hpp"

namespace win32
{
    void window::::setup_window()
    {
        hInstance = GetModuleHandle(NULL);

        WNDCLASSEX WC{};

        WC.cbSize = sizeof(WNDCLASSEX);
        WC.style = CS_HREDRAW | CS_VREDRAW;
        WC.lpfnWndProc = fp_wndproc;
        WC.cbClsExtra = 0;
        WC.cbWndExtra = 0;
        WC.hInstance = hInstance;
        WC.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        WC.hCursor = LoadCurosr(NULL, IDC_ARROW);
        WC.hbrbackground = (HBRUSH)GetStackObject(BLACK_BRUSH);
        WC.lpszMenuName = NULL;
        WC.lpszClassName = application_name;
        WC.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

        if(!RegisterClassEx(&WC))
        {
            std::cout << "Windows class registration failed!\n";
            fflush(stdcout);
            exit(1);
        }

        auto width = GetSystemMetrics(SM_CXSCREEN);
        auto height = GetSystemMetrics(SM_CYSCREEN);

        if (fullscreen)
        {
            if( (screen_width != static_cast<uint32_t>(width)) &&
                (screen_height != static_cast<uint32_t>(height)) )
            {
                DEVMODE DMSS{};
                DMSS.dmSize = sizeof(DMSS);
                DMSS.dmPelWidth = width;
                DMSS.dmPelHeight = height;
                DMSS.dmBitsPerPel = 32;
                DMSS.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

                if(ChangeDisplaySettings(&DMSS, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
                {
                    fullscreen = false;
                }

                screen_width = width;
                screen_height = height;
            }
        }

        DWORD dwExStyle = (fullscreen) ? (WS_EX_APPWINDOW) : (WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);
        DWORD dwStyle = (fullscreen) ? (WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN) : (WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);

        RECT WR;
        WR.left = 0L;
        WR.top = 0L;
        WR.right = (fullscreen) ? static_cast<long>(screen_width) : static_cast<long>(width);
        WR.bottom = (fullscreen) ? static_cast<long>(screen_height) : static_cast<long>(height);

        AdjustWindowRectEx(&WR, dwStyle, FALSE, dwExStyle);

        hWnd = CreateWindowEx(0, application_name, application_name,
        dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        0, 0,
        WR.right - WR.left,
        WR.bottom - WR.top,
        NULL,
        NULL,
        hInstance,
        NULL);

        if (!fullscreen)
        {
            auto x = (GetSystemMetrics(SM_CXSCREEN) - WR.right) / 2;
            auto y = (GetSystemMetrics(SM_CYSCREEN) - WR.bottom) / 2;
            SetWindowPos(hWnd, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
        }

        if (!hWnd)
        {
            std::cout << "Could not create window!\n";
            fflush(stdout);
            exit(1);
        }

        ShowWindow(hWnd, SW_SHOW);
        SetForegroundWindow(hWnd);
        SetFocus(hWnd);
    }

    void window::setup_DPI_awareness()
    {
        typedef HRESULT *(__stdcall *SetProcessDpiAwarenessFunc)(PROCESS_DPI_AWARENESS);

        HMODULE sh_core = LoadLibraryA("Shcore.dll");

        if(sh_core)
        {
            SetProcessDpiAwarenessFunc setProcessDpiAwareness =
			    (SetProcessDpiAwarenessFunc)GetProcAddress(shCore, "SetProcessDpiAwareness");

            if(setProcessDpiAwareness)
                setProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

            FreeLibrary(sh_core);
        }
    }
}