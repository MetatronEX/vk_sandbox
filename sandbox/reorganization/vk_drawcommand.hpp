#ifndef VK_COMMANDBUFFER_HPP
#define VK_COMMANDBUFFER_HPP

#include "vk.hpp"

namespace vk 
{
    struct draw_command
    {
        VkDevice                        device;
        std::vector<VkCommandBuffer>    commandbuffers;
        VkCommandBufferLevel            level;

        void create(VkCommandPool commandpool);
        void destroy(VkCommandPool commandpool);
    };
}

#endif