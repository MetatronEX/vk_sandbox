#include "vk_buffer.hpp"

namespace vk
{
    VkResult buffer::map(VkDeviceSize size, VkDeviceSize offset)
    {
        return vkMapMemory(device, memory, offset, size, &mapped);
    }

    void buffer::unmap()
    {
        if(mapped)
        {
            vkUnmapMemory(device, memory);
            mapped = nullptr;
        }
    }

    VkResult buffer::bind(VkDeviceSize offset)
    {
        return vkBindBufferMemory(device, buffer, memory, offset);
    }

    void buffer::setup_descriptor(VkDeviceSize size, VkDeviceSize offset)
    {
        descriptor.offset = offset;
        descriptor.buffer = buffer;
        descriptor.range = size;
    }

    void buffer::copy_to_device(void* data, VkDeviceSize size)
    {
        assert(mapped);
        memcpy(mapped, data, size);
    }

    VkResult buffer::flush(VkDeviceSize size, VkDeviceSize offset)
    {
        auto MMR = info::mapped_memory_range();
        MMR.memory = memory;
        MMR.offset = offset;
        MMR.size = size;
        return vkFlushMappedMemoryRanges(device, 1, &MMR);
    }

    VkResult buffer::invalidate(VkDeviceSize size, VkDeviceSize offset)
    {
        auto MMR = info::mapped_memory_range();
        MMR.memory = memory;
        MMR.offset = offset;
        MMR.size = size;
        return vkInvalidateMappedMemoryRanges(device, 1, &MMR);
    }
}