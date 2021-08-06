#include "vk_gpu.hpp"

#include <fstream>

namespace vk
{
    VkResult GPU::create()
    {
        queue_indices = find_queue_families(physical_device);

        constexpr float default_priority = 0.0f;

        std::vector<VkDeviceQueueCreateInfo>    DQCs{};

        if(VK_QUEUE_GRAPHICS_BIT & requested_queue_types)
        {
            VkDeviceQueueCreateInfo DQC{};
            DQC.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            DQC.queueFamilyIndex = queue_indices.graphics.value();
            DQC.queueCount = 1;
            DQC.pQueuePriorities = &default_priority;
            DQCs.push_back(DQC);
        }

        if(VK_QUEUE_COMPUTE_BIT & requested_queue_types)
        {
            if(queue_indices.compute.value() != queue_indices.graphics.value())
            {
                VkDeviceQueueCreateInfo DQC{};
                DQC.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                DQC.queueFamilyIndex = qft.compute.value();
                DQC.queueCount = 1;
                DQC.pQueuePriorities = &default_priority;
                DQCs.push_back(DQC);
            }
        }

        if(VK_QUEUE_TRANSFER_BIT & requested_queue_types)
        {
            if(
                queue_indices.graphics.value() != queue_indices.transfer.value() &&
                queue_indices.compute.value() != queue_indices.transfer.value()
            )
            {
                VkDeviceQueueCreateInfo DQC{};
                DQC.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                DQC.queueFamilyIndex = qft.transfer.value();
                DQC.queueCount = 1;
                DQC.pQueuePriorities = &default_priority;
                DQCs.push_back(DQC);
            }
        }

        std::vector<const char*> device_extensions(enabled_device_extensions);

        if(!headless_rendering)
            device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        vkGetPhysicalDeviceProperties2(_pd, &device_properties);

        device_memory_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
        vkGetPhysicalDeviceMemoryProperties2(_pd, &device_memory_properties);
        
        device_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        vkGetPhysicalDeviceFeatures2(physical_device, &device_features);
        device_features.pNext = features_chain;

        VkDeviceCreateinfo DC{};
        DC.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        DC.queueCreateinfoCount = static_cast<uint32_t>(DQCs.size());
        DC.pQueueCreateInfos = DQCs.data();
        DC.pNext = &device_features;

        // Debug stuff

        std::vector<const char*> supported_device_extensions;
        uint32_t extension_count = 0;
        vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);

        if (extension_count > 0) 
        {
            std::vector<VkExtensionProperties>  ext_props(extension_count);
            if(vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, ext_props.data()) == VK_SUCCESS)
                for (auto e : ext_props)
                    supported_device_extensions.push_back(e.extensionName);
        }

        if (device_extensions.size() > 0)
        {
            for(const auto& de : device_extensions)
            {
                if(std::find(supported_device_extensions.begin(),supported_device_extensions.end(),de) ==
                supported_device_extensions.end())
                    std::err << "Enabled device extension \"" << de << "\" is not present at device level\n";
            }

            DC.enabledExtensionCount static_cast<uint32_t>(device_extensions.size());
            DC.ppEnabledExtensionNames = device_extensions.data();
        }

        VkResult result = vkCreateDevice(physical_device, &DC, allocation_callbacks, &device);

        if(VK_SUCCESS != result)
            return result;

        commandpool = create_commandpool(queue_indices.graphics.value());

        return result;
    }

    VkFormat GPU::query_depth_format_support(const bool check_sampling_support)
    {
        VkFormat f;
        bool result = query_depth_format_support(physical_device, f, check_sampleing_support);

        if(result)
            return f;
    }

    bool GPU::query_extension_availability(const char* extension)
    {
        auto extensions = query_available_extensions(physical_device);
        return (std::find(extensions.begin(),extensions.end(),extension) != extensions.end());
    }

    uint32_t GPU::query_memory_type(uint32_t type_bits, VkMemoryPropertyFlags properties, bool *found)
    {
        for (uint32_t i = 0; i < device_memory_properties.memoryProperties.memoryTypeCount; i++)
        {
            if((type_filter & (1 << i)) &&
            (device_memory_properties.memoryProperties.memoryTypes[i].propertyFlags & flags) == flags)
            {
                if(found)
                    found = true;
            
                return i;
            }            
        }

        if(found)
            found = false;

        return 0;
    }

    VkCommandPool GPU::create_commandpool(const uint32_t queue_familiy_index, const VkCommandPoolCreateFlags flags)
    {
        return command::create_commandpool(device, queue_familiy_index, flags);
    }

    VkCommandBuffer GPU::create_commandbuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin)
    {
        auto CBA = info::command_buffer_allocate_info(pool, level, 1);
        VkCommandBuffer commandbuffer;

        OP_SUCCESS(vkAllocateCommandBuffers(device, &CBA, &commandbuffer));

        if (begin)
        {
            auto CBB = info::command_buffer_begin_info();
            OP_SUCCESS(vkBeginCommandBuffer(commandbuffer, &CBB));
        }

        return commandbuffer;
    }

    VkCommandBuffer GPU::create_commandbuffer(VkCommandBufferLevel level, bool begin)
    {
        return create_commandbuffer(level,begin);
    }

    VkShaderModule GPU::load_shader_module(const char* working_path)
    {
        std::ifstream s(working_path, std::ios::binary | std::ios::in | std::ios::ate);

        if (s.is_open())
        {
            size_t size = s.tellg();
            s.seekg(0, std::ios::beg);
            char* code = new char[size];
            s.read(code, size);
            s.close()

            assert(size > 0);

            VkShaderModule SM;
            VkShaderModuleCreateInfo SMC{};
            SMC.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            SMC.codeSize = size;
            SMC.pCode = reinterpret_cast<uint32_t*>(code);

            OP_SUCCESS(vkCreateShaderModule(device, &SMC, allocation_callbacks, &SM));

            delete[] code;

            shader_modules.push_back(SM);

            return SM;
        }
        else
        {
            std::cerr << "Error: Could not open shader file \"" << working_path << "\"\n";
            return VK_NULL_HANDLE;
        }
    }

    void GPU::copy_buffer(buffer& dst, buffer& src, VkQueue queue, VkBufferCopy* copy_region)
    {
        assert(dst.size <= src.size);
        assert(src.buffer);
        VkCommandBuffer copy_cmd = create_commandbuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
        VkBufferCopy BC{};
        if(!copy_region)
            BC.size = src.size;
        else
            BC = *copy_region;

        vkCmdCopyBuffer(copy_cmd, src.buffer. dst.buffer, 1, &BC);

        flush_command_buffer(copy_cmd, queue);
    }

    void GPU::flush_command_buffer(VkCommandBuffer commandbuffer, VkQueue queue, VkCommandPool pool, bool free)
    {
        if (VK_NULL_HANDLE == commandbuffer)
            return;

        OP_SUCCESS(vkEndCommandBuffer(commandbuffer));

        auto SI = info::submit_info();
        SI.commandBufferCount = 1;
        SI.pCommandBuffers = &commandbuffer;

        auto FC = info::fence_create_info(VK_FLAGS_NONE);
        VkFence fence;
        OP_SUCCESS(vkCretaeFence(device,&FC,allocation_callbacks,&fence));
        OP_SUCCESS(vkQueueSubmit(queue, 1, &SI, fence));
        OP_SUCCESS(vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
        vkDestroyFence(device, fence, allocation_callbacks);

        if(free)
            vkFreeCommandBuffers(device, pool, 1, &commandbuffer);
    }

	void GPU::flush_command_buffer(VkCommandBuffer commandbuffer, VkQueue queue, bool free)
    {
        flush_command_buffer(commandbuffer, queue, commandpool, free);
    }

    VkResult GPU::create_buffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags property, const VkDeviceSize size, VkBuffer* buffer, VkDeviceMemory* memory, void* data)
    {
        auto BC = info::buffer_create_info(usage, size);
        BC.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        OP_SUCCESS(vkCreateBuffer(device, &BC, allocation_callbacks, buffer));

        VkMemoryRequirements2 MR{};
        MR.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
        VkBufferMemoryRequiremntsInfo2 BMR{};
        BMR.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_@;
        BMR.buffer = buffer;
        vkGetBufferMemoryRequirements2(device, &BMR, &MR);
        
        auto MA = info::memory_allocate_info();
        MA.allocationSize = MR.memoryRequirements.size;
        MA.memoryTypeIndex = query_memory_type(physical_device, MR.memoryRequirements.memoryTypeBit, property);

        VkMemoryAllocateFlagsInfoKHR MAF{};
        
        if(usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
        {
            MAF.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
            MAF.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
            MA.pNext = &MAF;
        }

        OP_SUCCESS(vkAllocateMemory(device, &MA, allocation_callbacks, memory));

        if (data)
        {
            void* mapped;
            OP_SUCCESS(vkmapMemory(device, *memory, 0, size, 0, &mapped));
            memcpy(mapped, data, size);
            if((property & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
            {
                auto MMR = info::mapped_memory_range();
                MMR.memory = *memory;
                MMR.offset = 0;
                MMR.size = size;
                vkFlushMappedMemoryRanges(device, 1, &MMR);
            }
            vkUnmapMemory(device, *memory);
        }

        OP_SUCCESS(vkBindBufferMemory(device, *buffer, *memory, 0));

        return VK_SUCCESS;
    }

    VkResult GPU::create_buffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags property, buffer* buffer, const VkDeviceSize size, void* data)
    {
        buffer->device = device;

        auto BC = info::buffer_create_info(usage, size);
        OP_SUCCESS(vkCreateBuffer(device, &BC, allocation_callbacks, &buffer->buffer));

        VkMemoryRequirements2 MR{};
        MR.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
        VkbufferMemoryRequiremntsInfo2 BMR{};
        BMR.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_@;
        BMR.buffer = buffer;
        vkGetBufferMemoryRequirements2(device, &BMR, &MR);

        auto MA = info::memory_allocate_info();
        MA.allocationSize = MR.memoryRequirements.size;
        MA.memoryTypeIndex = query_memory_type(physical_device, MR.memoryRequirements.memoryTypeBit, property);

        VkMemoryAllocateFlagsInfoKHR MAF{};
        
        if(usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
        {
            MAF.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
            MAF.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
            MA.pNext = &MAF;
        }

        OP_SUCCESS(vkAllocateMemory(device, &MA, allocation_callbacks, memory));

        buffer->alignment = MR.memoryRequirements.alignment;
        buffer->size = size;
        buffer->usage = usage;
        buffer->memory_property = property;

        if (data)
        {
            OP_SUCCESS(buffer->map());
            memcpy(buffer->mapped, data, size);
            if(0 == (property & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
                buffer->flush();
            buffer->unmap();
        }

        buffer->setup_descriptor();

        return buffer->bind();
    }

    void GPU::destroy_depth_stencil()
    {
        vkDestroyImageView(device, depthstencil.view, allocation_callbacks);
        vkDestroyImage(device, depthstencil.image, allocation_callbacks);
        vkFreeMemory(device, depthstencil.memory, allocation_callbacks);
    }

    void GPU::destroy_framebuffers()
    {
        for (uint32_t i = 0; i < framebuffers.size(); i++)
            vkDestroyFramebuffer(device,framebuffers[i],allocation_callbacks);
    }

    voi GPU::destroy_renderpass()
    {
        vkDestroyRenderPass(device, renderpass, allocation_callbacks);
    }

    void GPU::destroy_buffer(buffer& buffer)
    {
        if (buffer.buffer)
            vkDestroyBuffer(device, buffer.buffer, allocation_callbacks);
        if (buffer.memory)
            vkFreeMemory(device, buffer.memory, allocation_callbacks);
    }

    void GPU::destroy()
    {
        if (commandpool)
            vkDestroyCommandPool(device, commandpool, allocation_callbacks);
        if (device)
            vkDestroyDevice(device, allocation_callbacks);
    }

    void GPU::setup_default_depth_stencil()
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

    void GPU::setup_default_renderpass()
    {
        std::array<VkAttachmentDescription2, 2> attachments {};
        // color
        attachments[0].sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
        attachments[0].format = swapchain.color_format;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        // depth
        attachments[1].sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
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

        std::array<VkSubpassDependency2, 2> SPDs{};

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

    void GPU::setup_default_framebuffers()
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
}