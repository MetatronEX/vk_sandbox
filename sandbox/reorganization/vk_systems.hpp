#ifndef VK_SYSTEMS_HPP
#define VK_SYSTEMS_HPP

#include "vk.hpp"

#include "vk_swapchain.hpp"
#include "vk_depthstencil.hpp"
#include "vk_commandbuffer.hpp"
#include "vk_fence.hpp"

#include <vector>

namespace vk
{
    struct system
    {
        VkInstance                      instance;
        VkPhysicalDevice                physical_device;
        VkDevice                        device;

        VkAllocationCallbacks*          allocation_callbacks {nullptr};
        VkSurfaceKHR                    surface;
        
        VkRenderPass                    renderpass;
        VkCommandPool                   commandpool;

        VkSemaphore                     present_complete;
        VkSemaphore                     render_complete;

        VkFormat                        depth_format;

        swap_chain                      swapchain;
        depth_stencil                   depthstencil;
        command_buffer                  commandbuffers;
        
        std::vector<VkFramebuffer>      framebuffers;
        std::vector<fence>              wait_fences;

        const char*                     application_name;
        const char*                     engine_name;

        uint32_t                        api_version;
        uint32_t                        width;
        uint32_t                        height;
        uint32_t                        new_width;
        uint32_t                        new_height;
        uint32_t                        buffer_index;

        bool                            prepared {false};
        bool                            resizing {false};
        bool                            view_updated {false};
        bool                            resized {false};
        bool                            vsync {false};

        std::vector<const char*>    supported_instance_extensions;
        std::vector<const char*>    enabled_instance_extensions;

        void setup_instance(const bool enable_validation);
        void pick_physical_device();

        void setup_swap_chain();
        void setup_depth_stencil();

        void setup_commandpool();
        void setup_commandbuffers();
        void setup_renderpass();
        void setup_framebuffers();
        void setup_semaphores();
        void setup_wait_fences();
        
        void setup_fence(fence& _fence);

        void prepare_frame();
        void submit_frame();

        void resize_window();

        void destroy_depth_stencil();
        void destroy_framebuffers();
        void destroy_commandbuffers();
        void destroy_commandpool();
        void destroy_semaphores();
        void destroy_wait_fences();

        uint32_t query_memory_type(const uint32_t type_filter, const VkMemoryPropertyFlags flags);
    };
}

#endif