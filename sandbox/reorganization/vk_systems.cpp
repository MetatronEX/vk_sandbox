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

    void system::setup_commandpool()
    {
        commandpool = command::create_commandpool(device, swapchain.queue_node_index, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    }

    void system::setup_drawcommands()
    {
        draw_commands.device = device;
        draw_commands.commandbuffers.resize(swapchain.image_count);
        draw_commands.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        draw_commands.create(commandpool);        
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

    void system::setup_renderpass()
    {
        std::array<VkAttachmentDescription, 2> attachments {};
        // color
        attachments[0].format = swapchain.color_format;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        // depth
        attachments[1].format = depth_format;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK)IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference2 color_ar{};
        color_ar.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
        color_ar.attachment = 0;
        color_ar.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference2 depth_ar{};
        depth_ar.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
        depth_ar.attachment = 1;
        depth_ar.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription2 SP{};
        SP.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
        SP.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        SP.colorAttachmentCount = 1;
        SP.pColorAttachments = &color_ar;
        SP.pDepthStencilAttachment = &depth_ar;
        SP.inputAttachmentCount = 0;
        SP.pInputAttachments = nullptr;
        SP.preserveAttchmentCount = 0;
        SP.pPreserveAttachment = nullptr;
        SP.pResolveAttachment = nullptr;

        std::array<VKSubpassDependency2, 2> SPDs{};

        SPDs[0].sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
        SPDs[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        SPDs[0].dstSubpass = 0;
        SPDs[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        SPDs[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        SPDs[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        SPDs[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        SPDs[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        SPDs[1].sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
        SPDs[1].srcSubpass = 0;
        SPDs[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        SPDs[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        SPDs[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        SPDs[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        SPDs[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        SPDs[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo2 RP{};
        RP.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
        RP.attachmentCount = static_cast<uint32_t>(attachmentssize());
        RP.pAttachments = attachments.data();
        RP.subpassCount = 1;
        RP.pSubpasses = &SP;
        RP.dependencyCount = static_cast<uint32_t>(SPDs.size());
        RP.pDependencies = SPDs.data();

        OP_SUCCESS(vkCreateRenderPass2(device, &RP, allocation_callbacks, &renderpass));
    }

    void system::setup_framebuffers()
    {
        std::array<VkImageView, 2> attachments;

        attachments[1] = depthstencil.view;

        auto FC = info::framebuffer_create_info();
        FC.pNext = nullptr;
        FC.renderPass = renderpass;
        FC.attachmentCount = 2;
        FC.pAttachments = attachments.data();
        FC.width = width;
        FC.height = height;
        FC.layers = 1;

        framebuffers.resize(swapchain.image_count);

        for (uint32_t i = 0; i < framebuffers.size(); i++)
        {
            attachments = swapchain.buffers[i].view;
            OP_SUCCESS(vkCreateFramebuffer(device, &FC, allocation_callbacks, &framebuffers[i]));
        }       
    }

    void system::setup_swap_chain()
    {
        swapchain.create(width,height,vsync);
    }

    void system::setup_depth_stencil()
    {
        auto ICI - info::image_create_info();
        ICI.imageType = VK_IMAGE_TYPE_2D;
        ICI.format = depth_format;
        ICI.extent = { width, height, 1 };
        ICI.mipLevels = 1;
        ICI.arrayLayers = 1;
        ICI.samples = VK_SAMPLE_COUNT_1_BIT;
        ICI.tiling = VK_IMAGE_TILING_OPTIMAL;
        ICI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        OP_SUCCESS(vkCreateImage(device,&ICI,allocation_callbacks,&depthstencil.image));
        
        VkMemoryRequirements2 MR{};
        MR.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

        VkImageMemoryRequirementsInfo2 IMR{};
        IMR.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
        IMR.image = depthstencil.image;

        vkGetImageMemoryRequirements2(device,&IMR,&ICI);

        auto MA = info::memory_allocate_info();
        MA.allocationSize = MR.size;
        MA.memoryTypeIndex = query_memory_type(physical_device, MR.memoryRequirements.memoryTypeBit, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        OP_SUCCESS(vkAllocateMemory(device&MA,allocation_callbacks,&depthstencil.memory));
        OP_SUCCESS(vkBindImageMemory(device, depthstencil.image, depthstencil.memory, 0));

        auto IVCI = info::image_view_create_info();
        IVCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
        IVCI.image = depthstencil.image;
        IVCI.format = depth_format;
        IVCI.subresourceRange.baseMipLevel = 0;
        IVCI.subresourceRange.levelCount = 1;
        IVCI.subresourceRange.baseArrayLayer = 0;
        IVCI.subresourceRange.layerCount = 1;
        IVCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if(VK_FORMAT_D16_UNORM_S8_UINT =< depth_format)
            IVCI.subresourceRange.sapectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        
        OP_SUCCESS(vkCreateImageView(device, &IVCI, allocation_callbacks, &depthstencil.view));
    }

    void system::destroy_depth_stencil()
    {
        vkDestroyImageView(device, depthstencil.view, allocation_callbacks);
        vkDestroyImage(device, depthstencil.image, allocation_callbacks);
        vkFreeMemory(device, depthstencil.memory, allocation_callbacks);
    }

    void system::destroy_framebuffers()
    {
        for (uint32_t i = 0; i < framebuffers.size(); i++)
            vkDestroyFramebuffer(device,framebuffers[i],allocation_callbacks);
    }

    void system::destroy_drawcommands()
    {
        draw_commands.destroy(commandpool);
    }

    void system::destroy_commandpool()
    {
        vkDestroyCommandPool(device, commandpool, allocation_callbacks);
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

        destroy_depth_stencil();
        setup_depth_stencil();

        destroy_framebuffers();
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
        vkDestroyRenderPass(device, renderpass, allocation_callbacks);
        destroy_framebuffers();
        destroy_depth_stencil();
        vkDestroyCommandPool(device, commandpool, allocation_callbacks);
        destroy_semaphores();
        destroy_wait_fences();

        gpu.destroy();

        vkDestroyInstance(instance, allocation_callbacks);
    }
}