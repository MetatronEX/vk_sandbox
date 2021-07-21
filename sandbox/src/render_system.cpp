#include "render_system.hpp"

#include <memory>
#include <set>
#include <cstring>
#include <algorithm> 

namespace sandbox
{
	namespace debug
	{
		const std::vector<const char*> validation_layers =
		{
			"VK_LAYER_KHRONOS_validation"
		};

		VkDebugUtilsMessengerEXT	debug_messenger;

#ifdef _DEBUG
		const bool enable_validation_layers = true;
#else
		const bool enable_validation_layers = false;
#endif

		bool check_validation_layer_support()
		{
			unsigned layer_count;
			vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

			std::vector<VkLayerProperties> available_layers(layer_count);
			vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

			for (auto l : validation_layers)
			{
				bool found = false;

				for (const auto& p : available_layers)
				{
					if (0 == strcmp(l, p.layerName))
					{
						found = true;
						break;
					}
				}

				if (!found)
				{
					return false;
				}
			}

			return true;
		}

		VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
			VkDebugUtilsMessageSeverityFlagBitsEXT msg_severity,
			VkDebugUtilsMessageTypeFlagsEXT msg_type,
			const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
			void* user_data)
		{
			if (msg_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
				std::cerr << "Validation Layer: " << callback_data->pMessage << std::endl;

			return VK_FALSE;
		}

		VkResult CreateDebugUtilsMessengerEXT(VkInstance inst, const VkDebugUtilsMessengerCreateInfoEXT*
			create_info, const VkAllocationCallbacks* allocator, VkDebugUtilsMessengerEXT* debug_messenger)
		{
			auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(inst,
				"vkCreateDebugUtilsMessengerEXT");

			if (nullptr != func)
			{
				return func(inst, create_info, allocator, debug_messenger);
			}
			else
			{
				return VK_ERROR_EXTENSION_NOT_PRESENT;
			}
		}

		void DestroyDebugUtilsMessengerEXT(VkInstance inst, VkDebugUtilsMessengerEXT debug_messenger,
			const VkAllocationCallbacks* allocator)
		{
			auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(inst,
				"vkDestroyDebugUtilsMessengerEXT");

			if (nullptr != func)
			{
				func(inst, debug_messenger, allocator);
			}
		}

		/*
			vkCreateDebugUtilsMessengerEXT call requires a valid instance to have been created and
			vkDestroyDebugUtilsMessengerEXT must be called before the instance is destroyed. This
			currently leaves us unable to debug any issues in the vkCreateInstance and vkDestroyInstance
			calls.
		*/
		void populate_debug_msg_info(VkDebugUtilsMessengerCreateInfoEXT& debug_info)
		{
			debug_info = {};
			debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			debug_info.pfnUserCallback = debug_callback;
			debug_info.pUserData = nullptr;
		}

		void setup_debug_messenger(VkInstance instance)
		{
			if (!enable_validation_layers)
				return;

			VkDebugUtilsMessengerCreateInfoEXT create_info;
			populate_debug_msg_info(create_info);

			if (!OP_SUCCESS(CreateDebugUtilsMessengerEXT(instance, &create_info, nullptr, &debug_messenger)))
			{
				throw std::runtime_error("Debug Messenger set up failed!");
			}
		}
	}

	const std::vector<const char*> dev_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	struct queue_family_indices
	{
		std::optional<unsigned> graphics_family;
		std::optional<unsigned> present_family;

		bool is_complete()
		{
			return graphics_family.has_value() && present_family.has_value();
		}
	};

	struct swap_chain_support
	{
		VkSurfaceCapabilitiesKHR			capabilities;
		std::vector<VkSurfaceFormatKHR>		formats;
		std::vector<VkPresentModeKHR>		present_modes;
	};

	bool check_dev_extension_support(VkPhysicalDevice dev)
	{
		unsigned ext_count = 0;
		vkEnumerateDeviceExtensionProperties(dev, nullptr, &ext_count, nullptr);

		std::vector<VkExtensionProperties> available_exts(ext_count);
		vkEnumerateDeviceExtensionProperties(dev, nullptr, &ext_count, available_exts.data());

		std::set<std::string> required_exts(dev_extensions.begin(), dev_extensions.end());

		for (const auto& e : available_exts)
		{
			required_exts.erase(e.extensionName);
		}

		return required_exts.empty();
	}

	queue_family_indices find_queue_families(VkPhysicalDevice dev, VkSurfaceKHR surface)
	{
		queue_family_indices indices;

		unsigned fam_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties2(dev, &fam_count, nullptr);

		std::vector<VkQueueFamilyProperties2> queue_fams(fam_count);

		for (auto& q : queue_fams)
		{
			q.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
		}

		vkGetPhysicalDeviceQueueFamilyProperties2(dev, &fam_count, queue_fams.data());

		int i = 0;
		for (const auto& qf : queue_fams)
		{
			if (qf.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphics_family = i;
			}

			VkBool32 present_support = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &present_support);

			if (present_support)
				indices.present_family = i;


			if (indices.is_complete())
				break;

			i++;
		}

		return indices;
	}

	swap_chain_support query_sc_support(VkPhysicalDevice dev, VkSurfaceKHR surface)
	{
		swap_chain_support details{};

		if (!OP_SUCCESS(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surface, &details.capabilities)))
		{
			throw std::runtime_error("Failed to query for physical device surface capabilities!");
		}

		unsigned fmt_count = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &fmt_count, nullptr);

		if (0 != fmt_count)
		{
			details.formats.resize(fmt_count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &fmt_count, details.formats.data());
		}

		unsigned pm_count = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &pm_count, nullptr);

		if (0 != pm_count)
		{
			details.present_modes.resize(pm_count);
			vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &pm_count, details.present_modes.data());
		}

		return details;
	}

	queue_family_indices find_queue_families(VkPhysicalDevice dev, VkSurfaceKHR surface)
	{
		queue_family_indices indices;

		unsigned fam_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties2(dev, &fam_count, nullptr);

		std::vector<VkQueueFamilyProperties2> queue_fams(fam_count);

		for (auto& q : queue_fams)
		{
			q.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
		}

		vkGetPhysicalDeviceQueueFamilyProperties2(dev, &fam_count, queue_fams.data());

		int i = 0;
		for (const auto& qf : queue_fams)
		{
			if (qf.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphics_family = i;
			}

			VkBool32 present_support = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &present_support);

			if (present_support)
				indices.present_family = i;


			if (indices.is_complete())
				break;

			i++;
		}

		return indices;
	}

	bool is_device_suitable(VkPhysicalDevice dev, VkSurfaceKHR surface)
	{
		VkPhysicalDeviceProperties2 dev_props{};
		dev_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		VkPhysicalDeviceFeatures2 dev_feats{};
		dev_feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

		vkGetPhysicalDeviceProperties2(dev, &dev_props);
		vkGetPhysicalDeviceFeatures2(dev, &dev_feats);

		queue_family_indices indices = find_queue_families(dev, surface);

		bool extensions_supported = check_dev_extension_support(dev);

		bool sw_adequate = false;

		if (extensions_supported)
		{
			swap_chain_support sw_support = query_sc_support(dev, surface);
			sw_adequate = !sw_support.formats.empty() && !sw_support.present_modes.empty();
		}

		return  dev_props.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
			dev_feats.features.tessellationShader &&
			indices.is_complete() &&
			extensions_supported &&
			dev_feats.features.samplerAnisotropy &&
			sw_adequate;
	};

	

	void render_system::create_surface()
	{
		if (!OP_SUCCESS(glfwCreateWindowSurface(instance, window, alloc_callback, &surface)))
		{
			throw std::runtime_error("Failed to create window surface!");
		}
	}

	VkSurfaceFormatKHR render_system::choose_swap_surface_fmt(const std::vector<VkSurfaceFormatKHR>& available_fmts)
	{
		for (const auto& fmt : available_fmts)
		{
			if (VK_FORMAT_R8G8B8A8_SRGB == fmt.format &&
				VK_COLOR_SPACE_SRGB_NONLINEAR_KHR == fmt.colorSpace)
			{
				return fmt;
			}
		}

		return available_fmts[0];
	}

	VkPresentModeKHR render_system::choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_modes)
	{
		for (const auto& mode : available_modes)
		{
			if (VK_PRESENT_MODE_MAILBOX_KHR == mode) // triple buffering
				return mode;
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D render_system::choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (std::numeric_limits<unsigned>::max() != capabilities.currentExtent.width)
		{
			return capabilities.currentExtent;
		}
		else
		{
			int width, height;

			glfwGetFramebufferSize(window, &width, &height);

			VkExtent2D actual_extent = {
				static_cast<unsigned>(width),
				static_cast<unsigned>(height)
			};

			actual_extent.width = std::max(capabilities.minImageExtent.width,
				std::min(capabilities.maxImageExtent.width, actual_extent.width));

			actual_extent.height = std::max(capabilities.minImageExtent.height,
				std::min(capabilities.maxImageExtent.height, actual_extent.height));

			return actual_extent;
		}
	}

	void render_system::create_swap_chain()
	{
		swap_chain_support support = query_sc_support(pd, surface);

		VkSurfaceFormatKHR surface_fmt = choose_swap_surface_fmt(support.formats);
		VkPresentModeKHR present_mode = choose_swap_present_mode(support.present_modes);
		VkExtent2D extent = choose_swap_extent(support.capabilities);

		unsigned image_count = support.capabilities.minImageCount + 1;

		if (support.capabilities.maxImageCount > 0 && image_count > support.capabilities.maxImageCount)
			image_count = support.capabilities.maxImageCount;

		VkSwapchainCreateInfoKHR create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		create_info.surface = surface;

		create_info.minImageCount = image_count;
		create_info.imageFormat = surface_fmt.format;
		create_info.imageColorSpace = surface_fmt.colorSpace;
		create_info.imageExtent = extent;
		create_info.imageArrayLayers = 1;
		
		create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		queue_family_indices indices = find_queue_families(pd, surface);
		unsigned q_fam_indices[] = { indices.graphics_family.value(), indices.present_family.value() };

		if (indices.graphics_family != indices.present_family)
		{
			create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			create_info.queueFamilyIndexCount = 2;
			create_info.pQueueFamilyIndices = q_fam_indices;
		}
		else
		{
			create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			create_info.queueFamilyIndexCount = 0;
			create_info.pQueueFamilyIndices = nullptr;
		}

		create_info.preTransform = support.capabilities.currentTransform;
		create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		create_info.presentMode = present_mode;
		create_info.clipped = VK_TRUE;

		create_info.oldSwapchain = VK_NULL_HANDLE;

		if (!OP_SUCCESS(vkCreateSwapchainKHR(dev, &create_info, alloc_callback, &swap_chain)))
		{
			throw std::runtime_error("Failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(dev, swap_chain, &image_count, nullptr);
		sc_images.resize(image_count);
		vkGetSwapchainImagesKHR(dev, swap_chain, &image_count, sc_images.data());

		sc_img_fmt = surface_fmt.format;
		sc_extent = extent;
	}

	void render_system::clean_swap_chain()
	{

	}

	void render_system::recreate_swap_chain()
	{

	}

	void update_ubo(uint32_t curr_img)
	{
		
	}

	void render_system::draw_frame()
	{
		vkWaitForFences(dev, 1, &in_flight_fences[curr_frame], VK_TRUE, std::numeric_limits<uint64_t>::max());

		unsigned image_index;

		auto result = vkAcquireNextImageKHR(dev, swap_chain, std::numeric_limits<uint64_t>::max(),
			image_semaphores[curr_frame], VK_NULL_HANDLE, &image_index);

		
		if (VK_ERROR_OUT_OF_DATE_KHR == result)
		{
			recreate_swap_chain();
			return;
		}
		else
		{
			if (VK_SUCCESS != result && VK_SUBOPTIMAL_KHR == result)
			{
				throw std::runtime_error("Failed to acquire swap chain image!");
			}
		}

		if (VK_NULL_HANDLE != images_in_flight[image_index])
		{
			vkWaitForFences(dev, 1, &images_in_flight[image_index], VK_TRUE,
				std::numeric_limits<uint64_t>::max());
		}

		images_in_flight[image_index] = in_flight_fences[curr_frame];

		update_ubo(image_index);

		VkSubmitInfo submit{};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore wait_sems[] = { image_semaphores[curr_frame] };
		VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submit.waitSemaphoreCount = 1;
		submit.pWaitSemaphores = wait_sems;
		submit.pWaitDstStageMask = wait_stages;
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &cmd_buffers[image_index];

		VkSemaphore sig_sems[] = { rp_semaphores[curr_frame] };
		submit.signalSemaphoreCount = 1;
		submit.pSignalSemaphores = sig_sems;

		vkResetFences(dev, 1, &in_flight_fences[curr_frame]);

		if (!OP_SUCCESS(vkQueueSubmit(graphics_queue, 1, &submit, in_flight_fences[curr_frame])))
		{
			throw std::runtime_error("Failed to submit draw command buffer!");
		}

		VkPresentInfoKHR present_info{};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = sig_sems;

		VkSwapchainKHR swap_chains[] = { swap_chain };
		present_info.swapchainCount = 1;
		present_info.pSwapchains = swap_chains;
		present_info.pImageIndices = &image_index;
		present_info.pResults = nullptr;

		result = vkQueuePresentKHR(present_queue, &present_info);

		if (VK_ERROR_OUT_OF_DATE_KHR == result || VK_SUBOPTIMAL_KHR == result || fb_resized)
		{
			fb_resized = false;
			recreate_swap_chain();
		}
		if (VK_SUCCESS != result)
		{
			std::runtime_error("Swap chain image acquisition failed!");
		}

		vkQueueWaitIdle(present_queue);

		curr_frame = (curr_frame + 1) % max_frames_in_flight;
	
	}

	void render_system::create_instance()
	{
		if (debug::enable_validation_layers && !debug::check_validation_layer_support())
		{
			throw std::runtime_error("Validation layer requested, but not available!");
		}

		VkApplicationInfo app_info{  };
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName = "sandbox";
		app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.pEngineName = "sandbox engine";
		app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.apiVersion = VK_MAKE_VERSION(1, 2, 0);

		VkInstanceCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.pApplicationInfo = &app_info;

		auto get_required_extensions = []()
		{
			unsigned glfw_ext_count = 0;
			const char** glfw_extensions = nullptr;

			glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_ext_count);


			std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_ext_count);

			if (debug::enable_validation_layers)
			{
				extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			}

			{
#ifdef  _DEBUG
				std::cout << "Available GLFW Extensions:\n";

				for (unsigned i = 0; i < extensions.size(); i++)
				{
					std::cout << extensions[i] << '\n';
				}

				std::cout << "Total GLFW Extensions count: " << extensions.size() << std::endl;
#endif
			}

			return extensions;
		};

		auto exts = get_required_extensions();
		create_info.enabledExtensionCount = static_cast<unsigned>(exts.size());
		create_info.ppEnabledExtensionNames = exts.data();

		VkDebugUtilsMessengerCreateInfoEXT debug_info;

		if (debug::enable_validation_layers)
		{
			create_info.enabledLayerCount = static_cast<unsigned>(debug::validation_layers.size());
			create_info.ppEnabledLayerNames = debug::validation_layers.data();

			debug::populate_debug_msg_info(debug_info);
			create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_info;
		}
		else
		{
			create_info.enabledLayerCount = 0;
			create_info.pNext = nullptr;
		}

		if (!OP_SUCCESS(vkCreateInstance(&create_info, nullptr, &instance)))
		{
			throw std::runtime_error("Failed to create vulkan instance!");
		}

		unsigned ext_count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, nullptr);

		std::vector<VkExtensionProperties> extensions(ext_count);
		vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, extensions.data());

		{
#if _DEBUG
			std::cout << "Available Extensions : Spec Version\n";

			for (const auto& e : extensions)
			{
				std::cout << '\t' << e.extensionName << " : " << e.specVersion << '\n';
			}

			std::cout << "Total Extension count: " << ext_count << '\n';
#endif
		}
	}

	void render_system::pick_physical_device()
	{
		unsigned dev_count = 0;
		vkEnumeratePhysicalDevices(instance, &dev_count, nullptr);

		if (0 == dev_count)
		{
			throw std::runtime_error("There are not GPU on this computer with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(dev_count);
		vkEnumeratePhysicalDevices(instance, &dev_count, devices.data());

#ifdef _DEBUG
		std::cout << "Available Devices" << std::endl;

		for (const auto& d : devices)
		{
			VkPhysicalDeviceProperties2 dev_props{};
			dev_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			vkGetPhysicalDeviceProperties2(d, &dev_props);
			std::cout << '\t' << dev_props.properties.deviceName << '\n';
		}
#endif
		for (const auto& d : devices)
		{
			if (is_device_suitable(d, surface))
			{
				pd = d;

				break;
			}
		}

		if (VK_NULL_HANDLE == pd)
		{
			throw std::runtime_error("Failed to find a suitable GPU!");
		}
	}

	void render_system::create_logical_device()
	{
		queue_family_indices indices = find_queue_families(pd,surface);

		std::vector<VkDeviceQueueCreateInfo> q_create_infos;
		std::set<unsigned> unique_q_fams = { indices.graphics_family.value(), indices.present_family.value() };

		float queue_priority = 1.f;

		for (const auto q_fam : unique_q_fams)
		{
			VkDeviceQueueCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			create_info.queueFamilyIndex = q_fam;
			create_info.queueCount = 1;
			create_info.pQueuePriorities = &queue_priority;
			q_create_infos.emplace_back(create_info);
		}

		VkPhysicalDeviceFeatures2 dev_feats{};
		dev_feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

		VkDeviceCreateInfo dev_info{};
		dev_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		dev_info.pQueueCreateInfos = q_create_infos.data();
		dev_info.queueCreateInfoCount = static_cast<unsigned>(q_create_infos.size());
		dev_info.pEnabledFeatures = &dev_feats.features;

		dev_info.enabledExtensionCount = static_cast<unsigned>(dev_extensions.size());
		dev_info.ppEnabledExtensionNames = dev_extensions.data();

		if (debug::enable_validation_layers)
		{
			dev_info.enabledLayerCount = static_cast<unsigned>(debug::validation_layers.size());
			dev_info.ppEnabledLayerNames = debug::validation_layers.data();
		}
		else
		{
			dev_info.enabledLayerCount = 0;
		}

		dev_feats.features.samplerAnisotropy = VK_TRUE;

		if (!OP_SUCCESS(vkCreateDevice(pd, &dev_info, nullptr, &dev)))
		{
			throw std::runtime_error("Failed to create a logical device!");
		}

		VkDeviceQueueInfo2 devq_info{};
		devq_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
		devq_info.queueFamilyIndex = indices.graphics_family.value();
		devq_info.queueIndex = 0;

		vkGetDeviceQueue2(dev, &devq_info, &graphics_queue);

		devq_info.queueFamilyIndex = indices.present_family.value();
		vkGetDeviceQueue2(dev, &devq_info, &present_queue);
	}
}