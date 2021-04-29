#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

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
			extern std::chrono::steady_clock::time_point start_time;

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

			void update_ubo(uint32_t curr_img);

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

		struct vertex
		{
			glm::vec2 position;
			glm::vec3 color;
			glm::vec2 tex_coord;

			static VkVertexInputBindingDescription get_binding_desc()
			{
				/*
				* A vertex binding describes at which rate to load data from memory throughout the vertices. 
				It specifies the number of bytes between data entries and whether to move to the next data 
				entry after each vertex or after each instance.
				*/
				VkVertexInputBindingDescription bind_desc{};
				/*
				* All of our per-vertex data is packed together in one array, so we're only going to have one 
				binding. The binding parameter specifies the index of the binding in the array of bindings. 
				The stride parameter specifies the number of bytes from one entry to the next, and the 
				inputRate parameter can have one of the following values:

					- VK_VERTEX_INPUT_RATE_VERTEX: Move to the next data entry after each vertex
					- VK_VERTEX_INPUT_RATE_INSTANCE: Move to the next data entry after each instance

				We're not going to use instanced rendering, so we'll stick to per-vertex data.
				*/
				bind_desc.binding = 0;
				bind_desc.stride = sizeof(vertex);
				bind_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

				return bind_desc;
			}

			static std::array<VkVertexInputAttributeDescription, 3>
				get_attr_desc()
			{
				std::array<VkVertexInputAttributeDescription, 3> attr_desc{};

				attr_desc[0].binding = 0;
				attr_desc[0].location = 0;
				attr_desc[0].format = VK_FORMAT_R32G32_SFLOAT;
				attr_desc[0].offset = offsetof(vertex, position);

				attr_desc[1].binding = 0;
				attr_desc[1].location = 1;
				attr_desc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
				attr_desc[1].offset = offsetof(vertex, color);

				attr_desc[2].binding = 0;
				attr_desc[2].location = 2;
				attr_desc[2].format = VK_FORMAT_R32G32_SFLOAT;
				attr_desc[2].offset = offsetof(vertex, tex_coord);

				return attr_desc;
			}
		};

		void create_instance();

		bool check_dev_extension_support(VkPhysicalDevice dev);

		queue_family_indices find_queue_families(VkPhysicalDevice dev);

		bool is_device_suitable(VkPhysicalDevice dev);

		void pick_physical_device();

		void create_logical_device();

		VkImageView create_img_view(VkImage img, VkFormat fmt);

		void create_image_views();

		VkShaderModule create_shader_module(const std::vector<char>& spv);

		void create_render_pass();

		void create_descriptor_set_layout();

		void create_graphics_pipeline();

		void create_framebuffers();

		void create_cmd_pool();

		uint32_t find_mem_type(uint32_t type_filter, VkMemoryPropertyFlags props);

		void create_image(uint32_t tex_w, uint32_t tex_h, VkFormat fmt, VkImageTiling tiling,
			VkImageUsageFlags usage, VkMemoryPropertyFlags props, VkImage& img, VkDeviceMemory& img_mem);

		VkCommandBuffer begin_single_time_cmds();

		void end_single_time_cmds(VkCommandBuffer cmd_buffer);

		void create_texture_image();

		void create_tex_img_view();

		void create_tex_sampler();

		void transition_image_layout(VkImage img, VkFormat fmt, VkImageLayout old_layout, VkImageLayout new_layout);

		void copy_buffer_to_img(VkBuffer buffer, VkImage img, uint32_t w, uint32_t h);

		void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, 
			VkMemoryPropertyFlags props,
			VkBuffer &buffer, VkDeviceMemory &dev_mem);

		void copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);

		void create_vertex_buffer();

		void create_index_buffer();

		void create_uniform_buffers();

		void create_descriptor_pool();

		void create_descriptor_sets();

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