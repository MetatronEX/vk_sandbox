#ifndef VK_TEXTURE_HPP
#define VK_TEXTURE_HPP

#include "vk.hpp"
#include "vk_buffer.hpp"
#include "vk_gpu.hpp"
#include "image_info.hpp"

namespace vk
{
    template <typename image_policy>
	struct texture : private image_policy
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

        using image_policy::image_header_ptr;
        using image_policy::load_image_file;
        using image_policy::destroy_header;

		void update_descriptor()
        {
            descriptor.sampler = sampler;
            descriptor.imageView = view;
            descriptor.imageLayout = image_layout;
        }

		void destroy()
        {
            vkDestroyImageView(gpu->device, view, gpu->allocation_callbacks);
            vkDestroyImage(gpu->device, image, gpu->allocation_callbacks);
            if (sampler)
                vkDestroySampler(gpu->device, sampler, gpu->allocation_callbacks);
            vkFreeMemory(gpu->device, memory, gpu->allocation_callbacks);
        }

        
	};
}

#endif