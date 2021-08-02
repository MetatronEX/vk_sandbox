#include "vk_logicaldevice.hpp"

namespace vk
{
    void logical_device::create(VkPhysicalDevice _pd)
    {
        physical_device = _pd;

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

        OP_SUCCESS(vkCreateDevice(physical_device, &DC, allocation_callbacks, &device));

        commandpool = create_commandpool(qfi.graphics.value());
    }

    VkCommandPool logical_device::create_commandpool(const uint32_t queue_familiy_index, const VkCommandPoolCreateFlags flags)
    {
        return create_commandpool(device, queue_familiy_index, flags);
    }
}