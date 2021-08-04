#include "vk_win32.hpp"

namespace vkwin32app
{
    static LRESULT CALLBACK app::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        // wndproc stuff
        switch(uMsg)
        {
            case WM_CLOSE:
                vk_system.prepared = false;
                DestroyWindow(hWnd);
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

        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    VkSurfaceKHR initialize_win32_surface(HWND hWnd, VkInstance _instance, VkAllocationCallbacks* allocation_callbacks)
    {
        VkWin32SurfaceCreateInfoKHR CI{};
        CI.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        CI.hInstance = hInstance;
        CI.hwnd = hWnd;

        VkSurfaceKHR surface;
        OP_SUCCESS(vkCreateWin32SurfaceKHR(instance, &CI, allocation_callbacks, &surface));

        return surface;
    }

    void app::prime()
    {
        app_window.fp_wndproc = WndProc;

        app_window.setup_window();
        app_window.setup_DPI_awareness();

        // it would seem like these guys sure be inside pipelines
        vk_system.setup_instance(false);
        vk_system.surface = initialize_win32_surface(app_window.hWnd, vk_system.instance, vk_system.allocation_callbacks);
        vk_system.pick_physical_device();
        // prime pipeline required features here
        vk_system.setup_logical_device();
        vk_system.setup_graphics_queue();
    }

    void app::destroy()
    {
        vk_system.destroy_system();
    }
}