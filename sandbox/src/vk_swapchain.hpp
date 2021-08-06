#ifndef VK_SWAPCHAIN_HPP
#define VK_SWAPCHAIN_HPP

#include "vk.hpp"

#include <vector>
#include <limits>
#include <tuple>

namespace vk
{
    struct swap_chain
    {
        struct swap_chain_buffer
        {
            VkImage         image;
            VkImageView     view;
        };

        VkInstance                          instance;
        VkPhysicalDevice                    physical_device;
        VkDevice                            device;
        VkAllocationCallbacks*              allocation_callbacks;
        VkSurfaceKHR                        surface;
        VkSwapchainKHR                      swapchain   {VK_NULL_HANDLE};
        
        VkFormat                            color_format;
        VkColorSpaceKHR                     color_space;

        uint32_t                            image_count;
        uint32_t                            queue_node_index;

        std::vector<VkImage>                images;
        std::vector<swap_chain_buffer>      buffers;

        void                                create(uint32_t& _width, uint32_t& _height, const bool vsync);
        std::tuple<VkResult,uint32_t>       acquire_next_image(VkSemaphore present_complete);
        VkResult                            queue_present(VkQueue queue, const uint32_t image_index, VkSemaphore wait);
        void                                cleanup();
    };
}

#endif