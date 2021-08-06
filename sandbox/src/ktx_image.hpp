#ifndef KTX_IMAGE_POLICY_HPP
#define KTX_IMAGE_POLICY_HPP

#include <ktx.h>
#include "common.hpp"
#include "vk_texture.hpp"

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
                    fatal_exit("Texture file: " + filename + " does not exist.\n",-1);

                ktxTexture* ktx_texture;
                ktxResult success = ktxTexture_CreateFromNamedFile(filename, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, ktx_texture);            
                assert(success == KTX_SUCCESS);

                info.image_width = ktx_texture->baseWidth;
                info.image_height = ktx_texture->baseHeight;
                info.image_mip_level = ktx_texture->numLevels;
                info.image_layer_count = ktx_texture->numLayers;
                info.image_size = ktxTexture_GetSize(ktx_texture);
                info.image_binary = ktxTexture_GetData(ktx_texture);

                return ktx_texture;
            }

            void destroy_image_header(image_header_ptr header)
            {
                assert(header);
                ktxTexture_Destroy(header);
                header = nullptr;
            }
        };

        struct texture_2D : public texture<KTX_image_policy>
        {
            void load_from_file(const char* filename, const VkFormat format, VkQueue copy_queue, 
                VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT,
                VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                bool force_linear = false); 
        };

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