#include "vk_commandbuffer.cpp"

namespace vk
{
    void command_buffer::create(VkCommanfPool commandpool)
    {
        VkCommandBufferAllocateInfo CBA{};
        CBA.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        CBA.commandPool = commandpool;
        CBA.level = level;
        CBA.commandBufferCount = static_cast<uint32_t>(commandbuffers.size());

        OP_SUCCESS(vkAllocateCommandBuffers(device, &CBA, commandbuffers.data()));
    }

    void command_buffer::destroy(VkCommandPool commandpool)
    {
        vkFreeCommandBuffers(device, commandpool, 
            static_cast<uint32_t>(commandbuffers.size()), commandbuffers.data());
    }

    void command_buffer::set_image_layout(const uint32_t buffer_index,
                            VkImage image,
                            VkImageAspectFlags aspect_mask,
                            VkImageLayout old_layout,
                            VkImageLayout new_layout,
                            VkPipelineStageFlags src_mask,
                            VkPipelineStageFlags dst_mask)
    {
        VkImageSubresourceRange ISR{};
        ISR.aspectMask = aspect_mask;
        ISR.baseMipLevel = 0;
        ISR.levelCount = 1;
        ISR.layerCount = 1;
        set_image_layout(buffer_index, image, old_layout, new_layout, ISR, src_mask, dst_mask);
    }

    void command_buffer::set_image_layout(const uint32_t buffer_index,
                        VkImage image,
                        VkImageAspectFlags aspect_mask,
                        VkImageLayout old_layout,
                        VkImageLayout new_layout,
                        VkImageSubresourceRange& subresource_range,
                        VkPipelineStageFlags src_mask,
                        VkPipelineStageFlags dst_mask)
    {
        VkImageMemoryBarrier IMB{};
        IMB.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        IMB.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        IMB.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        IMB.oldLayout = old_layout;
        IMB.newLayout = new_layout;
        IMB.image = image;
        IMB.subresourceRange = subresource_range;

        switch (old_layout)
        {
            case VK_IMAGE_LAYOUT_UNDEFINED:
				IMB.srcAccessMask = 0;
				break;

			case VK_IMAGE_LAYOUT_PREINITIALIZED:
				IMB.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				IMB.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				IMB.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				IMB.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				IMB.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				IMB.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				break;
        }

        switch (new_layout)
        {
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				IMB.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				IMB.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				IMB.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				IMB.dstAccessMask = IMB.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				if (IMB.srcAccessMask == 0)
					IMB.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
				
				IMB.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				break;
        }

        vkCmsPipelineBarrier(commandbuffer[buffer_index], 
                                src_mask, dst_mask, 0, 0, nullptr, 0, nullptr, 1, &IMB);
    }

    void command_buffer::insertImageMemoryBarrier(const uint32_t buffer_index,
                            VkImage image,
                            VkAccessFlags src_access,
                            VkAccessFlags dst_access,
                            VkImageLayout old_layout,
                            VkImageLayout new_layout,
                            VkPipelineStageFlags src_stage,
                            VkPipelineStageFlags dst_stage,
                            VkImageSubresourceRange subresource_range)
    {
        VkImageMemoryBarrier IMB{};
        IMB.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        IMB.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        IMB.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        IMB.srcAccessMask = src_access;
        IMB.dstAccessMask = dst_access;
        IMB.oldLayout = old_layout;
        IMB.newLayout = new_layout;
        IMB.image = image;
        IMB.subresourceRange = subresource_range;

        vkCmdPipelineBarrier(commandbuffers[buffer_index], src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &IMB);
    }
}