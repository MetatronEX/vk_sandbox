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
                {
                    std::err << "Enabled instance extension \"" << ee << "\" is not present at instance level\n";
                }
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

            CI.enabledExtensionCOunt = static_cast<uint32_t>(instance_extensions.size());
            CI.ppEnabledExtensionNames = instance_extensions.data();
        }

        // more validation set up here

        OP_SUCCESS(vkCreateInstance(&CI, allocation_callbacks, &instance));
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

    }

    uint32_t system::query_memory_type(const uint32_t type_filter, const VkMemoryPropertyFlags flags)
    {
        VkPhysicalDeviceMemoryProperties2 MP{};
        MP.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
        vkGetPhysicalDeviceMemoryProperties2(physical_device, &MP);

        for (uint32_t i = 0; i < MP.memoryProperties.memoryTypeCount; i++)
        {
            if((type_filter & (1 << i)) &&
            (MP.memoryProperties.memoryTypes[i].propertyFlags & flags) == flags)
            return i;
        }

        return 0;
    }

    void system::setup_commandpool()
    {
        VkCommandPoolCreateInfo CPC{}:
        CPC.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        CPC.queueFamilyIndex = swapchain.queue_node_index;
        CPC.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        OP_SUCCESS(vkCreateCommandPool(device,&CPC,allocation_callbacks,&commandpool));
    }

    void system::setup_commandbuffers()
    {
        commandbuffers.device = device;
        commandbuffers.commandbuffers.resize(swapchain.image_count);
        commandbuffers.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandbuffers.create(commandpool);        
    }

    void system::setup_fence(fence& _fence)
    {
        VkFenceCreateInfo FC{};
        FC.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        FC.flags = _fence.flags;
        OP_SUCCESS(vkCreateFence(device,&FC,allocation_callbacks,&_fence.fence));
    }

    void system::setup_semaphores()
    {
        VkSemaphoreCreateInfo SC{};
        SC.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        OP_SUCCESS(vkCreateSemaphore(device, &SC, allocation_callbacks, &present_complete));
        OP_SUCCESS(vkCreateSemaphore(device, &SC, allocation_callbacks, &render_complete));
    }

    void system::setup_wait_fences()
    {
        wait_fences.resize(commandbuffers.size());

        for(auto& f : wait_fences)
        {
            f.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            setup_fence(f);
        }        
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

        VkFramebufferCreateInfo FCI{};
        FCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        FCI.pNext = nullptr;
        FCI.renderPass = renderpass;
        FCI.attachmentCount = 2;
        FCI.pAttachments = attachments.data();
        FCI.width = width;
        FCI.height = height;
        FCI.layers = 1;

        framebuffers.resize(swapchain.image_count);

        for (uint32_t i = 0; i < framebuffers.size(); i++)
        {
            attachments = swapchain.buffers[i].view;
            OP_SUCCESS(vkCreateFramebuffer(device, &FCI, allocation_callbacks, &framebuffers[i]));
        }       
    }

    void system::setup_swap_chain()
    {
        swapchain.create(width,height,vsync);
    }

    void system::setup_depth_stencil()
    {
        VkImageCreateInfo ICI{};
        ICI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
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

        VkMemoryAllocateInfo MA{};
        MA.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        MA.allocationSize = MR.size;
        MA.memoryTypeIndex = query_memory_type(MR.memoryRequirements.memoryTypeBit, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        OP_SUCCESS(vkAllocateMemory(device&MA,allocation_callbacks,&depthstencil.memory));
        OP_SUCCESS(vkBindImageMemory(device, depthstencil.image, depthstencil.memory, 0));

        VkImageViewCreateInfo IVCI{};
        IVCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATEINFO;
        IVCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
        IVCI.image = depthstencil.image;
        IVCI.format = depth_format;
        IVCI.subresourceRange.baseMipLevel = 0;
        IVCI.subresourceRange.levelCount = 1;
        IVCI/subresourceRange.baseArrayLayer = 0;
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

    void system::destroy_commandbuffers()
    {
        commandbuffers.destroy(commandpool);
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
            vkDestroyFence(device, f.fence, allocation_callbacks);
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

        destroy_commandbuffers();
        setup_commandbuffers();

        vkDeviceWaitIdle(device);

        resizing = false;
        view_updated = true;

        prepared = true;
    }
}