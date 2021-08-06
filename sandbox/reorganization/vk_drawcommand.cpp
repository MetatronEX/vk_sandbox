#include "vk_drawcommand.cpp"

namespace vk
{
    void draw_command::allocate(VkCommanfPool commandpool, const VkCommandBufferLevel level)
    {
        auto CBA = info::command_buffer_allocate_info(commandpool, level, static_cast<uint32_t>(commandbuffers.size()));
        OP_SUCCESS(vkAllocateCommandBuffers(device, &CBA, commandbuffers.data()));
    }

    void draw_command::free(VkCommandPool commandpool)
    {
        vkFreeCommandBuffers(device, commandpool, 
            static_cast<uint32_t>(commandbuffers.size()), commandbuffers.data());
    }
}