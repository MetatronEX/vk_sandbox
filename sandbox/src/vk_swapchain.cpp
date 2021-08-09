#include "win32.hpp"
#include "vk_swapchain.hpp"

#undef max

namespace vk
{
    void swap_chain::create(uint32_t& _width, uint32_t& _height, const bool vsync)
    {
        VkSwapchainKHR old = swapchain;

        auto surf_caps = query_surface_capabilities(physical_device,surface);
        auto surface_formats = query_surface_formats(physical_device, surface);
        auto present_modes = query_surface_present_modes(physical_device, surface);

        if((1 == surface_formats.size()) && (VK_FORMAT_UNDEFINED == surface_formats[0].format))
        {
            color_format = VK_FORMAT_B8G8R8A8_UNORM;
            color_space  = surface_formats[0].colorSpace;
        }
        else
        {
            bool found = false;
            for(auto&& sf : surface_formats)
            {
                if (VK_FORMAT_B8G8R8A8_UNORM == sf.format)
                {
                    color_format = sf.format;
                    color_space  = sf.colorSpace;
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                color_format = surface_formats[0].format;
                color_space = surface_formats[0].colorSpace;
            }
        }

        VkExtent2D sc_extent {};

        constexpr auto infinity = std::numeric_limits<uint32_t>::max();

        if (surf_caps.currentExtent.width == infinity)
        {
            sc_extent.width = _width;
            sc_extent.height = _height;
        }
        else
        {
            sc_extent = surf_caps.currentExtent;
            _width = sc_extent.width;
            _height = sc_extent.height;
        }

        VkPresentModeKHR scpm = VK_PRESENT_MODE_FIFO_KHR;

        if (!vsync)
        {
            for (auto p : present_modes)
            {
                if(p == VK_PRESENT_MODE_MAILBOX_KHR)
                {
                    scpm = VK_PRESENT_MODE_MAILBOX_KHR;
                    break;
                }
            }
        }

        uint32_t sc_image_count = surf_caps.minImageCount + 1;
        if((surf_caps.maxImageCount > 0) && (sc_image_count > surf_caps.maxImageCount))
        {
            sc_image_count = surf_caps.maxImageCount;
        }
        
        VkSurfaceTransformFlagsKHR pre_transform = surf_caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR ?
                            VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : surf_caps.currentTransform;
        
        VkCompositeAlphaFlagBitsKHR composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        std::vector<VkCompositeAlphaFlagBitsKHR> composite_alpha_flags = {
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
        };

        for ( auto& flag : composite_alpha_flags )
        {
            if (surf_caps.supportedCompositeAlpha & flag)
            {
                composite_alpha = flag;
                break;
            }
        }

        auto indices = find_queue_families(physical_device, surface);
        assert(indices.complete());

        queue_node_index = indices.graphics.value();

        std::array<uint32_t,2> queue_family_indices {indices.graphics.value(), indices.present.value()};

        VkSharingMode sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
        uint32_t queue_family_index_count = 0;
        const uint32_t* queue_indices = nullptr;

        if (indices.graphics != indices.present)
        {
            sharing_mode = VK_SHARING_MODE_CONCURRENT;
            queue_family_index_count = 2;
            queue_indices = queue_family_indices.data();
        }

        VkSwapchainCreateInfoKHR CI {};
        CI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        CI.surface = surface;
        CI.minImageCount = sc_image_count;
        CI.imageFormat = color_format;
        CI.imageColorSpace = color_space;
        CI.imageExtent = { sc_extent.width, sc_extent.height };
        CI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        CI.preTransform = static_cast<VkSurfaceTransformFlagBitsKHR>(pre_transform);
        CI.imageArrayLayers = 1;
        CI.imageSharingMode = sharing_mode;
        CI.queueFamilyIndexCount = queue_family_index_count;
        CI.pQueueFamilyIndices = queue_indices;
        CI.presentMode = scpm;
        CI.oldSwapchain = old;
        CI.clipped = VK_TRUE;
        CI.compositeAlpha = composite_alpha;

        if (surf_caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
            CI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        if (surf_caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            CI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        OP_SUCCESS(vkCreateSwapchainKHR(device, &CI, allocation_callbacks,&swapchain));

        if(VK_NULL_HANDLE != old)
        {
            for (size_t i = 0; i < image_count; i++)
                vkDestroyImageView(device, buffers[i].view, allocation_callbacks);

            vkDestroySwapchainKHR(device, old, allocation_callbacks);
        }

        OP_SUCCESS(vkGetSwapchainImagesKHR(device,swapchain,&image_count, nullptr));

        images.resize(image_count);
        OP_SUCCESS(vkGetSwapchainImagesKHR(device,swapchain,&image_count,images.data()));

        buffers.resize(image_count);
        for(size_t i = 0; i < image_count; i++)
        {
            auto color_attch_view = info::image_view_create_info();
            color_attch_view.pNext = nullptr;
            color_attch_view.format = color_format;
            color_attch_view.components = {
                VK_COMPONENT_SWIZZLE_R,
                VK_COMPONENT_SWIZZLE_G,
                VK_COMPONENT_SWIZZLE_B,
                VK_COMPONENT_SWIZZLE_A
            };
            color_attch_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            color_attch_view.subresourceRange.baseMipLevel = 0;
            color_attch_view.subresourceRange.levelCount = 1;
            color_attch_view.subresourceRange.baseArrayLayer = 0;
            color_attch_view.subresourceRange.layerCount = 1;
            color_attch_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
            color_attch_view.flags = 0;

            buffers[i].image = images[i];

            OP_SUCCESS(vkCreateImageView(device, &color_attch_view, allocation_callbacks, &buffers[i].view));
        }
    }

    std::tuple<VkResult,uint32_t> swap_chain::acquire_next_image(VkSemaphore present_complete)
    {
        constexpr uint64_t time_out = std::numeric_limits<uint64_t>::max();
        uint32_t index;
        VkResult result = vkAcquireNextImageKHR(device, swapchain, time_out, present_complete, VK_NULL_HANDLE, &index);

        return std::tie(result,index);
    }

    VkResult swap_chain::queue_present(VkQueue queue, const uint32_t image_index, VkSemaphore wait)
    {
        VkPresentInfoKHR PI {};
        PI.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        PI.pNext = nullptr;
        PI.swapchainCount = 1;
        PI.pSwapchains = &swapchain;
        PI.pImageIndices = &image_index;

        if(VK_NULL_HANDLE != wait)
        {
            PI.pWaitSemaphores = &wait;
            PI.waitSemaphoreCount = 1;
        }

        return vkQueuePresentKHR(queue, &PI);
    }

    void swap_chain::cleanup()
    {
        if (VK_NULL_HANDLE != swapchain)
        {
            for (uint32_t i = 0; i < image_count; i++)
                vkDestroyImageView(device, buffers[i].view, allocation_callbacks);
        }
        if (VK_NULL_HANDLE != surface)
        {
            vkDestroySwapchainKHR(device, swapchain, allocation_callbacks);
            vkDestroySurfaceKHR(instance, surface, allocation_callbacks);
        }

        surface = VK_NULL_HANDLE;
        swapchain = VK_NULL_HANDLE;
    }
}