#ifndef VK_COMMANDBUFFER_HPP
#define VK_COMMANDBUFFER_HPP

#include "vk.hpp"

namespace vk 
{
    struct command_buffer
    {
        VkDevice                        device;
        std::vector<VkCommandBuffer>    commandbuffers;
        VkCommandBufferLevel            level;

        void create(VkCommandPool commandpool);
        void destroy(VkCommandPool commandpool);

        void set_image_layout(const uint32_t buffer_index,
                            VkImage image,
                            VkImageAspectFlags aspect_mask,
                            VkImageLayout old_layout,
                            VkImageLayout new_layout,
                            VkPipelineStageFlags src_mask,
                            VkPipelineStageFlags dst_mask);

        void set_image_layout(const uint32_t buffer_index,
                            VkImage image,
                            VkImageAspectFlags aspect_mask,
                            VkImageLayout old_layout,
                            VkImageLayout new_layout,
                            VkImageSubresourceRange subresource_range,
                            VkPipelineStageFlags src_mask,
                            VkPipelineStageFlags dst_mask);
        
        void insertImageMemoryBarrier(const uint32_t buffer_index,
                            VkImage image,
                            VkAccessFlags src_access,
                            VkAccessFlags dst_access,
                            VkImageLayout old_layout,
                            VkImageLayout new_layout,
                            VkPipelineStageFlags src_stage,
                            VkPipelineStageFlags dst_stage,
                            VkImageSubresourceRange subresource_range);
    };
}

#endif