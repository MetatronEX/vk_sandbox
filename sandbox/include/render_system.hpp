#ifndef RENDER_SYSTEM_HPP
#define RENDER_SYSTEM_HPP

#define VK_USE_PLATFORM_WIN32_KHR
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

#include <chrono>
#include <stdexcept>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <optional>
#include <array>


namespace sandbox
{
	class render_system
	{
		static constexpr unsigned max_frames_in_flight = 2;
		bool fb_resized = false;

		std::chrono::steady_clock::time_point start_time;

		struct swap_chain_support
		{
			VkSurfaceCapabilitiesKHR			capabilities;
			std::vector<VkSurfaceFormatKHR>		formats;
			std::vector<VkPresentModeKHR>		present_modes;
		};

		void create_surface();

		VkSurfaceFormatKHR choose_swap_surface_fmt(const std::vector<VkSurfaceFormatKHR>& available_fmts);

		VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_modes);

		VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);

		swap_chain_support query_sc_support(VkPhysicalDevice dev);

		void create_swap_chain();

		void clean_swap_chain();

		void recreate_swap_chain();

	public:

		GLFWwindow*						window;
		VkInstance						instance;
		VkPhysicalDevice				pd;
		VkDevice						dev;
		VkSurfaceKHR					surface;
		VkSwapchainKHR					wap_chain;

		render_system() = default;
		~render_system() = default;
		render_system(const render_system&) = delete;
		render_system(render_system&&) = delete;
		render_system& operator= (const render_system&) = delete;
		render_system& operator= (render_system&&) = delete;



		void draw_frame();
	};
}

#endif // !RENDER_SYSTEM_HPP
