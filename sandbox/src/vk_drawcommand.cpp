#include "vk_drawcommand.hpp"

namespace vk
{
    void draw_command::allocate(VkDevice device, VkCommandPool commandpool, const VkCommandBufferLevel level)
    {
        auto CBA = info::command_buffer_allocate_info(commandpool, level, static_cast<uint32_t>(commandbuffers.size()));
        OP_SUCCESS(vkAllocateCommandBuffers(device, &CBA, commandbuffers.data()));
    }

    void draw_command::free(VkDevice device, VkCommandPool commandpool)
    {
        vkFreeCommandBuffers(device, commandpool, 
            static_cast<uint32_t>(commandbuffers.size()), commandbuffers.data());
    }
}