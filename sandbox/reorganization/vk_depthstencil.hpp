#ifndef VK_DEPTHSTENCIL_HPP
#define VK_DEPTHSTENCIL_HPP

#include "vk.hpp"

namespace vk
{
    struct depth_stencil
    {
        VkImage         image;
        VkDeviceMemory  memory;
        VkImageView     view;
    };
}

#endif