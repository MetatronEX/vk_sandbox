#ifndef VK_HPP
#define VK_HPP

#inlcude <vulkan/vulkan.h>

#include <optional>

#define VK_FLAGS_NONE 0

#define DEFAULT_FENCE_TIMEOUT 100000000000

#define OP_SUCCESS(f)																				                                      \
{																										                                  \
	VkResult res = (f);																					                                  \
	if (res != VK_SUCCESS)																				                                  \
	{																									                                  \
		std::cout << "Fatal : VkResult is \"" << vks::tools::errorString(res) << "\" in " << __FILE__ << " at line " << __LINE__ << "\n"; \
		assert(res == VK_SUCCESS);																		                                  \
	}																									                                  \
}

namespace vk
{
    const std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    struct queue_family_indices
    {
        std::optional<uint32_t> graphics;
        std::optional<uint32_t> present;
        std::optional<uint32_t> compute;
        std::optional<uint32_t> transfer;
        std::optional<uint32_t> sparse_binding;

        bool complete()
        {
            return graphics.has_value() && present.has_value();
        }
    };

    VkSurfaceKHR initialize_win32_surface(VkInstance _instance, VkAllocationCallbacks* allocation_callbacks)
    {
        HWND hWnd = GetActiveWindow();
        HINSTANCE hInstance = GetModuleHandle(NULL);

        VkWin32SurfaceCreateInfoKHR CI{};
        CI.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        CI.hInstance = hInstance;
        CI.hwnd = hWnd;

        VkSurfaceKHR surface;
        OP_SUCCESS(vkCreateWin32SurfaceKHR(instance, &CI, allocation_callbacks, &surface));

        return surface;
    }

    VkSurfaceCapabilitiesKHR query_surface_capabilities(VkPhysicalDevice _pd, VkSurfaceKHR _s)
    {
        VkSurfaceCapabilitiesKHR surf_caps;

        OP_SUCCESS(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_pd, _s, &surf_caps));

        return surf_caps;
    }

    std::vector<VkSurfaceFormatKHR> query_surface_formats(VkPhysicalDevice _pd, VkSurfaceKHR _s)
    {
        uint32_t format_count;
        OP_SUCCESS(vkGetPhysicalDeviceSurfaceFormatsKHR(_pd, _s, &format_count, nullptr));
        assert(format_count > 0);

        std::vector<VkSurfaceFormatKHR> surface_formats(format_count);
        OP_SUCCESS(vkGetPhysicalDeviceSurfaceFormatsKHR(_pd, _s, &format_count, surface_formats.data()));

        return surface_formats;
    }

    std::vector<VkPresentModeKHR> query_surface_present_modes(VkPhysicalDevice _pd, VkSurfaceKHR _s)
    {
        uint32_t present_mode_count;
        OP_SUCCESS(vkGetPhysicalDeviceSurfacePresentModesKHR(_pd, _s, &present_mode_count, nullptr));
        assert(present_mode_count > 0);

        std::vector<VkPresentModeKHR> present_modes;
        OP_SUCCESS(vkGetPhysicalDeviceSurfacePresentModesKHR(_pd, _s, &present_mode_count, present_modes.data()));

        return present_modes;
    }

    bool support_present_to_surface(VkPhysicalDevice _pd, VkSurfaceKHR _s, const uint32_t index)
    {
        VkBool32 support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(_pd, index, _s, &support);
        return support ? true : false;
    }

    queue_family_indices find_queue_families(VkPhysicalDevice _pd, VkSurfaceKHR _s)
    {
        queue_family_indices indices;

        auto queue_families = query_queue_family_properties(_pd);

        for (size_t i = 0; i < queue_families.size(); i++)
        {
            if(!indices.graphics.has_value() && 
            queue_families[i].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                indices.graphics = i;
            
            if(!indices.compute.has_value() &&
            queue_families[i].queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT)
                indices.compute = i;

            if(!indices.transfer.has_value() &&
            queue_families[i].queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT)
                indices.transfer = i;

            if(!indices.sparse_binding.has_value() &&
            queue_families[i].queueFamilyProperties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
                indices.sparse_binding = i;
        }

        bool support_present = support_present_to_surface(_pd,_s,indices.graphics.value());

        if (support_present)
            indices.present = indices.graphics.value();
        else
        {
            for (uint32_t i = 0; i < queue_families.size(); i++)
            {
                bool support = support_present_to_surface(_pd, _s, i);

                if (support)
                {
                    indices.present = i;
                    break;
                }
            }
        }

        return indices;
    }

    std::vector<VkQueueFamilyProperties2> query_queue_family_properties(VkPhysicalDevice _pd)
    {
        uint32_t family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties2(_pd, &family_count, nullptr);
        assert(family_count >= 1);

        std::vector<VkQueueFamilyProperties2> queue_families(family_count);

        for (auto& F : queue_families)
            F.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
        
        vkGetPhysicalDeviceQueueFamilyProperties2(_pd, &family_count, queue_families.data());

        return queue_families;
    }

    bool check_extension_support(VkPhysicalDevice _pd)
    {
        uint32_t ext_count = 0;
        vkEnumerateDeviceExtensionProperties(_pd, nullptr, &ext_count, nullptr);

        std::vector<VkExtensionProperties> available_extensions(ext_count);
        vkEnumerateDeviceExtensionProperties(_pd, nullptr, &ext_count, available_extensions.data());

        std::set<const char*> required_extensions(device_extensions.begin(), device_extensions.end());

        for (const auto& e : available_extensions)
        {
            required_extensions.erase(e.extensionName);
        }

        return required_extensions.empty();
    }

    bool is_device_suitable(VkPhysicalDevice _pd, VkSurfaceKHR _s)
    {
        VkPhysicalDeviceProperties2 dev_props{};
        dev_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        VkPhysicalDeviceFeatures2 dev_feats{};
        dev_feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

        vkGetPhysicalDeviceProperties2(_pd, &dev_props);
		vkGetPhysicalDeviceFeatures2(_pd, &dev_feats);

        auto indices = find_queue_families(_pd, _s);

        bool extensions_supported = check_extension_support(_pd);

        bool adequate = false;

        if (extensions_supported)
        {
            auto formats = query_surface_formats(_pd,_s);
            auto present_modes = query_surface_present_modes(_pd,_s);

            adequate = !formats.empty() && !present_modes.empty();
        }

       return  dev_props.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
        dev_feats.features.tessellationShader &&
        indices.complete() &&
        extensions_supported &&
        dev_feats.features.samplerAnisotropy &&
        adequate; 
    }

    bool query_depth_format_support(VkPhysicalDevice _pd, VkFormat& _f)
    {
        std::array<VkFormat, 5> formats - {
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D24_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM
        };

        for (auto& f : formats)
        {
            VkFormatProperties2 FP{};
            FP.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
            vkGetPhysicalDeviceFormatProperties2(_pd, f, &FP);

            if(FP.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                _f = f;
                return true;
            }
        }

        return false;
    }

    bool is_depth_format_filterable(VkPhysicalDevice _pd, const VkFormat _f, const VKImageTiling _t)
    {
        VkFormatProperties2 FP{};
        FP.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
        vkGetPhysicalDeviceFormatProperties2(_pd, f, &FP);

        if (VK_IMAGE_TILING_OPTIMAL == _t) 
            return FP.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
        if (VK_IMAGE_TILING_LINEAR == _t)
            return FP.formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;

        return false;
    }

    VkCommandPool create_commandpool(VkDevice device, const uint32_t queue_familiy_index, const VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
    {
        VkCommandPool CP;
        VkCommandPoolCreateInfo CPC{}:
        CPC.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        CPC.queueFamilyIndex = queue_familiy_index;
        CPC.flags = flags;
        OP_SUCCESS(vkCreateCommandPool(device,&CPC,allocation_callbacks,&CP));
        return CP;
    }
}

#endif