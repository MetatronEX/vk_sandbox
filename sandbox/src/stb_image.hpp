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

			size_t get_offset(image_header_ptr header, const uint32_t level)
			{
				return 0;
			}
		};

		using texture2D = texture_2D<STB_image_policy>;
	}
}

#endif