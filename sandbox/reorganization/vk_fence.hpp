#ifndef VK_FENCE_HPP
#define VK_FENCE_HPP

#include "vk.hpp"

namespace vk
{
    struct fence
    {
        VkFence                     fence;
        VkFenceCreateFlagBits       flags;
    };
}

#endif