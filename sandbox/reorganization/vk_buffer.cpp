#include "vk_buffer.hpp"

namespace vk
{
    VkResult map(VkDeviceSize size, VkDeviceSize offset)
    {
        return vkmapMemory(device, memory, offset, size, &mapped);
    }

    void unmap()
    {
        if(mapped)
        {
            vkUnmapMemory(device, memory);
            mapped = nullptr;
        }
    }

    VkResult bind(VkDeviceSize offset)
    {
        return vkBindBufferMemory(device, buffer, memory, offset);
    }

    void setup_descriptor(VkDeviceSize size, VkDeviceSize offset)
    {
        descriptor.offset = offset;
        descriptor.buffer = buffer;
        descriptor.range = size;
    }

    void copy_to(void* data, VkDeviceSize size)
    {
        assert(mapped);
        memcpy(mapped, data, size);
    }

    VkResult flush(vkDeviceSize size, VkDeviceSize offset)
    {
        auto MMR = info::mapped_memory_range();
        MMR.memory = memory;
        MMR.offset = offset;
        MMR.size = size;
        return vkFlushMappedMemoryRanges(device, 1, &MMR);
    }

    VkResult invalidate(vkDeviceSize size, VkDeviceSize offset)
    {
        auto MMR = info::mapped_memory_range();
        MMR.memory = memory;
        MMR.offset = offset;
        MMR.size = size;
        return vkInvalidateMappedMemoryRanges(device, 1, &MMR);
    }
}