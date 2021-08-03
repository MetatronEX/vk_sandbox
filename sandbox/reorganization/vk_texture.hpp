#ifndef VK_TEXTURE_HPP
#define VK_TEXTURE_HPP

#include "vk.hpp"
#include "vk_buffer.hpp"
#include "vk_gpu.hpp"

namespace vk
{
	struct texture
	{
		GPU*					gpu;
		VkImage					image;
		VkImageLayout			image_layout;
		VkDeviceMemory			memory;
		VkImageView				view;
		uint32_t				width;
		uint32_t				height;
		uint32_t				mip_levels;
		uint32_t				layer_count;
		VkDescriptorImageInfo	descriptor;
		VkSampler				sampler;

		void update_descriptor();
		void destroy();

	};
}

#endif