#ifndef DEFERRED_SHADING_HPP
#define DEFERRED_SHADING_HPP

#include "vk_gpu.hpp"
#include "stb_image.hpp"
#include "vk_buffer.hpp"
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
					stb::texture_2D albedo;
					stb::texture_2D normal;
				} model;
				struct
				{
					stb::texture_2D albedo;
					stb::texture_2D normal;
				} background;
			} textures;

			struct
			{
				glm::mat4 proj;
				glm::mat4 view;
				glm::mat4 model;
			} offscreen_VS_UBO;

			VkDescriptorSet			descriptor_set;
			VkDescriptorSetLayout	descriptor_set_layout;

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
		};
	}
}

#endif
