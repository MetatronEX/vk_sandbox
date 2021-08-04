#ifndef VK_FRAMEBUFFER_HPP
#define VK_FRAMEBUFFER_HPP

#include "vk.hpp"

namespace vk
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
        VkimageUsageFlags       usage;
        VkSampleCountFlagBits   image_sample_count = VK_SAMPLE_COUNT_1_BIT;
    };

    struct framebuffer_attachment
    {
        GPU*                    gpu;
        uint32_t                width;
        uint32_t                height;
        VkFramebuffer           framebuffer;
        VkRenderPass            renderpass;
        vkSampler               sampler;

        std::vector<framebuffer_attachment> attachments;

        void destroy()
        {
            for (auto a : attachments)
            {
                vkDestroyImage(gpu->device, a.image, gpu->allocation_callbacks);
                vkDestroyImageView(gpu->device, a.view, gpu->allocation_callbacks);
                vkFreeMemory(gpu->device, a.memory, gpu->allocation_callbacks);
            }

            vkDestroySampler(gpu->device, sampler, gpu->allocation_callbacks);
            vkDestroyRenderPass(gpu->device, renderpass, gpu->allocation_callbacks);
            vkDestroyFramebuffer(gpu->device, framebuffer, gpu->allocation_callbacks);
        }

        uint32_t add_attachment(attachment_create_info& create_info)
        {
            framebuffer_attachment FA;
            FA.format = create_info.format;

            VkImageAspectFlags aspect_mask = VK_FLAGS_NONE;

            if (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT & create_info.usage)
                aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
            
            if (VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT & create_info.usage)
            {
                if (FA.has_depth())
                    aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
                
                if (FA.has_stencil())
                    aspect_mask = aspect_mask | VK_IMAGE_ASPECT_STENCIL_BIT;
            }

            assert(aspect_mask > 0);

            auto IC = info::image_create_info();
            IC.imageType = VK_IMAGE_TYPE_2D;
            IC.format = create_info.format;
            IC.extent.width = create_info.width;
            IC.extent.height = create_info.height;
            IC.extent.depth = 1;
            IC.mipLevels = 1;
            IC.arrayLayers = create_info.layer_count;
            IC.samples = create_info.image_sample_count;
            IC.tiling = VK_IMAGE_TILING_OPTIMAL;
            IC.usage = create_info.usage;
            OP_SUCCESS(vkCreateImage(gpu->device, &IC, gpu->allocation_callbacks, &FA.image));

            VkMemoryRequirements2 MR{};
            MR.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
            VkImageMemoryRequirementsInfo2 IMR{};
			IMR.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
			IMR.image = FA.image;
            vkGetImageMemoryRequirements2(gpu->device, &IMR, &MR);

            auto MA = info::memory_allocate_info();
            MA.allocationSize = MR.size;
            MA.memoryTypeIndex = gpu->query_memory_type(MR.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            OP_SUCCESS(vkAllocateMemory(gpu->device, &MA, gpu->allocation_callbacks, &FA.memory));
            OP_SUCCESS(vkBindImageMemory(gpu->device, FA.image, FA.memory, 0));

            FA.subresource_range = {};
            FA.subresource_range.aspectMask = aspect_mask;
            FA.subresource_range.levelCount = 1;
            FA.subresource_range.layerCount = create_info.layer_count;

            auto IVC = info::image_view_create_info();
            IVC.viewType = (1 == create_info.layer_count) ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            IVC.format = create_info.format;
            IVC.subresourceRange = FA.subresource_range;
            IVC.subresourceRange.aspectMask = (FA.has_depth()) ? VK_IMAGE_ASPECT_DEPTH_BIT : aspect_mask;
            IVC.image = FA.image;
            OP_SUCCESS(vkCreateImageView(gpu->device, &IVC, gpu->allocation_callbacks, &FA.view));

            FA.description = {};
            FA.description.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
            FA.description.samples = create_info.image_sample_count;
            FA.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            FA.description.storeOp = (VK_IMAGE_USAGE_SAMPLED_BIT & create_info.usage) ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
            FA.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CONT_CARE;
            FA.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            FA.description.format = create_info.format;
            FA.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            FA.description.finalLayout = (FA.has_depth() || FA.has_stencil()) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            attachments.push_back(FA);

            return static_cast<uint32_t>(attachments.size() - 1);
        }

        void create_sampler(const VkFilter mag_filter, cosnt VkFilter min_filter, VkSamplerAddressMode address_mode)
        {
            auto SC = info::sampler_create_info();
            SC.magFilter = mag_filter;
            SC.minFilter = min_filter;
            SC.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            SC.addressModeU = address_mode;
            SC.addressModeV = address_mode;
            SC.addressModeW = address_mode;
            SC.mipLodBias = 0.f;
            SC.maxAnisotropy = 1.f;
            SC.minLod = 0.f;
            SC.maxLod = 1.f;
            SC.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

            OP_SUCCESS(vkCreateSampler(gpu->device, &SC, gpu->allocation_callbacks, &sampler));
        }

        void create_renderpass()
        {
            std::vector<VkAttachmentDescription2> ADs;
            for (const auto& a : attachments)
                ADs.push_back(a.description);
            
            std::vector<VkAttachmentReference2> CRs;
            VkAttachmentReference2 DR{};
            bool has_depth = false;
            bool has_color = false;

            uint32_t index = 0;

            for (const auto& a : attachments)
            {
                if (a.is_depth_stencil())
                {
                    assert(!has_depth);
                    DR.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
                    DR.attachment = index;
                    DR.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                    has_depth = true;
                }
                else
                {
                    VkAttachmentDescription2 CR{};
                    CR.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
                    CR.aspectMask = VK_FLAGS_NONE;
                    CR.attachment = index;
                    CR.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    CRs.push_back(CR);
                    has_color = true;
                }

                index++;
            }

            VkSubpassDescription2 SP{};
            SP.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
            SP.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            if (has_color)
            {
                SP.pColorAttachments = CRs.data();
                SP.colorAttachmentCount = static_cast<uint32_t>(CRs.size());
            }
            if (has_depth)
                SP.pDepthStencilAttachment = &DR;

            std::array<VkSubpassDependency2,2> SPDs;

            SPDs[0].sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
            SPDs[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            SPDs[0].dstSubpass = 0;
            SPDs[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            SPDs[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            SPDs[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            SPDs[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            SPDs[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            SPDs[1].sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
            SPDs[1].srcSubpass = 0;
            SPDs[1].dstSubpass = VK_SUBPASS_EXTERNAL;
            SPDs[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            SPDs[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            SPDs[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            SPDs[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            SPDs[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            auto RP = info::renderpass_create_info();
            RP.pAttachments = ADs.data();
            RP.attachmentCount = static_cast<uint32_t>(ADs.size());
            RP.subpassCount = 1;
            RP.pSubpasses = &SP;
            RP.dependencyCount = 2;
            RP.pDependencies = SPDs.data();
            OP_SUCCESS(vkCreateRenderPass(gpu->device, &RP, gpu->allocation_callbacks, &renderpass));

            std::vector<VkImageView>    AVs;
            for (auto a : attachments)
                AVs.push_back(a.view);

            uint32_t max_layers = 0;
            
            for (const auto a : attachments)
                if (a.subresource_range.layerCount > max_layers)
                    max_layers = a.subresource_range.layerCount;
            
            auto FBC = info::framebuffer_create_info();
            FBC.renderPass = renderpass;
            FBC.pAttachments = AVs.data();
            FBC.attachmentCount = static_cast<uint32_t>(AVs.size());
            FBC.width = width;
            FBC.height = height;
            FBC.layers = max_layers;
            OP_SUCCESS(vkCreateFramebuffer(gpu->device, &FBC, gpu->allocation_callbacks, &framebuffer));
        }
    };
}

#endif