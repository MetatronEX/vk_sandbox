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

		VkQueue						graphics_queue;
		VkQueue						present_queue;

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
		}

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


		void create_instance()
		{
			if (debug::enable_validation_layers && !debug::check_validation_layer_support())
			{
				throw std::runtime_error("Validation layer requested, but not available!");
			}

			VkApplicationInfo app_info{  };
			app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			app_info.pApplicationName = sandbox_app::app_name;
			app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
			app_info.pEngineName = "sandbox engine";
			app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
			app_info.apiVersion = VK_MAKE_VERSION(1, 2, 0);

			VkInstanceCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			create_info.pApplicationInfo = &app_info;
			
			auto get_required_extensions = [&create_info]()
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

		swap_chain_support query_sw_support(VkPhysicalDevice dev)
		{
			swap_chain_support details{};

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
				swap_chain_support sw_support = query_sw_support(dev);
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

		VkDevice create_logical_device()
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
				
	}

	void sandbox_app::run()
	{
		initialize();
		app_loop();
		cleanup();
	}

	void sandbox_app::initialize()
	{
		glfw::glfw_initialization(RES_WIDTH, RES_HEIGHT);
		vulkan::create_instance();
		vulkan::debug::setup_debug_messenger();
		vulkan::KHR::create_surface();
		vulkan::pick_physical_device();
		vulkan::create_logical_device();
	}

	void sandbox_app::app_loop()
	{
		while (!glfwWindowShouldClose(glfw::window))
		{
			glfwPollEvents();
		}
	}

	void sandbox_app::cleanup()
	{
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