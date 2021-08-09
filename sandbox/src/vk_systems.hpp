#ifndef VK_SYSTEMS_HPP
#define VK_SYSTEMS_HPP

#include "vk.hpp"

#include "vk_swapchain.hpp"
#include "vk_drawcommand.hpp"
#include "vk_gpu.hpp"
#include "vk_pipeline.hpp"

#include <vector>

namespace vk
{
    //template<typename pipeline_policy>
    struct system //: public pipeline_policy
    {
        VkInstance                      instance;
        VkPhysicalDevice                physical_device;
        VkDevice                        device;

        VkAllocationCallbacks*          allocation_callbacks {nullptr};
        VkSurfaceKHR                    surface;
        VkCommandPool                   draw_commandpool;
        VkDescriptorPool                descriptor_pool {VK_NULL_HANDLE};
        VkQueue                         present_queue;
        VkSemaphore                     present_complete;
        VkSemaphore                     render_complete;
        VkQueueFlags                    requested_queue_types;
        VkSubmitInfo                    submit_info;

        GPU                             gpu;
        swap_chain                      swapchain;
        pipeline*                       render_pipeline;

        std::vector<VkFence>            wait_fences;
        
        void*                           features_chain {nullptr};

        const char*                     application_name;
        const char*                     engine_name {"Luminodynamics"};

        VkPipelineStageFlags            submit_pipeline_stages {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

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
        
        std::vector<const char*>        instance_extensions;
        std::vector<const char*>        enabled_instance_extensions;

        void                            setup_instance(const bool enable_validation);

        void                            pick_physical_device();

        void                            setup_logical_device();

        void                            setup_graphics_queue();

        void                            setup_swap_chain();

        void                            setup_draw_commandpool();

        void                            setup_drawcommands();

        void                            setup_semaphores();

        void                            setup_wait_fences();
			                            
        void                            prepare_frame();

        void                            submit_frame();

        void                            resize_window();
			                            
        void                            destroy_drawcommands();

        void                            destroy_draw_commandpool();

        void                            destroy_semaphores();
        
        void                            destroy_wait_fences();
			                            
        void                            destroy_system();
    };
}

#endif