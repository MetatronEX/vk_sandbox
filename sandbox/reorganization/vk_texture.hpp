#ifndef VK_TEXTURE_HPP
#define VK_TEXTURE_HPP

#include "vk.hpp"
#include "vk_buffer.hpp"
#include "vk_gpu.hpp"
#include "image_info.hpp"

namespace vk
{
    template <typename image_policy>
	struct texture
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

        image_policy::image_header_ptr load_image(const char* filename)
        {
            image_policy::image_header_ptr header = image_policy::load_image_file(filename, image_info& info);
            width = info.image_width;
            height = info.image_height;
            mip_levels = info.image_mip_level;
            size = info.image_size;
            image_binary = info.image_binary;

            return header;
        }

        void destroy_image_header(image_policy::image_header_ptr header)
        {
            image_policy::destroy_image_header(header);
        }
	};
}

#endif