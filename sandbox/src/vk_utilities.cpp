#include "vk.hpp"

namespace vk
{
	namespace command
	{
		void set_image_layout(VkCommandBuffer commandbuffer, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, VkImageSubresourceRange& subresource_range,
			VkPipelineStageFlags src_mask, VkPipelineStageFlags dst_mask)
		{
			auto IMB = info::image_memory_barrier();
			IMB.oldLayout = old_layout;
			IMB.newLayout = new_layout;
			IMB.image = image;
			IMB.subresourceRange = subresource_range;

			switch (old_layout)
			{
			case VK_IMAGE_LAYOUT_UNDEFINED:
				IMB.srcAccessMask = 0;
				break;

			case VK_IMAGE_LAYOUT_PREINITIALIZED:
				IMB.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				IMB.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				IMB.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				IMB.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				IMB.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				IMB.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				break;
			}

			switch (new_layout)
			{
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				IMB.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				IMB.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				IMB.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				IMB.dstAccessMask = IMB.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				if (IMB.srcAccessMask == 0)
					IMB.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;

				IMB.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				break;
			}

			vkCmdPipelineBarrier(commandbuffer, src_mask, dst_mask, 0, 0, nullptr, 0, nullptr, 1, &IMB);
		}

		void set_image_layout(VkCommandBuffer commandbuffer, VkImage image, VkImageAspectFlags aspect_mask, VkImageLayout old_layout, VkImageLayout new_layout,
			VkPipelineStageFlags src_mask, VkPipelineStageFlags dst_mask)
		{
			VkImageSubresourceRange ISR{};
			ISR.aspectMask = aspect_mask;
			ISR.baseMipLevel = 0;
			ISR.levelCount = 1;
			ISR.layerCount = 1;
			set_image_layout(commandbuffer, image, old_layout, new_layout, ISR, src_mask, dst_mask);
		}

		void insert_image_memory_barrier(VkCommandBuffer commandbuffer, VkImage image, VkAccessFlags src_access, VkAccessFlags dst_access, VkImageLayout old_layout, VkImageLayout new_layout,
			VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage, VkImageSubresourceRange subresource_range)
		{
			auto IMB = info::image_memory_barrier();
			IMB.srcAccessMask = src_access;
			IMB.dstAccessMask = dst_access;
			IMB.oldLayout = old_layout;
			IMB.newLayout = new_layout;
			IMB.image = image;
			IMB.subresourceRange = subresource_range;

			vkCmdPipelineBarrier(commandbuffer, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &IMB);
		}
	}

    const char* error_string(VkResult error_code)
    {
        switch (error_code)
        {
#define STR(r) case VK_ ##r: return #r
            STR(NOT_READY);
            STR(TIMEOUT);
            STR(EVENT_SET);
            STR(EVENT_RESET);
            STR(INCOMPLETE);
            STR(ERROR_OUT_OF_HOST_MEMORY);
            STR(ERROR_OUT_OF_DEVICE_MEMORY);
            STR(ERROR_INITIALIZATION_FAILED);
            STR(ERROR_DEVICE_LOST);
            STR(ERROR_MEMORY_MAP_FAILED);
            STR(ERROR_LAYER_NOT_PRESENT);
            STR(ERROR_EXTENSION_NOT_PRESENT);
            STR(ERROR_FEATURE_NOT_PRESENT);
            STR(ERROR_INCOMPATIBLE_DRIVER);
            STR(ERROR_TOO_MANY_OBJECTS);
            STR(ERROR_FORMAT_NOT_SUPPORTED);
            STR(ERROR_SURFACE_LOST_KHR);
            STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
            STR(SUBOPTIMAL_KHR);
            STR(ERROR_OUT_OF_DATE_KHR);
            STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
            STR(ERROR_VALIDATION_FAILED_EXT);
            STR(ERROR_INVALID_SHADER_NV);
#undef STR
        default:
            return "UNKNOWN_ERROR";
        }
    }

    void fatal_exit(const char* message, const int32_t code)
    {
        std::cerr << message << error_string(static_cast<VkResult>(code)) << "\n";
        exit(code);
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

    queue_family_indices find_queue_families(VkPhysicalDevice _pd, VkSurfaceKHR _s)
    {
        queue_family_indices indices;

        auto queue_families = query_queue_family_properties(_pd);

        for (size_t i = 0; i < queue_families.size(); i++)
        {
            if (!indices.graphics.has_value() &&
                queue_families[i].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                indices.graphics = i;

            if (!indices.compute.has_value() &&
                queue_families[i].queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT)
                indices.compute = i;

            if (!indices.transfer.has_value() &&
                queue_families[i].queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT)
                indices.transfer = i;

            if (!indices.sparse_binding.has_value() &&
                queue_families[i].queueFamilyProperties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
                indices.sparse_binding = i;
        }

        bool support_present = support_present_to_surface(_pd, _s, indices.graphics.value());

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

    uint32_t query_memory_type(VkPhysicalDevice _pd, const uint32_t type_filter, const VkMemoryPropertyFlags flags, bool* found)
    {
        VkPhysicalDeviceMemoryProperties2 MP{};
        MP.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
        vkGetPhysicalDeviceMemoryProperties2(_pd, &MP);

        for (uint32_t i = 0; i < MP.memoryProperties.memoryTypeCount; i++)
        {
            if ((type_filter & (1 << i)) &&
                (MP.memoryProperties.memoryTypes[i].propertyFlags & flags) == flags)
            {
                if (found)
                    *found = true;

                return i;
            }
        }

        if (found)
            *found = false;

        return 0;
    }

    std::vector<VkExtensionProperties> query_available_extensions(VkPhysicalDevice _pd)
    {
        uint32_t ext_count = 0;
        vkEnumerateDeviceExtensionProperties(_pd, nullptr, &ext_count, nullptr);

        std::vector<VkExtensionProperties> available_extensions(ext_count);
        vkEnumerateDeviceExtensionProperties(_pd, nullptr, &ext_count, available_extensions.data());

        return available_extensions;
    }

    bool check_extension_support(VkPhysicalDevice _pd, const std::vector<const char*>& device_extensions)
    {
        auto available_extensions = query_available_extensions(_pd);
        std::set<const char*> required_extensions(device_extensions.begin(), device_extensions.end());

        for (const auto& e : available_extensions)
            required_extensions.erase(e.extensionName);

        return required_extensions.empty();
    }

    bool is_device_suitable(VkPhysicalDevice _pd, VkSurfaceKHR _s, const std::vector<const char*>& device_extensions)
    {
        VkPhysicalDeviceProperties2 dev_props{};
        dev_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        VkPhysicalDeviceFeatures2 dev_feats{};
        dev_feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

        vkGetPhysicalDeviceProperties2(_pd, &dev_props);
        vkGetPhysicalDeviceFeatures2(_pd, &dev_feats);

        auto indices = find_queue_families(_pd, _s);

        bool extensions_supported = check_extension_support(_pd, device_extensions);

        bool adequate = false;

        if (extensions_supported)
        {
            auto formats = query_surface_formats(_pd, _s);
            auto present_modes = query_surface_present_modes(_pd, _s);

            adequate = !formats.empty() && !present_modes.empty();
        }

        return (dev_props.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
            dev_feats.features.tessellationShader &&
            indices.complete() &&
            extensions_supported &&
            dev_feats.features.samplerAnisotropy &&
            adequate);
    }

    bool query_depth_format_support(VkPhysicalDevice _pd, VkFormat& _f, const bool check_sample_support)
    {
        std::array<VkFormat, 5> formats = {
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D24_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM
        };

        VkFormatProperties2 FP{};
        FP.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;

        for (auto& f : formats)
        {
            vkGetPhysicalDeviceFormatProperties2(_pd, f, &FP);

            if (FP.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                if (check_sample_support)
                    if (!(VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT & FP.formatProperties.optimalTilingFeatures))
                        continue;

                _f = f;
                return true;
            }
        }

        return false;
    }

    VkFormatProperties query_format_properties(VkPhysicalDevice _pd, const VkFormat _f)
    {
        VkFormatProperties FP;
        vkGetPhysicalDeviceFormatProperties(_pd, _f, &FP);
        return FP;
    }

    bool is_format_filterable(VkPhysicalDevice _pd, const VkFormat _f, const VkImageTiling _t)
    {
        VkFormatProperties2 FP{};
        FP.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
        vkGetPhysicalDeviceFormatProperties2(_pd, _f, &FP);

        if (VK_IMAGE_TILING_OPTIMAL == _t)
            return FP.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
        if (VK_IMAGE_TILING_LINEAR == _t)
            return FP.formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;

        return false;
    }
}