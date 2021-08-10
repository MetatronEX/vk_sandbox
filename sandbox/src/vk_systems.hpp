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
    template<typename draw_policy>
    struct system 
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
        pipeline<draw_policy>           render_pipeline;

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

        // calling order setup_instance, init surface, prime

        void                            prime()
        {
            pick_physical_device();
            setup_logical_device();
            setup_graphics_queue();
            setup_drawcommands();
            setup_swap_chain();
            setup_drawcommands();
            setup_wait_fences();
            setup_semaphores();
            render_pipeline.setup_pipeline_depth_stencil();
            render_pipeline.setup_pipeline_renderpass();
            render_pipeline.setup_pipeline_framebuffers();
        }

        void                            setup_instance(const bool enable_validation)
        {
            VkApplicationInfo AI{};
            AI.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            AI.pApplicationName = application_name;
            AI.pEngineName = engine_name;
            AI.apiVersion = api_version;

            uint32_t extensions_count = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, nullptr);

            std::vector<const char*> supported_instance_extensions;

            if (extensions_count > 0)
            {
                std::vector<VkExtensionProperties> extensions(extensions_count);
                if (vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, extensions.data()) == VK_SUCCESS)
                {
                    for (const auto e : extensions)
                        supported_instance_extensions.push_back(e.extensionName);
                }
            }

            if (enabled_instance_extensions.size() > 0)
            {
                for (const auto& ee : enabled_instance_extensions)
                {
                    if (std::find(supported_instance_extensions.begin(), supported_instance_extensions.end(), ee) ==
                        supported_instance_extensions.end())
                        std::cerr << "Enabled instance extension \"" << ee << "\" is not present at instance level\n";

                    instance_extensions.push_back(ee);
                }
            }

            VkInstanceCreateInfo CI{};
            CI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            CI.pNext = nullptr;
            CI.pApplicationInfo = &AI;

            if (instance_extensions.size() > 0)
            {
                // if (enable_validation)
                // {
                //     // validation stuff here
                // }

                CI.enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size());
                CI.ppEnabledExtensionNames = instance_extensions.data();
            }

            // more validation set up here

            VkResult error = vkCreateInstance(&CI, allocation_callbacks, &instance);

            if (error)
                fatal_exit("No instance of Vulkan was created successfully: ", error);

        }

        void                            pick_physical_device()
        {
            uint32_t gpu_count = 0;
            OP_SUCCESS(vkEnumeratePhysicalDevices(instance, &gpu_count, nullptr));

            if (0 == gpu_count)
            {
                // deal with this TODO: verbose prompter
            }

            std::vector<VkPhysicalDevice> PDs(gpu_count);
            OP_SUCCESS(vkEnumeratePhysicalDevices(instance, &gpu_count, PDs.data()));

            for (const auto& pd : PDs)
            {
                if (is_device_suitable(pd, surface, gpu.supported_device_extensions))
                {
                    physical_device = pd;
                    break;
                }
            }

            if (VK_NULL_HANDLE == physical_device)
            {
                // deal with this TODO: verbose prompter
            }
        }

        void                            setup_logical_device()
        {
            render_pipeline.gpu = &gpu;

            gpu.allocation_callbacks = allocation_callbacks;
            gpu.physical_device = physical_device;
            gpu.headless_rendering = headless_rendering;
            gpu.queue_indices = find_queue_families(physical_device, surface);

            render_pipeline.setup_pipeline_queried_features();
            
            gpu.create();

            device = gpu.device;
        }

        void                            setup_graphics_queue()
        {
            vkGetDeviceQueue(device, gpu.queue_indices.graphics.value(), 0, &present_queue);

            auto SC = info::semaphore_create_info();

            OP_SUCCESS(vkCreateSemaphore(device, &SC, allocation_callbacks, &present_complete));
            OP_SUCCESS(vkCreateSemaphore(device, &SC, allocation_callbacks, &render_complete));

            submit_info = info::submit_info();
            submit_info.pWaitDstStageMask = &submit_pipeline_stages;
            submit_info.waitSemaphoreCount = 1;
            submit_info.pWaitSemaphores = &present_complete;
            submit_info.signalSemaphoreCount = 1;
            submit_info.pSignalSemaphores = &render_complete;
        }

        void                            setup_swap_chain()
        {
            swapchain.surface = surface;
            swapchain.create(width, height, vsync);
            gpu.swapchain = &swapchain;
        }

        void                            setup_draw_commandpool()
        {
            auto CPC = info::command_pool_create_info();
            CPC.queueFamilyIndex = swapchain.queue_node_index;
            CPC.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            OP_SUCCESS(vkCreateCommandPool(device, &CPC, allocation_callbacks, &draw_commandpool));
        }

        void                            setup_drawcommands()
        {
            gpu.draw_commands.commandbuffers.resize(swapchain.image_count);
            gpu.draw_commands.allocate(device, draw_commandpool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        }

        void                            setup_semaphores()
        {
            auto SC = info::semaphore_create_info();
            OP_SUCCESS(vkCreateSemaphore(device, &SC, allocation_callbacks, &present_complete));
            OP_SUCCESS(vkCreateSemaphore(device, &SC, allocation_callbacks, &render_complete));
        }

        void                            setup_wait_fences()
        {
            wait_fences.resize(gpu.draw_commands.commandbuffers.size());
            auto FC = info::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
            for (auto& f : wait_fences)
                OP_SUCCESS(vkCreateFence(device, &FC, allocation_callbacks, &f));
        }
			                            
        void                            prepare_frame()
        {
            auto [result, index] = swapchain.acquire_next_image(present_complete);

            if (VK_ERROR_OUT_OF_DATE_KHR == result || VK_SUBOPTIMAL_KHR == result)
                resize_window();
            else
                OP_SUCCESS(result);

            buffer_index = index;
        }

        void                            submit_frame()
        {
            auto result = swapchain.queue_present(present_queue, buffer_index, render_complete);

            if (!(VK_SUCCESS == result || VK_SUBOPTIMAL_KHR == result))
            {
                if (VK_ERROR_OUT_OF_DATE_KHR == result)
                {
                    resize_window();
                    return;
                }
                else
                {
                    OP_SUCCESS(result);
                }
            }

            OP_SUCCESS(vkQueueWaitIdle(present_queue));
        }

        void                            resize_window()
        {
            if (!prepared)
                return;

            prepared = false;
            resized = true;

            vkDeviceWaitIdle(device);

            width = new_width;
            height = new_height;
            gpu.width = width;
            gpu.height = height;
            setup_swap_chain();

            gpu.destroy_depth_stencil();
            render_pipeline.setup_pipeline_depth_stencil();

            gpu.destroy_framebuffers();
            render_pipeline.setup_pipeline_framebuffers();

            destroy_drawcommands();
            setup_drawcommands();

            vkDeviceWaitIdle(device);

            resizing = false;
            view_updated = true;

            prepared = true;
        }
			                            
        void                            destroy_drawcommands()
        {
            gpu.draw_commands.free(device, draw_commandpool);
        }

        void                            destroy_draw_commandpool()
        {
            vkDestroyCommandPool(device, draw_commandpool, allocation_callbacks);
        }

        void                            destroy_semaphores()
        {
            vkDestroySemaphore(device, present_complete, allocation_callbacks);
            vkDestroySemaphore(device, render_complete, allocation_callbacks);
        }
        
        void                            destroy_wait_fences()
        {
            for (auto& f : wait_fences)
                vkDestroyFence(device, f, allocation_callbacks);
        }
			                            
        void                            destroy_system()
        {
            swapchain.cleanup();

            if (VK_NULL_HANDLE != descriptor_pool)
                vkDestroyDescriptorPool(device, descriptor_pool, allocation_callbacks);

            destroy_drawcommands();
            gpu.destroy_renderpass();
            gpu.destroy_framebuffers();
            gpu.destroy_depth_stencil();
            destroy_draw_commandpool();
            destroy_semaphores();
            destroy_wait_fences();

            gpu.destroy();

            vkDestroyInstance(instance, allocation_callbacks);
        }
    };
}

#endif