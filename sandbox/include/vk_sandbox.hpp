#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <stdexcept>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <optional>

namespace sandbox
{
	class app
	{
	public:

		static constexpr char* app_name = "sandbox";

		void run();

	private:

		static constexpr unsigned RES_WIDTH = 1280;
		static constexpr unsigned RES_HEIGHT = 720;

		void initialize();
		void app_loop();
		void cleanup();
	};

	namespace vulkan
	{
		extern bool fb_resized;

		namespace debug
		{
			bool check_validation_layer_support();
			
			VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
				VkDebugUtilsMessageSeverityFlagBitsEXT msg_severity,
				VkDebugUtilsMessageTypeFlagsEXT msg_type,
				const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
				void* user_data);
			
			VkResult CreateDebugUtilsMessengerEXT(VkInstance inst, const VkDebugUtilsMessengerCreateInfoEXT*
				create_info, const VkAllocationCallbacks* allocator, VkDebugUtilsMessengerEXT* debug_messenger);
			
			void DestroyDebugUtilsMessengerEXT(VkInstance inst, VkDebugUtilsMessengerEXT debug_messenger,
				const VkAllocationCallbacks* allocator);

			void populate_debug_msg_info(VkDebugUtilsMessengerCreateInfoEXT& debug_info);

			void setup_debug_messenger();
		}

		namespace KHR
		{
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

			void draw_frame();
		}

		struct queue_family_indices
		{
			std::optional<unsigned> graphics_family;
			std::optional<unsigned> present_family;

			bool is_complete()
			{
				return graphics_family.has_value() && present_family.has_value();
			}
		};

		void create_instance();

		bool check_dev_extension_support(VkPhysicalDevice dev);

		queue_family_indices find_queue_families(VkPhysicalDevice dev);

		bool is_device_suitable(VkPhysicalDevice dev);

		void pick_physical_device();

		void create_logical_device();

		void create_image_views();

		VkShaderModule create_shader_module(const std::vector<char>& spv);

		void create_render_pass();

		void create_graphics_pipeline();

		void create_framebuffers();

		void create_cmd_pool();

		void create_cmd_buffers();

		void create_syncs();

		void wait_for_device_completion();

		void destroy_resources();
	}

	namespace glfw
	{
		extern GLFWwindow* window;
		void framebuffer_resize_callback(GLFWwindow* win, int w, int h);
		void glfw_initialization(const unsigned res_width, const unsigned res_height);
		void destroy_resources();
	}

}