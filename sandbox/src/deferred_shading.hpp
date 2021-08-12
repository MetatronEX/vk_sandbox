#ifndef DEFERRED_SHADING_HPP
#define DEFERRED_SHADING_HPP

#include "vk_gpu.hpp"
#include "stb_image.hpp"
#include "vk_buffer.hpp"
#include "vk_framebuffer.hpp"
#include "math.hpp"

namespace vk
{
	namespace policy
	{
		struct deferred_shading
		{
			GPU* gpu;

			struct
			{
				struct
				{
					stb::texture2D albedo;
					stb::texture2D normal;
				} model;
				struct
				{
					stb::texture2D albedo;
					stb::texture2D normal;
				} background;
			} textures;

			struct Light
			{
				glm::vec4 position;
				glm::vec4 target;
				glm::vec4 color;
				glm::mat4 view_mtx;
			};

			struct
			{
				glm::mat4 proj;
				glm::mat4 view;
				glm::mat4 model;
			} offscreen_VS_UBO;

			struct
			{
				glm::vec4 view_position;
				Light lights[3];
				uint32_t shadow = 1;
			} composition_UBO;

			struct
			{
				buffer	offscreen;
				buffer	composition;
				buffer	shadow_geometry;
			} uniform_buffers;

			struct
			{
				VkPipeline deferred;
				VkPipeline offscreen;
				VkPipeline shadow;
			} pipelines;

			struct
			{
				VkDescriptorSet model;
				VkDescriptorSet background;
				VkDescriptorSet shadow;
			} descriptor_sets;

			struct
			{
				framebuffer::framebuffer deferred;
				framebuffer::framebuffer shadow;
			} framebuffers;

			struct
			{
				VkCommandBuffer deferred = VK_NULL_HANDLE;
			} command_buffers;

			VkPipelineLayout		pipeline_layout;
			VkDescriptorSet			descriptor_set;
			VkDescriptorSetLayout	descriptor_set_layout;
			VkSemaphore				offscreen_semaphore;

			const float             depth_bias_constant { 1.25f };
			const float             depth_bias_slope{ 1.75f };

			// policy interface functions
			void setup_queried_features();
			void prime();
			void setup_depth_stencil();
			void setup_drawcommands();
			void setup_renderpass();
			void setup_framebuffers();
			void prepare();
			void update();
			void render();
			void cleanup();
			
			// unique
			void setup_shadow_pass();
			void setup_deferred_pass();
			void setup_deferred_commands();
			void setup_descriptor_pool();
			void setup_descriptor_layout();
			void setup_descriptor_set();
			void prepare_pipelines();
			void prepare_uniform_buffers();
			void update_uniform_buffers();

			void render_scene(VkCommandBuffer cmd_buffer, bool shadow);
		};
	}
}

#endif
