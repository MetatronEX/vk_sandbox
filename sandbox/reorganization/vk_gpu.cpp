#include "vk_logicaldevice.hpp"

#include <fstream>

namespace vk
{
    VkResult GPU::create()
    {
        queue_familiy_indices qfi = find_queue_families(physical_device);

        constexpr float default_priority = 0.0f;

        std::vector<VkDeviceQueueCreateInfo>    DQCs{};

        if(VK_QUEUE_GRAPHICS_BIT & requested_queue_types)
        {
            VkDeviceQueueCreateInfo DQC{};
            DQC.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            DQC.queueFamilyIndex = qfi.graphics.value();
            DQC.queueCount = 1;
            DQC.pQueuePriorities = &default_priority;
            DQCs.push_back(DQC);
        }

        if(VK_QUEUE_COMPUTE_BIT & requested_queue_types)
        {
            if(qfi.compute.value() != qfi.graphics.value())
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
                qfi.graphics.value() != qfi.transfer.value() &&
                qfi.compute.value() != qfi.transfer.value()
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

        if(headless_rendering)
            device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        VkPhysicalDeviceFeatures2 PDF {};
        PDF.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

        vkGetPhysicalDeviceFeatures2(physical_device, &PDF);
        PDF.pNext = features_chain;

        VkDeviceCreateinfo DC{};
        DC.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        DC.queueCreateinfoCount = static_cast<uint32_t>(DQCs.size());
        DC.pQueueCreateInfos = DQCs.data();
        DC.pNext = &PDF;

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

        commandpool = create_commandpool(qfi.graphics.value());

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
        return query_memory_type(physical_device, type_bits, properties, found);
    }

    VkCommandPool GPU::create_commandpool(const uint32_t queue_familiy_index, const VkCommandPoolCreateFlags flags)
    {
        return create_commandpool(device, queue_familiy_index, flags);
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

    VkShaderModule GPU::load_shader(const char* working_path)
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
        VkbufferMemoryRequiremntsInfo2 BMR{};
        BMR.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_@;
        BMR.buffer = buffer;
        vkGetBufferMemoryRequirements2(device, &BMR, &MR);
        
        auto MA = info::memory_allocate_info();
        MA.allocationSize = MR.memoryRequirements.size;
        MA.memoryTypeIndex = query_memory_type(physical_device, MR.memoryRequirements.memoryTypeBit, property);

        VkmemoryAllocateFlagsInfoKHR MAF{};
        
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

        VkmemoryAllocateFlagsInfoKHR MAF{};
        
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
}