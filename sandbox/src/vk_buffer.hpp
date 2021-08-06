#ifndef VK_BUFFER_HPP
#define VK_BUFFER_HPP

#include "vk.hpp"

namespace vk
{
    struct buffer
    {
        VkBuffer                buffer;
        VkDeviceSize            size {0};
        VkDeviceSize            alignment {0};
        VkDevice                device;        
        VkDeviceMemory          memory;
        VkDescriptorBufferInfo  descriptor;
        VkBufferUsageFlags      usage;
        VkMemoryPropertyFlags   memory_property;
        void*                   mapped {nullptr};

        VkResult    map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void        unmap();
        VkResult    bind(VkDeviceSize offset = 0);
        void        setup_descriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void        copy_to_device(void* data, VkDeviceSize size);
        VkResult    flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        VkResult    invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    };
}

#endif