#ifndef STB_IMAGE_HPP
#define STB_IMAGE_HPP

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "common.hpp"
#include "vk_texture.hpp"
#include <string>

namespace vk
{
	namespace stb
	{
		struct STB_image_policy
		{
			using  image_header_ptr = stbi_uc*;

			image_header_ptr load_image_file(const char* filename, image_info& info)
			{
				if (!file_exists(filename))
				{
					std::string msg(filename);
					msg += " does not exist.\n";
					fatal_exit(msg.c_str(), -1);
				}

				int width, height, channel;
				stbi_uc* pixels = stbi_load(filename, &width, &height, &channel, STBI_rgb_alpha);
				
				info.image_width = width;
				info.image_height = height;
				info.image_size = width * height * 4;
				info.image_layer_count = 1;
				info.image_mip_level = 1;
				info.image_binary = pixels;

				return pixels;
			}

			void destroy_header(image_header_ptr header)
			{
				assert(header);
				stbi_image_free(header);
				header = nullptr;
			}
		};

		using STB_texture = texture<STB_image_policy>;

		struct texture_2D : private STB_texture
		{
			image_header_ptr load_image(const char* filename)
			{
				image_info info;

				image_header_ptr header = load_image_file(filename, info);
				width = info.image_width;
				height = info.image_height;
				mip_levels = info.image_mip_level;
				size = info.image_size;
				image_binary = info.image_binary;

				return header;
			}

			void destroy_image_header(image_header_ptr header)
			{
				destroy_header(header);
			}

			void load_from_file(const char* filename, const VkFormat format, VkQueue copy_queue,
				VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT,
				VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				bool force_linear = false);
		};
	}
}

#endif