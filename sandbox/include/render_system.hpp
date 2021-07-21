#ifndef RENDER_SYSTEM_HPP
#define RENDER_SYSTEM_HPP

#include <vulkan/vulkan.h>

#define VK_USE_PLATFORM_WIN32_KHR
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

#define OP_SUCCESS(X) VK_SUCCESS == X

namespace sandbox
{
	namespace vk
	{
		class render_system
		{

		private:

			static constexpr unsigned max_frames_in_flight = 2;
			bool fb_resized = false;

			std::chrono::steady_clock::time_point start_time;

			void create_surface();

			VkSurfaceFormatKHR choose_swap_surface_fmt(const std::vector<VkSurfaceFormatKHR>& available_fmts);

			VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_modes);

			VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);

			void create_swap_chain();

			void clean_swap_chain();

			void recreate_swap_chain();

			void create_image_views();

		public:

			GLFWwindow* window;
			VkInstance									instance;
			VkPhysicalDevice							pd;
			VkDevice									dev;
			VkSurfaceKHR								surface;
			VkSwapchainKHR								wap_chain;

			VkFormat									sc_img_fmt;
			VkExtent2D									sc_extent;

			VkQueue										compute_queue;
			VkQueue										graphics_queue;
			VkQueue										present_queue;

			VkSwapchainKHR								swap_chain;

			std::vector<VkImage>						sc_images;
			std::vector<VkImageView>					sc_image_views;

			std::vector<VkCommandBuffer>				cmd_buffers;

			std::vector<VkSemaphore>					image_semaphores;
			std::vector<VkSemaphore>					rp_semaphores;
			std::vector<VkFence>						in_flight_fences;
			std::vector<VkFence>						images_in_flight;

			VkAllocationCallbacks* alloc_callback{ nullptr };

			size_t										curr_frame{ 0 };

			render_system() = default;
			~render_system() = default;
			render_system(const render_system&) = delete;
			render_system(render_system&&) = delete;
			render_system& operator= (const render_system&) = delete;
			render_system& operator= (render_system&&) = delete;

			void draw_frame();

			void create_instance();
			void pick_physical_device();
			void create_logical_device();

			VkFormat find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags feats);
			VkFormat find_depth_format();
			bool has_stencil(VkFormat fmt);
			
			VkImageView create_img_view(const VkImageViewCreateInfo& iv_info);
			VkShaderModule create_shader_module(const VkShaderModuleCreateInfo& create_info);
			VkRenderPass create_render_pass(const VkRenderPassCreateInfo2& rp_info);
			VkDescriptorSetLayout create_descriptor_set_layout(const VkDescriptorSetLayoutCreateInfo& dsl_info);
			VkFramebuffer create_framebuffers(const VkFramebufferCreateInfo& fb_info);
			VkCommandPool create_cmd_pool(const VkCommandPoolCreateInfo& pool_info);
			VkPipeline create_graphics_pipeline(const std::vector<VkGraphicsPipelineCreateInfo>& pl_infos, const VkPipelineCache pl_cache = VK_NULL_HANDLE);
			VkPipeline create_compute_pipeline(const std::vector<VkComputePipelineCreateInfo>& pl_infos, const VkPipelineCache pl_cache = VK_NULL_HANDLE);
		};
	}
	
}

#endif // !RENDER_SYSTEM_HPP
