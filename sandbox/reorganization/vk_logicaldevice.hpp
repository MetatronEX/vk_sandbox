#ifndef VK_LOGICALDEVICE_HPP
#define VK_LOGICALDEVICE_HPP

#include "vk.hpp"

namespace vk
{
    struct logical_device 
    {
        VkPhysicalDevice    physical_device;
        VkDevice            device;

        vkCommandPool       commandpool;

        void create(VkPhysicalDevice _pd);
        VkCommandPool create_commandpool(const uint32_t queue_familiy_index, const VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    };  
}

#endif