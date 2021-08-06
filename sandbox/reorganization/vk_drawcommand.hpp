#ifndef VK_COMMANDBUFFER_HPP
#define VK_COMMANDBUFFER_HPP

#include "vk.hpp"

namespace vk 
{
    struct draw_command
    {
        std::vector<VkCommandBuffer>    commandbuffers;
        
        void allocate(VkDevice device, VkCommandPool commandpool, const VkCommandBufferLevel level);
        void free(VkDevice device, VkCommandPool commandpool);
    };
}

#endif