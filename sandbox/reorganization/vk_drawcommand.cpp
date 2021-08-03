#include "vk_commandbuffer.cpp"

namespace vk
{
    void draw_command::create(VkCommanfPool commandpool)
    {
        auto CBA = info::command_buffer_allocate_info(commandpool, level, static_cast<uint32_t>(commandbuffers.size()));
        OP_SUCCESS(vkAllocateCommandBuffers(device, &CBA, commandbuffers.data()));
    }

    void draw_command::destroy(VkCommandPool commandpool)
    {
        vkFreeCommandBuffers(device, commandpool, 
            static_cast<uint32_t>(commandbuffers.size()), commandbuffers.data());
    }
}