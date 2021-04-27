#include "vk_sandbox.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm.hpp>

#include <memory>
#include <vector>
#include <set>
#include <cstring>
#include <optional>
#include <algorithm> 
#include <fstream>

#define OP_SUCCESS(X) VK_SUCCESS == X

namespace sandbox
{
	namespace glfw
	{
		GLFWwindow* window;

		void glfw_initialization(const unsigned res_width, const unsigned res_height)
		{
			glfwInit();

			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
			glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

			window = glfwCreateWindow(res_width, res_height, "sandbox", nullptr, nullptr);
		}
	}
	
	namespace vulkan
	{
		const std::vector<const char*> dev_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		VkInstance					instance;
		VkPhysicalDevice			pd{ VK_NULL_HANDLE };
		VkDevice					dev;
		VkSurfaceKHR				surface;

		VkRenderPass				render_pass;
		VkPipelineLayout			pipeline_layout;

		VkQueue						graphics_queue;
		VkQueue						present_queue;

		VkPipeline					graphics_pipeline;

		VkFormat					sc_img_fmt;
		VkExtent2D					sc_extent;

		std::vector<VkImage>		sc_images;
		std::vector<VkImageView>	sc_image_views;

		struct queue_family_indices
		{
			std::optional<unsigned> graphics_family;
			std::optional<unsigned> present_family;

			bool is_complete()
			{
				return graphics_family.has_value() && present_family.has_value();
			}
		};

		queue_family_indices find_queue_families(VkPhysicalDevice dev)
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



			// find at least one queue family that supports VK_QUEUE_GRAPHICS_BIT
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

			void setup_debug_messenger()
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
		
		namespace KHR
		{
			VkSwapchainKHR swap_chain;

			struct swap_chain_support
			{
				VkSurfaceCapabilitiesKHR			capabilities;
				std::vector<VkSurfaceFormatKHR>		formats;
				std::vector<VkPresentModeKHR>		present_modes;
			};

			/*
				See: https://vulkan-tutorial.com/en/Drawing_a_triangle/Presentation/Window_surface
				For non-glfw create surface
			*/
			void create_surface()
			{
				if (!OP_SUCCESS(glfwCreateWindowSurface(instance, glfw::window, nullptr, &surface)))
				{
					throw std::runtime_error("Failed to create  window surface!");
				}
			}

			VkSurfaceFormatKHR choose_swap_surface_fmt(const std::vector<VkSurfaceFormatKHR>& available_fmts)
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

			VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_modes)
			{
				for (const auto& mode : available_modes)
				{
					if (VK_PRESENT_MODE_MAILBOX_KHR == mode) // triple buffering
						return mode;
				}

				return VK_PRESENT_MODE_FIFO_KHR;
			}

			VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities)
			{
				if (std::numeric_limits<unsigned>::max() != capabilities.currentExtent.width)
				{
					return capabilities.currentExtent;
				}
				else
				{
					int width, height;

					glfwGetFramebufferSize(glfw::window, &width, &height);

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

			swap_chain_support query_sc_support(VkPhysicalDevice dev)
			{
				KHR::swap_chain_support details{};

				vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surface, &details.capabilities);

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

			void create_swap_chain()
			{
				swap_chain_support support = query_sc_support(pd);

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
				/*
				* In that case you may use a value like VK_IMAGE_USAGE_TRANSFER_DST_BIT instead and 
					use a memory operation to transfer the rendered image to a swap chain image.
				*/
				create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

				queue_family_indices indices = find_queue_families(pd);
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

				/*
				* We can specify that a certain transform should be applied to images in the swap chain if it is 
				supported (supportedTransforms in capabilities), like a 90 degree clockwise rotation or horizontal flip. 
				To specify that you do not want any transformation, simply specify the current transformation.
				*/
				create_info.preTransform = support.capabilities.currentTransform;
				create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

				create_info.presentMode = present_mode;
				create_info.clipped = VK_TRUE;

				create_info.oldSwapchain = VK_NULL_HANDLE;

				if (!OP_SUCCESS(vkCreateSwapchainKHR(dev, &create_info, nullptr, &swap_chain)))
				{
					throw std::runtime_error("Failed to create swap chain!");
				}

				vkGetSwapchainImagesKHR(dev, swap_chain, &image_count, nullptr);
				sc_images.resize(image_count);
				vkGetSwapchainImagesKHR(dev, swap_chain, &image_count, sc_images.data());

				sc_img_fmt = surface_fmt.format;
				sc_extent = extent;
			}
		}

		void create_instance()
		{
			if (debug::enable_validation_layers && !debug::check_validation_layer_support())
			{
				throw std::runtime_error("Validation layer requested, but not available!");
			}

			VkApplicationInfo app_info{  };
			app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			app_info.pApplicationName = app::app_name;
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
				
				// iterate through GLFW extensions
				{
#ifdef  _DEBUG
					std::cout << "Available GLFW Extensions:\n";

					for (unsigned i = 0; i < extensions.size(); i++)
					{
						std::cout << extensions[i] << '\n';
					}

					std::cout << "Total GLFW Extensions count: " << extensions.size() << std::endl;
#endif //  _DEBUG
				}

				return extensions;
			};

			auto exts = get_required_extensions();
			create_info.enabledExtensionCount = static_cast<unsigned>(exts.size());
			create_info.ppEnabledExtensionNames = exts.data();

			/*
				debug_info variable is placed outside the if statement to ensure that it is not 
				destroyed before the vkCreateInstance call. By creating an additional debug messenger 
				this way it will automatically be used during vkCreateInstance and vkDestroyInstance 
				and cleaned up after that.
			*/
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
			
			// iterate through extensions
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

		/*
			Instead of just checking if a device is suitable or not and going with the first one,
			you could also give each device a score and pick the highest one. That way you could
			favor a dedicated graphics card by giving it a higher score, but fall back to an
			integrated GPU if that's the only available one.

			See: https://vulkan-tutorial.com/en/Drawing_a_triangle/Setup/Physical_devices_and_queue_families
			For device suitability test
		*/
		bool is_device_suitable(VkPhysicalDevice dev)
		{
			VkPhysicalDeviceProperties2 dev_props{};
			dev_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			VkPhysicalDeviceFeatures2 dev_feats{};
			dev_feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

			vkGetPhysicalDeviceProperties2(dev, &dev_props);
			vkGetPhysicalDeviceFeatures2(dev, &dev_feats);

			queue_family_indices indices = find_queue_families(dev);

			bool extensions_supported = check_dev_extension_support(dev);

			bool sw_adequate = false;

			if(extensions_supported)
			{
				KHR::swap_chain_support sw_support = KHR::query_sc_support(dev);
				sw_adequate = !sw_support.formats.empty() && !sw_support.present_modes.empty();
			}

			return dev_props.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
					dev_feats.features.tessellationShader && 
					indices.is_complete() && 
					extensions_supported && 
					sw_adequate;
		};

		void pick_physical_device()
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
				if (is_device_suitable(d))
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

		void create_logical_device()
		{
			queue_family_indices indices = find_queue_families(pd);

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

		void create_image_views()
		{
			sc_image_views.resize(sc_images.size());

			for (size_t i = 0; i < sc_images.size(); i++)
			{
				VkImageViewCreateInfo create_info{};
				create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				create_info.image = sc_images[i];
				/*
				The viewType and format fields specify how the image data should be interpreted. 
				The viewType parameter allows you to treat images as 1D textures, 2D textures, 
				3D textures and cube maps.
				*/
				create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
				create_info.format = sc_img_fmt;
				/*
				The components field allows you to swizzle the color channels around. For example, 
				you can map all of the channels to the red channel for a monochrome texture. You can 
				also map constant values of 0 and 1 to a channel.
				*/
				create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
				create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
				create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
				create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
				/*
				The subresourceRange field describes what the image's purpose is and which part of 
				the image should be accessed. 

				If you were working on a stereographic 3D application, then you would create a swap 
				chain with multiple layers. You could then create multiple image views for each image 
				representing the views for the left and right eyes by accessing different layers.
				*/
				create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				create_info.subresourceRange.baseMipLevel = 0;
				create_info.subresourceRange.levelCount = 1;
				create_info.subresourceRange.baseArrayLayer = 0;
				create_info.subresourceRange.layerCount = 1;

				/*
				* Unlike images, the image views were explicitly created by us, so we need to add a 
				similar loop to destroy them again at the end of the program
				*/
				if (!OP_SUCCESS(vkCreateImageView(dev, &create_info, nullptr, &sc_image_views[i])))
				{
					throw std::runtime_error("Failed to createimage views!");
				}
				/*
				* An image view is sufficient to start using an image as a texture, but it's not quite
				ready to be used as a render target just yet. That requires one more step of indirection,
				known as a framebuffer. 
				*/
			}
		}

		VkShaderModule create_shader_module(const std::vector<char>& spv)
		{
			VkShaderModuleCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			create_info.codeSize = spv.size();
			create_info.pCode = reinterpret_cast<const unsigned*>(spv.data());

			VkShaderModule shader_module;

			if (!OP_SUCCESS(vkCreateShaderModule(dev, &create_info, nullptr, &shader_module)))
			{
				throw std::runtime_error("failed to create shader module!");
			}

			return shader_module;
		}

		void create_render_pass()
		{
			VkAttachmentDescription2 color_attachment{};
			color_attachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
			color_attachment.format = sc_img_fmt;
			color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;

			/*
			The loadOp and storeOp determine what to do with the data in the attachment before rendering and 
			after rendering. We have the following choices for loadOp:

				- VK_ATTACHMENT_LOAD_OP_LOAD: Preserve the existing contents of the attachment
				- VK_ATTACHMENT_LOAD_OP_CLEAR: Clear the values to a constant at the start
				- VK_ATTACHMENT_LOAD_OP_DONT_CARE: Existing contents are undefined; we don't care about them

			In our case we're going to use the clear operation to clear the framebuffer to black before drawing 
			a new frame. There are only two possibilities for the storeOp:

				- VK_ATTACHMENT_STORE_OP_STORE: Rendered contents will be stored in memory and can be read later
				- VK_ATTACHMENT_STORE_OP_DONT_CARE: Contents of the framebuffer will be undefined after the rendering operation
			*/
			color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

			/*
			The loadOp and storeOp apply to color and depth data, and stencilLoadOp / stencilStoreOp apply to stencil data. 
			Our application won't do anything with the stencil buffer, so the results of loading and storing are irrelevant.
			*/
			color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

			/*
			Textures and framebuffers in Vulkan are represented by VkImage objects with a certain pixel format, however the layout 
			of the pixels in memory can change based on what you're trying to do with an image.

			Some of the most common layouts are:
			
				- VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: Images used as color attachment
				- VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: Images to be presented in the swap chain
				- VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: Images to be used as destination for a memory copy operation

			The initialLayout specifies which layout the image will have before the render pass begins. The finalLayout specifies 
			the layout to automatically transition to when the render pass finishes.

			 Using VK_IMAGE_LAYOUT_UNDEFINED for initialLayout means that we don't care what previous layout the image was in.

			 The caveat of this special value is that the contents of the image are not guaranteed to be preserved, but that 
			 doesn't matter since we're going to clear it anyway.

			 We want the image to be ready for presentation using the swap chain after rendering, which is why we use
			 VK_IMAGE_LAYOUT_PRESENT_SRC_KHR as finalLayout.
			*/
			color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			
			VkAttachmentReference2 ca_ref{};
			ca_ref.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
			/*
			The attachment parameter specifies which attachment to reference by its index in the attachment descriptions array. 
			Our array consists of a single VkAttachmentDescription, so its index is 0. The layout specifies which layout we would 
			like the attachment to have during a subpass that uses this reference. Vulkan will automatically transition the 
			attachment to this layout when the subpass is started. We intend to use the attachment to function as a color buffer 
			and the VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL layout will give us the best performance, as its name implies.
			*/
			ca_ref.attachment = 0;
			ca_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			
			/*
			Vulkan may also support compute subpasses in the future, so we have to be explicit about this being a graphics subpass.
			*/
			VkSubpassDescription2 sp{};
			sp.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
			sp.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			/*
			The index of the attachment in this array is directly referenced from the fragment shader 
			with the layout(location = 0) out vec4 outColor directive.

			The following other types of attachments can be referenced by a subpass:

				- pInputAttachments: Attachments that are read from a shader
				- pResolveAttachments: Attachments used for multisampling color attachments
				- pDepthStencilAttachment: Attachment for depth and stencil data
				- pPreserveAttachments: Attachments that are not used by this subpass, but for which 
				the data must be preserved
			*/
			sp.colorAttachmentCount = 1;
			sp.pColorAttachments = &ca_ref;

			/*
			The render pass object can then be created by filling in the VkRenderPassCreateInfo structure with an array of attachments 
			and subpasses. The VkAttachmentReference objects reference attachments using the indices of this array.
			*/
			VkRenderPassCreateInfo2 rp_info{};
			rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
			rp_info.attachmentCount = 1;
			rp_info.pAttachments = &color_attachment;
			rp_info.subpassCount = 1;
			rp_info.pSubpasses = &sp;

			if (!OP_SUCCESS(vkCreateRenderPass2(dev, &rp_info, nullptr, &render_pass)))
			{
				throw std::runtime_error("Failed to create render pass!");
			}
		}

		/*
		See: https://vulkan-tutorial.com/en/Drawing_a_triangle/Graphics_pipeline_basics/Introduction
		Important read.
		And this: https://vulkan-tutorial.com/en/Drawing_a_triangle/Graphics_pipeline_basics/Conclusion
		*/
		void create_graphics_pipeline()
		{
			// shader stuff
			// serialize spir-v
			auto read_spv = [](const std::string& wp) 
			{
				std::ifstream source(wp, std::ios::ate | std::ios::binary);

				if (!source.is_open())
				{
					throw std::runtime_error("Failed to open .spv file!");
				}

				size_t src_size = static_cast<size_t>(source.tellg());
				std::vector<char> buffer(src_size);

				source.seekg(0);
				source.read(buffer.data(), src_size);

				source.close();

				return buffer;
			};

			auto vert_spv = read_spv("shader/vert.spv");
			auto frag_spv = read_spv("shader/frag.spv");

			VkShaderModule vert_mod = create_shader_module(vert_spv);
			VkShaderModule frag_mod = create_shader_module(frag_spv);

			VkPipelineShaderStageCreateInfo vert_ssinfo{};
			vert_ssinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vert_ssinfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vert_ssinfo.module = vert_mod;
			vert_ssinfo.pName = "main";
			/*
				There is one more (optional) member, pSpecializationInfo, which we won't be using here, 
				but is worth discussing. It allows you to specify values for shader constants. You can 
				use a single shader module where its behavior can be configured at pipeline creation by 
				specifying different values for the constants used in it. This is more efficient than 
				configuring the shader using variables at render time, because the compiler can do 
				optimizations like eliminating if statements that depend on these values. If you don't 
				have any constants like that, then you can set the member to nullptr, which our struct 
				initialization does automatically.

				vert_ssinfo.pSpecializationInfo = nullptr;
			*/
			
			VkPipelineShaderStageCreateInfo frag_ssinfo{};
			frag_ssinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			frag_ssinfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			frag_ssinfo.module = frag_mod;
			frag_ssinfo.pName = "main";

			VkPipelineShaderStageCreateInfo stages[] = { vert_ssinfo, frag_ssinfo };

			// vertex input stuff
			VkPipelineVertexInputStateCreateInfo vtx_input_info{};
			vtx_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vtx_input_info.vertexBindingDescriptionCount = 0;
			vtx_input_info.pVertexBindingDescriptions = nullptr;
			vtx_input_info.vertexAttributeDescriptionCount = 0;
			vtx_input_info.pVertexAttributeDescriptions = nullptr;

			// input assembly stuff
			VkPipelineInputAssemblyStateCreateInfo ia_info{};
			ia_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			ia_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			/*
			Normally, the vertices are loaded from the vertex buffer by index in sequential order, 
			but with an element buffer you can specify the indices to use yourself. This allows you 
			to perform optimizations like reusing vertices. If you set the primitiveRestartEnable 
			member to VK_TRUE, then it's possible to break up lines and triangles in the _STRIP 
			topology modes by using a special index of 0xFFFF or 0xFFFFFFFF.
			*/
			ia_info.primitiveRestartEnable = VK_FALSE;

			//viewport and scissors stuff
			VkViewport vp{};
			vp.x = 0.f;
			vp.y = 0.f;
			vp.width  = static_cast<float>(sc_extent.width);
			vp.height = static_cast<float>(sc_extent.height);
			/*
			* The minDepth and maxDepth values specify the range of depth values to use for the framebuffer. 
			These values must be within the [0.0f, 1.0f] range, but minDepth may be higher than maxDepth. 
			If you aren't doing anything special, then you should stick to the standard values of 0.0f and 
			1.0f.
			*/
			vp.minDepth = 0.f;
			vp.maxDepth = 1.f;

			VkRect2D scissor{};
			scissor.offset = { 0, 0 };
			scissor.extent = sc_extent;

			/*
			* Now this viewport and scissor rectangle need to be combined into a viewport state using the 
			VkPipelineViewportStateCreateInfo struct. It is possible to use multiple viewports and scissor 
			rectangles on some graphics cards, so its members reference an array of them. Using multiple 
			requires enabling a GPU feature (see logical device creation).
			*/
			VkPipelineViewportStateCreateInfo vp_state{};
			vp_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			vp_state.viewportCount = 1;
			vp_state.pViewports = &vp;
			vp_state.scissorCount = 1;
			vp_state.pScissors = &scissor;

			//rasterizer stuff
			/*
			* The rasterizer takes the geometry that is shaped by the vertices from the vertex shader and turns 
			it into fragments to be colored by the fragment shader. It also performs depth testing, face culling 
			and the scissor test, and it can be configured to output fragments that fill entire polygons or just 
			the edges (wireframe rendering). All this is configured using the VkPipelineRasterizationStateCreateInfo 
			structure.
			*/
			VkPipelineRasterizationStateCreateInfo raster_info{};
			raster_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			/*
			* If depthClampEnable is set to VK_TRUE, then fragments that are beyond the near and far planes are
			clamped to them as opposed to discarding them. This is useful in some special cases like shadow maps.
			Using this requires enabling a GPU feature.
			*/
			raster_info.depthBiasEnable = VK_FALSE;
			/*
			* If rasterizerDiscardEnable is set to VK_TRUE, then geometry never passes through the rasterizer stage. 
			This basically disables any output to the framebuffer.
			*/
			raster_info.rasterizerDiscardEnable = VK_FALSE;
			/*
			* Using any mode other than fill requires enabling a GPU feature.
			*/
			raster_info.polygonMode = VK_POLYGON_MODE_FILL;
			/*
			* The lineWidth member is straightforward, it describes the thickness of lines in terms of number of fragments. 
			The maximum line width that is supported depends on the hardware and any line thicker than 1.0f requires you to 
			enable the wideLines GPU feature.
			*/
			raster_info.lineWidth = 1.f;
			raster_info.cullMode = VK_CULL_MODE_BACK_BIT;
			raster_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
			raster_info.depthBiasEnable = VK_FALSE;
			raster_info.depthBiasConstantFactor = 0.f;
			raster_info.depthBiasClamp = 0.f;
			raster_info.depthBiasSlopeFactor = 0.f;

			// Multisampling (MSAA)
			VkPipelineMultisampleStateCreateInfo ms_info{};
			ms_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			ms_info.sampleShadingEnable = VK_FALSE;
			ms_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			ms_info.minSampleShading = 1.f;
			ms_info.pSampleMask = nullptr;
			ms_info.alphaToCoverageEnable = VK_FALSE;
			ms_info.alphaToOneEnable = VK_FALSE;

			// depth stencil to come

			// color blending
			/*
			* After a fragment shader has returned a color, it needs to be combined with the color that is already in the framebuffer. 
			This transformation is known as color blending and there are two ways to do it:

					- Mix the old and new value to produce a final color
					- Combine the old and new value using a bitwise operation
			*/
			/*
			* There are two types of structs to configure color blending. The first struct, VkPipelineColorBlendAttachmentState contains 
			the configuration per attached framebuffer and the second struct, VkPipelineColorBlendStateCreateInfo contains the global color 
			blending settings. In our case we only have one framebuffer:
			*/
			VkPipelineColorBlendAttachmentState cb_attachment{};
			cb_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
				VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			cb_attachment.blendEnable = VK_FALSE;
			cb_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
			cb_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			cb_attachment.colorBlendOp = VK_BLEND_OP_ADD;
			cb_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			cb_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			cb_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
			/*
				This per-framebuffer struct allows you to configure the first way of color blending. The operations that will be performed 
				are best demonstrated using the following pseudocode:

					if (blendEnable) 
					{
						finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb);
						finalColor.a = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a);
					} 
					else 
					{
						finalColor = newColor;
					}	

					finalColor = finalColor & colorWriteMask;

				If blendEnable is set to VK_FALSE, then the new color from the fragment shader is passed through unmodified. Otherwise, the two 
				mixing operations are performed to compute a new color. The resulting color is AND'd with the colorWriteMask to determine which 
				channels are actually passed through.

				The most common way to use color blending is to implement alpha blending, where we want the new color to be blended with the old 
				color based on its opacity. The finalColor should then be computed as follows:

					finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor;
					finalColor.a = newAlpha.a;	

				This can be accomplished with the following parameters:

					colorBlendAttachment.blendEnable = VK_TRUE;
					colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
					colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
					colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
					colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
					colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
					colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

				You can find all of the possible operations in the VkBlendFactor and VkBlendOp enumerations in the specification.
			*/

			/*
				The second structure references the array of structures for all of the framebuffers and allows you to set blend constants that 
				you can use as blend factors in the aforementioned calculations.

				If you want to use the second method of blending (bitwise combination), then you should set logicOpEnable to VK_TRUE. The bitwise 
				operation can then be specified in the logicOp field. Note that this will automatically disable the first method, as if you had 
				set blendEnable to VK_FALSE for every attached framebuffer! The colorWriteMask will also be used in this mode to determine which 
				channels in the framebuffer will actually be affected. It is also possible to disable both modes, as we've done here, in which 
				case the fragment colors will be written to the framebuffer unmodified.
			*/
			VkPipelineColorBlendStateCreateInfo color_blend{};
			color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			color_blend.logicOpEnable = VK_FALSE;
			color_blend.logicOp = VK_LOGIC_OP_COPY;
			color_blend.attachmentCount = 1;
			color_blend.pAttachments = &cb_attachment;
			color_blend.blendConstants[0] = 0.f;
			color_blend.blendConstants[1] = 0.f;
			color_blend.blendConstants[2] = 0.f;
			color_blend.blendConstants[3] = 0.f;

			/*
				Optional dynamic state stuff

				VkDynamicState dynamicStates[] = {
					VK_DYNAMIC_STATE_VIEWPORT,
					VK_DYNAMIC_STATE_LINE_WIDTH
				};

				VkPipelineDynamicStateCreateInfo dynamicState{};
				dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
				dynamicState.dynamicStateCount = 2;
				dynamicState.pDynamicStates = dynamicStates;
			*/

			// pipeline layout
			VkPipelineLayoutCreateInfo pll_info{};
			pll_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pll_info.setLayoutCount = 0;
			pll_info.pSetLayouts = nullptr;
			pll_info.pushConstantRangeCount = 0;
			pll_info.pPushConstantRanges = nullptr;

			if (!OP_SUCCESS(vkCreatePipelineLayout(dev, &pll_info, nullptr, &pipeline_layout)))
			{
				throw std::runtime_error("Failed to create pipeline layout!");
			}

			VkGraphicsPipelineCreateInfo pl_info{};
			pl_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			// shader stages
			pl_info.stageCount = 2;
			pl_info.pStages = stages;
			// fixed functions
			pl_info.pVertexInputState = &vtx_input_info;
			pl_info.pInputAssemblyState = &ia_info;
			pl_info.pViewportState = &vp_state;
			pl_info.pRasterizationState = &raster_info;
			pl_info.pMultisampleState = &ms_info;
			pl_info.pDepthStencilState = nullptr;
			pl_info.pColorBlendState = &color_blend;
			pl_info.pDynamicState = nullptr;
			// pipeline layout
			pl_info.layout = pipeline_layout;
			// render pass
			pl_info.renderPass = render_pass;
			pl_info.subpass = 0;
			/*
			Vulkan allows you to create a new graphics pipeline by deriving from an existing pipeline. The idea 
			of pipeline derivatives is that it is less expensive to set up pipelines when they have much functionality 
			in common with an existing pipeline and switching between pipelines from the same parent can 
			also be done quicker. 

			You can either specify the handle of an existing pipeline with basePipelineHandle or reference another 
			pipeline that is about to be created by index with basePipelineIndex.

			These values are only used if the VK_PIPELINE_CREATE_DERIVATIVE_BIT flag is also specified in the flags field 
			of VkGraphicsPipelineCreateInfo.
			*/
			pl_info.basePipelineHandle = VK_NULL_HANDLE;
			pl_info.basePipelineIndex = 0;

			if (!OP_SUCCESS(vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &pl_info, nullptr, &graphics_pipeline)))
			{
				throw std::runtime_error("Failed to create graphics pipeline!");
			}

			vkDestroyShaderModule(dev, frag_mod, nullptr);
			vkDestroyShaderModule(dev, vert_mod, nullptr);
		}
	}

	void app::run()
	{
		initialize();
		app_loop();
		cleanup();
	}

	void app::initialize()
	{
		glfw::glfw_initialization(RES_WIDTH, RES_HEIGHT);
		vulkan::create_instance();
		vulkan::debug::setup_debug_messenger();
		vulkan::KHR::create_surface();
		vulkan::pick_physical_device();
		vulkan::create_logical_device();
		vulkan::KHR::create_swap_chain();
		vulkan::create_image_views();
		vulkan::create_render_pass();
		vulkan::create_graphics_pipeline();
	}

	void app::app_loop()
	{
		while (!glfwWindowShouldClose(glfw::window))
		{
			glfwPollEvents();
		}
	}

	void app::cleanup()
	{
		vkDestroyRenderPass(vulkan::dev, vulkan::render_pass, nullptr);

		vkDestroyPipeline(vulkan::dev, vulkan::graphics_pipeline, nullptr);

		vkDestroyPipelineLayout(vulkan::dev, vulkan::pipeline_layout, nullptr);

		for (auto iv : vulkan::sc_image_views)
		{
			vkDestroyImageView(vulkan::dev, iv, nullptr);
		}

		vkDestroySwapchainKHR(vulkan::dev, vulkan::KHR::swap_chain, nullptr);

		vkDestroyDevice(vulkan::dev, nullptr);

		if (vulkan::debug::enable_validation_layers)
		{
			vulkan::debug::DestroyDebugUtilsMessengerEXT(vulkan::instance, vulkan::debug::debug_messenger, nullptr);
		}

		vkDestroySurfaceKHR(vulkan::instance, vulkan::surface, nullptr);

		vkDestroyInstance(vulkan::instance, nullptr);

		if (glfw::window)
		{
			glfwDestroyWindow(glfw::window);
		}

		glfwTerminate();
	}
}