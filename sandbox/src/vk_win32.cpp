#include "vk_win32.hpp"

namespace vkwin32app
{
    LRESULT CALLBACK app::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        app* app_ptr = reinterpret_cast<app*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

        if (app_ptr)
            return app_ptr->appWndProc(hwnd, uMsg, wParam, lParam);

        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    LRESULT CALLBACK app::appWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        // wndproc stuff
        switch(uMsg)
        {
            case WM_CLOSE:
                vk_system.prepared = false;
                DestroyWindow(hwnd);
                PostQuitMessage(0);
                break;
            case WM_PAINT:
                ValidateRect(app_window.hWnd, NULL);
                break;
            case WM_SIZE:
                if ( (SIZE_MINIMIZED != wParam) && (vk_system.prepared) )
                {
                    if( vk_system.resizing || (SIZE_MAXIMIZED == wParam) || (SIZE_RESTORED == wParam))
                    {
                        vk_system.new_width = LOWORD(lParam);
                        vk_system.new_height = HIWORD(lParam);
                        vk_system.resize_window();
                    }
                }
                break;
            case WM_ENTERSIZEMOVE:
                vk_system.resizing = true;
                break;
            case WM_EXITSIZEMOVE:
                vk_system.resizing = false;
                break;
        }

        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    VkSurfaceKHR initialize_win32_surface(HWND hWnd, VkInstance _instance, VkAllocationCallbacks* allocation_callbacks)
    {
        VkWin32SurfaceCreateInfoKHR CI{};
        CI.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        CI.hinstance = GetModuleHandle(NULL);
        CI.hwnd = hWnd;

        VkSurfaceKHR surface;
        OP_SUCCESS(vkCreateWin32SurfaceKHR(_instance, &CI, allocation_callbacks, &surface));

        return surface;
    }

    void app::prime()
    {
        app_window.fp_wndproc = WndProc;

        app_window.setup_window();
        app_window.setup_DPI_awareness();

        // it would seem like these guys sure be inside pipelines
        vk_system.instance_extensions = { VK_KHR_SURFACE_EXTENSION_NAME };
        vk_system.instance_extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
        vk_system.setup_instance(false);
        vk_system.surface = initialize_win32_surface(app_window.hWnd, vk_system.instance, vk_system.allocation_callbacks);
        vk_system.prime();
    }

    void app::destroy()
    {
        vk_system.destroy_system();
    }
}