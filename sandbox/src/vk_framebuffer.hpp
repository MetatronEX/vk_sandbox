#ifndef VK_FRAMEBUFFER_HPP
#define VK_FRAMEBUFFER_HPP

#include "vk.hpp"
#include "vk_gpu.hpp"

namespace vk
{
    namespace framebuffer
    {
        struct framebuffer_attachment
        {
            VkImage                     image;
            VkDeviceMemory              memory;
            VkImageView                 view;
            VkFormat                    format;
            VkImageSubresourceRange     subresource_range;
            VkAttachmentDescription2    description;

            bool has_depth()
            {
                std::vector<VkFormat> formats =
                {
                    VK_FORMAT_D16_UNORM,
                    VK_FORMAT_X8_D24_UNORM_PACK32,
                    VK_FORMAT_D32_SFLOAT,
                    VK_FORMAT_D16_UNORM_S8_UINT,
                    VK_FORMAT_D24_UNORM_S8_UINT,
                    VK_FORMAT_D32_SFLOAT_S8_UINT,
                };
                return std::find(formats.begin(), formats.end(), format) != formats.end();
            }

            bool has_stencil()
            {
                std::vector<VkFormat> formats =
                {
                    VK_FORMAT_S8_UINT,
                    VK_FORMAT_D16_UNORM_S8_UINT,
                    VK_FORMAT_D24_UNORM_S8_UINT,
                    VK_FORMAT_D32_SFLOAT_S8_UINT,
                };
                return std::find(formats.begin(), formats.end(), format) != formats.end();
            }

            bool is_depth_stencil()
            {
                return has_depth() || has_stencil();
            }
        };

        struct attachment_create_info
        {
            uint32_t                width;
            uint32_t                height;
            uint32_t                layer_count;
            VkFormat                format;
            VkImageUsageFlags       usage;
            VkSampleCountFlagBits   image_sample_count = VK_SAMPLE_COUNT_1_BIT;
        };

        struct framebuffer
        {
            GPU* gpu;
            uint32_t                width;
            uint32_t                height;
            VkFramebuffer           framebuffer;
            VkRenderPass            renderpass;
            VkSampler               sampler;

            std::vector<framebuffer_attachment> attachments;

            void        destroy();
            uint32_t    add_attachment(attachment_create_info& create_info);
            void        create_sampler(const VkFilter mag_filter, const VkFilter min_filter, VkSamplerAddressMode address_mode);
            void        create_renderpass();
        };
    }
}

#endif