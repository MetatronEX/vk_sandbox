#include "vk_rendertarget.hpp"
#include "vk_buffer.hpp"

namespace vk
{
    void render_target::update_descriptor()
    {
        descriptor.sampler = sampler;
        descriptor.imageView = view;
        descriptor.imageLayout = image_layout;
    }

    void render_target::destroy()
    {
        vkDestroyImageView(gpu->device, view, gpu->allocation_callbacks);
        vkDestroyImage(gpu->device, image, gpu->allocation_callbacks);
        if (sampler)
            vkDestroySampler(gpu->device, sampler, gpu->allocation_callbacks);
        vkFreeMemory(gpu->device, memory, gpu->allocation_callbacks);
    }

    void render_target::load_from_buffer(void* buffer, const VkDeviceSize size, const VkFormat format,
            const uint32_t buffer_width, const uint32_t buffer_height, VkQueue copy_queue,
            VkFilter filter = VK_FILTER_LINEAR,
            VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT,
            VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        width = buffer_width;
        height = buffer_height;
        mip_levels = 1;

        auto MA = info::memory_allocate_info();
        VkMemoryRequirements2 MR{};
        MR.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
        VkImageMemoryRequirementsInfo2 IMR{};
        IMR.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
        
        VkComandBuffer copy_cmd = gpu->create_commandbuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        buffer staging_buffer;
        
        gpu->create_buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            size, &staging_buffer, buffer) ;

        VkBufferImageCopy BCR{};
		BCR.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		BCR.imageSubresource.mipLevel = 0;
		BCR.imageSubresource.baseArrayLayer = 0;
		BCR.imageSubresource.layerCount = 1;
		BCR.imageExtent.width = width;
		BCR.imageExtent.height = height;
		BCR.imageExtent.depth = 1;
		BCR.bufferOffset = 0;

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

        if (!(VK_IMAGE_USAGE_TRANSFER_DST_BIT & IC.usage))
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
        MA.memoryTypeIndex = gpu->query_memory_type(MR.memoryRequirements.memoryTypeBit, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        OP_SUCCESS(vkAllocateMemory(gpu->device, &MA, gpu->allocation_callbacks, &memory));
        OP_SUCCESS(vkBindImageMemory(gpu->device, image, memory, 0));

        VkImageSubresourceRange ISR{};
        ISR.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ISR.baseMipLevel = 0;
        ISR.levelCount = mip_levels;
        ISR.layerCount = 1;

        command::set_image_layout(copy_cmd, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,ISR);
        vkCmdCopyBufferToImage(copy_cmd, staging_buffer.buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &BCRs);
        image_layout = layout;
        command::set_image_layout(copy_cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, image_layout, ISR);

        gpu->flush_command_buffer(copy_cmd, copy_queue);
        gpu->destroy_buffer(staging_buffer);

        auto SC = info::sampler_create_info();
        SC.magFilter = filter;
        SC.minFilter = filter;
        SC.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        SC.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        SC.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        SC.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        SC.mipLodBias = 0.f;
        SC.compareOp = VK_COMPARE_OP_NEVER;
        SC.minLod = 0.f;
        SC.maxLod = 0.f;
        SC.maxAnisotropy = 1.f;
        OP_SUCCESS(vkCreaetSampler(gpu->device, &SC, gpu->allocation_callbacks, &sampler));

        auto IVC = info::image_view_create_info();
        IVC.viewType = VK_IMAGE_VIEW_TYPE_2D;
        IVC.format = format;
        IVC.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
        IVC.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        IVC.subresourceRange.levelCount = 1;
        IVC.image = image;
        OP_SUCCESS(vkCreateImageView(gpu->device, &IVC, gpu->allocation_callbacks, &view));

        update_descriptor();
    }
}