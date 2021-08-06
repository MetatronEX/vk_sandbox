#ifndef VK_RENDERTARGET_HPP
#define VK_RENDERTARGET_HPP

#include "vk.hpp"

namespace vk
{
    struct render_target
    {
        VkDescriptorImageInfo	descriptor;
        VkImage					image;
		VkImageLayout			image_layout;
		VkDeviceMemory			memory;
		VkImageView				view;
		VkSampler				sampler;
        size_t                  size;
        uint32_t				width;
		uint32_t				height;
		uint32_t				mip_levels;
		uint32_t				layer_count;
		uint8_t*                image_binary;
        GPU*					gpu;

        void update_descriptor();
		void destroy();
        void load_from_buffer(void* buffer, const VkDeviceSize size, const VkFormat format,
                const uint32_t buffer_width, const uint32_t buffer_height, VkQueue copy_queue,
                VkFilter filter = VK_FILTER_LINEAR,
                VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT,
                VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    };
}

#endif