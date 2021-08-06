#ifndef VK_WIN32_HPP
#define VK_WIN32_HPP

#define VK_USE_PLATFORM_WIN32_KHR

#include "win32.hpp"
#include "vk_systems.hpp"

namespace vkwin32app
{
    struct app
    {
        win32::window   app_window;
        vk::system      vk_system;

        static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        LRESULT CALLBACK appWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        void prime();

        void destroy();
    };
}

#endif