#include "vk_systems.hpp"

namespace vk
{
    void system::setup_instance(const bool enable_validation)
    {
        VkApplicationInfo AI{};
        AI.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        AI.pApplicationName = application_name;
        AI.pEngineName = engine_name;
        AI.apiVersion = api_version;

        std::vector<const char*> instance_extensions = { VK_KHR_SURFACE_EXTENSION_NAME };
        instance_extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

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
                if(std::find(supported_instance_extensions.begin(), supported_instance_extensions.end(), ee) == 
                supported_instance_extensions.end())
                    std::err << "Enabled instance extension \"" << ee << "\" is not present at instance level\n";
                
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
            fatal_exit("No instance of Vulkan was created successfully: " + error_string(error), error);

    }

    void system::pick_physical_device()
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
            if(is_device_suitable(pd,surface))
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

    void system::setup_logical_device()
    {
        gpu.allocation_callbacks = allocation_callbacks;
        gpu.physical_device = physical_device;
        gpu.create();
        device = gpu.device;
    }

    void system::setup_graphics_queue()
    {
        vkGetDeviceQueue(device, gpu.queue_indices.graphics.value(), 0, &present_queue);

        bool valid_depth = query_depth_format_support(physical_device, &depth_format);
        assert(valid_depth);

        auto SC = info::semaphore_create_info();

        OP_SUCCESS(vkCreateSemaphore(device, &SC, allocation_callbacks, &present_complete));
        OP_SUCCESS(vkCreateSemaphore(device, &SC, allocation_callbacks, &render_complete));

        submit_info = info::submit_info();
        submit_info.pWaitDstStageMask = &submit_pipeline_stages;
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &present_complete;
        submit_info.signalSemaphoreCount = 1;
        submit_info..pSignalSemaphores = &render_complete;
    }

    void system::prepare_frame()
    {
        auto [result, index] = swapchain.acquire_next_image(present_complete);

        if( VK_ERROR_OUT_OF_DATE_KHR == result || VK_SUBOPTIMAL_KHR == result)
            resixe_window();
        else
            OP_SUCCESS(result);
        
        buffer_index = index;
    }

    void system::submit_frame()
    {
        auto result = swapchain.queue_present(present_queue, buffer_index, render_complete);

        if (!(VK_SUCCESS == result || VK_SUBOPTIMAL_KHR == result))
        {
            if(VK_ERROR_OUT_OF_DATE_KHR == result)
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

    void system::setup_draw_commandpool()
    {
        draw_commandpool = command::create_commandpool(device, swapchain.queue_node_index, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    }

    void system::setup_drawcommands()
    {
        gpu.draw_commands.commandbuffers.resize(swapchain.image_count);
        gpu.draw_commands.allocate(device, draw_commandpool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);        
    }

    void system::setup_semaphores()
    {
        auto SC = info::semaphore_create_info();
        OP_SUCCESS(vkCreateSemaphore(device, &SC, allocation_callbacks, &present_complete));
        OP_SUCCESS(vkCreateSemaphore(device, &SC, allocation_callbacks, &render_complete));
    }

    void system::setup_wait_fences()
    {
        wait_fences.resize(draw_commands.size());
        auto FC = info::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
        for(auto& f : wait_fences)
            OP_SUCCESS(vkCreateFence(device,&FC,allocation_callbacks,&f));     
    }

    void system::setup_swap_chain()
    {
        swapchain.create(width,height,vsync);
    }

    void system::destroy_drawcommands()
    {
        gpu.draw_commands.free(draw_commandpool);
    }

    void system::destroy_draw_commandpool()
    {
        vkDestroyCommandPool(device, draw_commandpool, allocation_callbacks);
    }

    void system::destroy_semaphores()
    {
        vkDestroySemaphore(device, present_complete, allocation_callbacks);
        vkDestroySemaphore(device, render_complete, allocation_callbacks);
    }

    void system::destroy_wait_fences()
    {
        for (auto& f : wait_fences)
            vkDestroyFence(device, f, allocation_callbacks);
    }

    void system::resize_window()
    {
        if (!prepared)
            return;
        
        prepared = false;
        resized = true;

        vkDeviceWaitIdle(device);

        width = new_width;
        height = new_height;
        setup_swap_chain();

        gpu.destroy_depth_stencil();
        /*policy*/setup_depth_stencil();

        gpu.destroy_framebuffers();
        setup_framebuffers();

        destroy_drawcommands();
        setup_drawcommands();

        vkDeviceWaitIdle(device);

        resizing = false;
        view_updated = true;

        prepared = true;
    }

    void system::destroy_system()
    {
        swapchain.cleanup();

        if(VK_NULL_HANDLE != descriptor_pool)
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
}