#ifndef VK_SYSTEMS_HPP
#define VK_SYSTEMS_HPP

#include "vk.hpp"

#include "vk_swapchain.hpp"
#include "vk_depthstencil.hpp"
#include "vk_drawcommand.hpp"
#include "vk_fence.hpp"
#include "vk_gpu.hpp"

#include <vector>

namespace vk
{
    const std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    struct system
    {
        VkInstance                      instance;
        VkPhysicalDevice                physical_device;
        VkDevice                        device;

        VkAllocationCallbacks*          allocation_callbacks {nullptr};
        VkSurfaceKHR                    surface;
        
        VkRenderPass                    renderpass;
        VkCommandPool                   commandpool;

        VkQueue                         present_queue;

        std::vector<VkFramebuffer>      framebuffers;

        VkSemaphore                     present_complete;
        VkSemaphore                     render_complete;

        VkFormat                        depth_format;

        VkQueueFlags                    requested_queue_types;

        VKSubmitInfo                    submit_info;

        GPU                             gpu;
        swap_chain                      swapchain;
        depth_stencil                   depthstencil;
        draw_command                    draw_commands;
        
        std::vector<VkFence>            wait_fences;

        void*                           features_chain {nullptr};

        const char*                     application_name;
        const char*                     engine_name {"Luminodynamics"};

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
        bool                            headless_rendering {false};

        
        std::vector<const char*>        enabled_instance_extensions;
        
        void setup_instance(const bool enable_validation);
        void pick_physical_device();
        void setup_logical_device();

        void setup_swap_chain();
        void setup_depth_stencil();

        void setup_commandpool();
        void setup_drawcommands();
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
        void destroy_drawcommands();
        void destroy_commandpool();
        void destroy_semaphores();
        void destroy_wait_fences();
    };
}

#endif