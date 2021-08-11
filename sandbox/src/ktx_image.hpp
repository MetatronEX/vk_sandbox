#ifndef KTX_IMAGE_POLICY_HPP
#define KTX_IMAGE_POLICY_HPP

#include <ktx.h>
#include "common.hpp"
#include "vk_texture.hpp"
#include <string>

namespace vk
{
    namespace ktx
    {
        struct KTX_image_policy
        {
            using image_header_ptr = ktxTexture*;

            image_header_ptr load_image_file(const char* filename, image_info& info)
            {
                if (!file_exists(filename))
                {
                    std::string msg(filename);
                    msg += " does not exist.\n";
                    fatal_exit(msg.c_str(), -1);
                }

                ktxTexture* ktx_texture;
                ktxResult success = ktxTexture_CreateFromNamedFile(filename, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktx_texture);            
                assert(success == KTX_SUCCESS);

                info.image_width = ktx_texture->baseWidth;
                info.image_height = ktx_texture->baseHeight;
                info.image_mip_level = ktx_texture->numLevels;
                info.image_layer_count = ktx_texture->numLayers;
                info.image_size = ktx_texture->dataSize;
                info.image_binary = ktxTexture_GetData(ktx_texture);

                return ktx_texture;
            }

            void destroy_header(image_header_ptr header)
            {
                assert(header);
                ktxTexture_Destroy(header);
                header = nullptr;
            }

            size_t get_offset(image_header_ptr header, const uint32_t level)
            {
                ktx_size_t offset;
                KTX_error_code result = ktxTexture_GetImageOffset(header, level, 0, 0, &offset);
                assert(KTX_SUCCESS == result);

                return offset;
            }

        };

        using texture2D = texture_2D<KTX_image_policy>;

        // struct texture_2D_array : public texture<KTX_image_policy>
        // {
        //     void load_from_file(const char* filename, const VkFormat format, VkQueue copy_queue, 
        //         VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT,
        //         VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        // }

        // struct texture_cubemap : public texture<KTX_image_policy>
        // {
        //     void load_from_file(const char* filename, const VkFormat format, VkQueue copy_queue, 
        //         VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT,
        //         VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        // }
    }
}

#endif