#include "ktx_image.hpp"
#undef max

namespace vk
{
    namespace ktx
    {
        void texture_2D::load_from_file(const char* filename, const VkFormat format, VkQueue copy_queue, 
                VkImageUsageFlags usage, VkImageLayout layout,bool force_linear)
        {
            KTX_image_policy::image_header_ptr header;
            image_info info;
            header = load_image_file(filename,info);

            size = info.image_size;
            width = info.image_width;
            height = info.image_height;
            mip_levels = info.image_mip_level;
            layer_count = info.image_layer_count;
            image_binary = info.image_binary;

            bool use_staging = !force_linear;

            VkCommandBuffer copy_cmd = gpu->create_commandbuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

            if(use_staging)
            {
                buffer staging_buffer;
                staging_buffer.device = gpu->device;

                OP_SUCCESS(gpu->create_buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    &staging_buffer, size, static_cast<void*>(image_binary)));

                std::vector<VkBufferImageCopy> BCRs;

                for (uint32_t i = 0; i < mip_levels; i++)
                {
                    auto offset = query_offset(header, i);

                    VkBufferImageCopy BCR{};
                    BCR.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    BCR.imageSubresource.mipLevel = i;
                    BCR.imageSubresource.baseArrayLayer = 0;
                    BCR.imageSubresource.layerCount = 1;
                    BCR.imageExtent.width = std::max(1u, width >> i);
                    BCR.imageExtent.height = std::max(1u, height >> i);
                    BCR.imageExtent.depth = 1;
                    BCR.bufferOffset = offset;

                    BCRs.push_back(BCR);
                }

                auto IC = info::image_create_info();
                IC.imageType = VK_IMAGE_TYPE_2D;
                IC.format = format;
                IC.mipLevels = mip_levels;
                IC.arrayLayers = 1;
                IC.samples = VK_SAMPLE_COUNT_1_BIT;
                IC.tiling = VK_IMAGE_TILING_OPTIMAL;
                IC.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                IC.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                IC.extent = { width, height, 1 };
                IC.usage = usage;

                if(!(VK_IMAGE_USAGE_TRANSFER_DST_BIT & IC.usage))
                    IC.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                OP_SUCCESS(vkCreateImage(gpu->device, &IC, gpu->allocation_callbacks, &image));

                VkMemoryRequirements2 MR{};
			    MR.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
                VkImageMemoryRequirementsInfo2 IMR{};
			    IMR.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
                IMR.image = image;
                vkGetImageMemoryRequirements2(gpu->device, &IMR, &MR);

                auto MA = info::memory_allocate_info();
                MA.allocationSize = MR.memoryRequirements.size;
                MA.memoryTypeIndex = gpu->query_memory_type(MR.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                OP_SUCCESS(vkAllocateMemory(gpu->device, &MA, gpu->allocation_callbacks, &memory));
                OP_SUCCESS(vkBindImageMemory(gpu->device, image, memory, 0));

                VkImageSubresourceRange ISR{};
                ISR.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                ISR.baseMipLevel = 0;
                ISR.levelCount = mip_levels;
                ISR.layerCount = 1;

                command::set_image_layout(copy_cmd, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,ISR);
                vkCmdCopyBufferToImage(copy_cmd, staging_buffer.buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                    static_cast<uint32_t>(BCRs.size()), BCRs.data());
                image_layout = layout;
                command::set_image_layout(copy_cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, image_layout, ISR);

                gpu->flush_command_buffer(copy_cmd, copy_queue);
                gpu->destroy_buffer(staging_buffer);
            }
            else
            {
                auto FP = query_format_properties(gpu->physical_device,format);
                assert(FP.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);

                VkImage         mappable_image;
                VkDeviceMemory  mappable_memory;

                auto IC = info::image_create_info();
                IC.imageType = VK_IMAGE_TYPE_2D;
                IC.format = format;
                IC.extent = { width, height, 1 };
                IC.mipLevels = 1;
                IC.arrayLayers = 1;
                IC.samples = VK_SAMPLE_COUNT_1_BIT;
                IC.tiling = VK_IMAGE_TILING_LINEAR;
                IC.usage = usage;
                IC.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                IC.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

                OP_SUCCESS(vkCreateImage(gpu->device, &IC, gpu->allocation_callbacks, &mappable_image));

                VkMemoryRequirements2 MR{};
			    MR.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
                VkImageMemoryRequirementsInfo2 IMR{};
			    IMR.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
                IMR.image = mappable_image;
                vkGetImageMemoryRequirements2(gpu->device, &IMR, &MR);

                auto MA = info::memory_allocate_info();
                MA.allocationSize = MR.memoryRequirements.size;
                MA.memoryTypeIndex = gpu->query_memory_type(MR.memoryRequirements.memoryTypeBits, 
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                
                OP_SUCCESS(vkAllocateMemory(gpu->device, &MA, gpu->allocation_callbacks, &mappable_memory));
                OP_SUCCESS(vkBindImageMemory(gpu->device, mappable_image, mappable_memory, 0));

                VkImageSubresource IS{};
                IS.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                IS.mipLevel = 0; 

                VkSubresourceLayout  SL{};
                void* data;

                vkGetImageSubresourceLayout(gpu->device, mappable_image, &IS, &SL);

                OP_SUCCESS(vkMapMemory(gpu->device, mappable_memory, 0, MR.memoryRequirements.size, 0, &data));
                memcpy(data, image_binary, MR.memoryRequirements.size);
                vkUnmapMemory(gpu->device, mappable_memory);

                image = mappable_image;
                memory = mappable_memory;
                image_layout = layout;

                command::set_image_layout(copy_cmd, image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, image_layout);
                gpu->flush_command_buffer(copy_cmd, copy_queue);
            }

            destroy_image_header(header);

            auto SC = info::sampler_create_info();
            SC.magFilter = VK_FILTER_LINEAR;
            SC.minFilter = VK_FILTER_LINEAR;
            SC.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            SC.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            SC.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            SC.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            SC.mipLodBias = 0.f;
            SC.compareOp = VK_COMPARE_OP_NEVER;
            SC.minLod = 0.f;
            SC.maxLod = (use_staging) ? static_cast<float>(mip_levels) : 0.f;
            SC.maxAnisotropy = gpu->enabled_features.features.samplerAnisotropy ? gpu->device_properties.properties.limits.maxSamplerAnisotropy : 1.f;
            SC.anisotropyEnable = gpu->enabled_features.features.samplerAnisotropy;
            SC.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            OP_SUCCESS(vkCreateSampler(gpu->device, &SC, gpu->allocation_callbacks, &sampler));

            auto IVC = info::image_view_create_info();
            IVC.viewType = VK_IMAGE_VIEW_TYPE_2D;
            IVC.format = format;
            IVC.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
            IVC.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
            IVC.subresourceRange.levelCount = (use_staging) ? mip_levels : 1;
            IVC.image = image;
            OP_SUCCESS(vkCreateImageView(gpu->device, &IVC, gpu->allocation_callbacks, &view));

            update_descriptor();
        }
    }
}