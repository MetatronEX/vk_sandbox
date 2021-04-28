#include "vk_sandbox.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm.hpp>

#include <memory>
#include <set>
#include <cstring>
#include <algorithm> 
#include <fstream>

#define OP_SUCCESS(X) VK_SUCCESS == X

namespace sandbox
{
	namespace glfw
	{
		GLFWwindow* window;

		void framebuffer_resize_callback(GLFWwindow* win, int w, int h) 
		{
			vulkan::fb_resized = true;
		}

		void glfw_initialization(const unsigned res_width, const unsigned res_height)
		{
			glfwInit();

			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
			glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

			window = glfwCreateWindow(res_width, res_height, "sandbox", nullptr, nullptr);
			
			glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);
		}

		void destroy_resources()
		{
			if (window)
			{
				glfwDestroyWindow(window);
			}

			glfwTerminate();
		}
	}
	
	namespace vulkan
	{
		constexpr unsigned max_frames_in_flight = 2;

		const std::vector<const char*> dev_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		const std::vector<vertex> vertices =
		{
			{{0.f,-.5f},{1.f,0.f,0.f}},
			{{.5f, .5f},{0.f,1.f,0.f}},
			{{-.5f,.5f},{0.f,0.f,1.f}}
		};

		VkInstance						instance;
		VkPhysicalDevice				pd{ VK_NULL_HANDLE };
		VkDevice						dev;
		VkSurfaceKHR					surface;

		VkRenderPass					render_pass;
		VkPipelineLayout				pipeline_layout;

		VkQueue							graphics_queue;
		VkQueue							present_queue;

		VkPipeline						graphics_pipeline;

		VkFormat						sc_img_fmt;
		VkExtent2D						sc_extent;

		VkCommandPool					cmd_pool;

		VkBuffer						vertex_buffer;
		VkDeviceMemory					vtx_buffer_mem;

		size_t							curr_frame{ 0 };

		std::vector<VkImage>			sc_images;
		std::vector<VkImageView>		sc_image_views;

		std::vector<VkFramebuffer>		sc_framebuffers;

		std::vector<VkCommandBuffer>	cmd_buffers;

		std::vector<VkSemaphore>		image_semaphores;
		std::vector<VkSemaphore>		rp_semaphores;

		std::vector<VkFence>			in_flight_fences;
		std::vector<VkFence>			images_in_flight;

		bool							fb_resized{ false };

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

			/*
			* We could recreate the command pool from scratch, but that is rather wasteful.
			Instead I've opted to clean up the existing command buffers with the vkFreeCommandBuffers
			function. This way we can reuse the existing pool to allocate the new command buffers.
			*/
			void clean_swap_chain()
			{
				for (auto fb : sc_framebuffers)
				{
					vkDestroyFramebuffer(dev, fb, nullptr);
				}

				vkFreeCommandBuffers(dev, cmd_pool, static_cast<uint32_t>(cmd_buffers.size()), cmd_buffers.data());

				vkDestroyRenderPass(dev, render_pass, nullptr);

				vkDestroyPipeline(dev, graphics_pipeline, nullptr);

				vkDestroyPipelineLayout(dev, pipeline_layout, nullptr);

				for (auto iv : sc_image_views)
				{
					vkDestroyImageView(dev, iv, nullptr);
				}

				vkDestroySwapchainKHR(dev, swap_chain, nullptr);
			}

			void recreate_swap_chain()
			{
				int w = 0, h = 0;

				glfwGetFramebufferSize(glfw::window, &w, &h);

				while (0 == w || 0 == h)
				{
					glfwGetFramebufferSize(glfw::window, &w, &h);
					glfwWaitEvents();
				}

				vkDeviceWaitIdle(dev);

				clean_swap_chain();

				create_swap_chain();
				create_image_views();
				create_render_pass();
				create_graphics_pipeline();
				create_framebuffers();
				create_cmd_buffers();
			}

			void draw_frame()
			{
				vkWaitForFences(dev, 1, &in_flight_fences[curr_frame], VK_TRUE, std::numeric_limits<uint64_t>::max());

				/*
				The first thing we need to do in the drawFrame function is acquire an image from the swap chain.
				Recall that the swap chain is an extension feature, so we must use a function with the vk*KHR naming convention.

				The first two parameters of vkAcquireNextImageKHR are the logical device and the swap chain from which we wish to acquire
				an image. The third parameter specifies a timeout in nanoseconds for an image to become available. Using the maximum value
				of a 64 bit unsigned integer disables the timeout.

				The next two parameters specify synchronization objects that are to be signaled when the presentation engine is finished
				using the image. That's the point in time where we can start drawing to it. It is possible to specify a semaphore, fence
				or both.

				The last parameter specifies a variable to output the index of the swap chain image that has become available. The index
				refers to the VkImage in our swapChainImages array. We're going to use that index to pick the right command buffer.
				*/
				unsigned image_index;

				auto result = vkAcquireNextImageKHR(dev, swap_chain, std::numeric_limits<uint64_t>::max(),
					image_semaphores[curr_frame], VK_NULL_HANDLE, &image_index);

				/*
				* Need to figure out when swap chain recreation is necessary and call our new recreateSwapChain function.
				Vulkan will usually just tell us that the swap chain is no longer adequate during presentation. The vkAcquireNextImageKHR
				and vkQueuePresentKHR functions can return the following special values to indicate this.

					- VK_ERROR_OUT_OF_DATE_KHR: The swap chain has become incompatible with the surface and can no longer be used for rendering.
					Usually happens after a window resize.

					- VK_SUBOPTIMAL_KHR: The swap chain can still be used to successfully present to the surface, but the surface properties are
					no longer matched exactly.
				*/
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

				// check if previous frame is using this image -> there is a fence to wait on it
				if (VK_NULL_HANDLE != images_in_flight[image_index])
				{
					vkWaitForFences(dev, 1, &images_in_flight[image_index], VK_TRUE,
						std::numeric_limits<uint64_t>::max());
				}

				// mark the image as now being in use by this frame
				images_in_flight[image_index] = in_flight_fences[curr_frame];

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
				/*
				The first two parameters specify which semaphores to wait on before presentation can happen, just like VkSubmitInfo.
				*/
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

			/*
			Note:

			Transfer queue

			The buffer copy command requires a queue family that supports transfer operations, 
			which is indicated using VK_QUEUE_TRANSFER_BIT. The good news is that any queue family 
			with VK_QUEUE_GRAPHICS_BIT or VK_QUEUE_COMPUTE_BIT capabilities already implicitly support 
			VK_QUEUE_TRANSFER_BIT operations. The implementation is not required to explicitly list it 
			in queueFlags in those cases.
			
			If you like a challenge, then you can still try to use a different queue family specifically 
			for transfer operations. It will require you to make the following modifications to your program:
			
				-	Modify QueueFamilyIndices and findQueueFamilies to explicitly look for a queue family with the 
				VK_QUEUE_TRANSFER_BIT bit, but not the VK_QUEUE_GRAPHICS_BIT.
				
				-	Modify createLogicalDevice to request a handle to the transfer queue
				
				-	Create a second command pool for command buffers that are submitted on the transfer queue family
				
				-	Change the sharingMode of resources to be VK_SHARING_MODE_CONCURRENT and specify both the graphics 
				and transfer queue families
				
				-	Submit any transfer commands like vkCmdCopyBuffer (which we'll be using in this chapter) to the 
				transfer queue instead of the graphics queue
			
			It's a bit of work, but it'll teach you a lot about how resources are shared between queue families.
			*/
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
			* There are two built-in dependencies that take care of the transition at the start of the
			render pass and at the end of the render pass, but the former does not occur at the right time.
			It assumes that the transition occurs at the start of the pipeline, but we haven't acquired the
			image yet at that point! There are two ways to deal with this problem. We could change the waitStages
			for the imageAvailableSemaphore to VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT to ensure that the render passes
			don't begin until the image is available, or we can make the render pass wait for the
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT stage.

			The first two fields specify the indices of the dependency and the dependent subpass. The special value 
			VK_SUBPASS_EXTERNAL refers to the implicit subpass before or after the render pass depending on whether 
			it is specified in srcSubpass or dstSubpass.

			The index 0 refers to our subpass, which is the first and only one. The dstSubpass must always be higher 
			than srcSubpass to prevent cycles in the dependency graph (unless one of the subpasses is VK_SUBPASS_EXTERNAL).
			*/
			VkSubpassDependency2 spd{};
			spd.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
			spd.srcSubpass = VK_SUBPASS_EXTERNAL;
			spd.dstSubpass = 0;
			/*
			* The next two fields specify the operations to wait on and the stages in which these operations occur. We need 
			to wait for the swap chain to finish reading from the image before we can access it. This can be accomplished by 
			waiting on the color attachment output stage itself.
			*/
			spd.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			spd.srcAccessMask = 0;
			/*
			* The operations that should wait on this are in the color attachment stage and involve the writing of the color 
			attachment. These settings will prevent the transition from happening until it's actually necessary (and allowed): 
			when we want to start writing colors to it.
			*/
			spd.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			spd.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

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
			rp_info.dependencyCount = 1;
			rp_info.pDependencies = &spd;

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
			auto binding_description = vertex::get_binding_desc();
			auto attr_descriptions = vertex::get_attr_desc();

			VkPipelineVertexInputStateCreateInfo vtx_input_info{};
			vtx_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vtx_input_info.vertexBindingDescriptionCount = 1;
			vtx_input_info.pVertexBindingDescriptions = &binding_description;
			vtx_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attr_descriptions.size());
			vtx_input_info.pVertexAttributeDescriptions = attr_descriptions.data();

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

		void create_framebuffers()
		{
			sc_framebuffers.resize(sc_image_views.size());

			for (size_t i = 0; i < sc_image_views.size(); i++)
			{
				VkImageView attachments[] =
				{
					sc_image_views[i]
				};

				VkFramebufferCreateInfo fb_info{};
				fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				/*
				We first need to specify with which renderPass the framebuffer needs to be compatible. You can only use a 
				framebuffer with the render passes that it is compatible with, which roughly means that they use the same 
				number and type of attachments.
				*/
				fb_info.renderPass = render_pass;
				/*
				The attachmentCount and pAttachments parameters specify the VkImageView objects that should be bound to the 
				respective attachment descriptions in the render pass pAttachment array.
				*/
				fb_info.attachmentCount = 1;
				fb_info.pAttachments = attachments;
				fb_info.width = sc_extent.width;
				fb_info.height = sc_extent.height;
				fb_info.layers = 1;

				if (!OP_SUCCESS(vkCreateFramebuffer(dev, &fb_info, nullptr, &sc_framebuffers[i])))
				{
					throw std::runtime_error("Failed to create framebuffer!");
				}
			}
		}

		void create_cmd_pool()
		{
			queue_family_indices qfi = find_queue_families(pd);

			/*
			Command buffers are executed by submitting them on one of the device queues, like the graphics and 
			presentation queues we retrieved. Each command pool can only allocate command buffers that are 
			submitted on a single type of queue. We're going to record commands for drawing, which is why we've 
			chosen the graphics queue family.


			There are two possible flags for command pools:

				- VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: Hint that command buffers are rerecorded with new commands 
				very often (may change memory allocation behavior)
				
				- VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow command buffers to be rerecorded individually, 
				without this flag they all have to be reset together

			*/
			VkCommandPoolCreateInfo pool_info{};
			pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			pool_info.queueFamilyIndex = qfi.graphics_family.value();
			pool_info.flags = 0;

			if (!OP_SUCCESS(vkCreateCommandPool(dev, &pool_info, nullptr, &cmd_pool)))
			{
				throw std::runtime_error("Failed to create command pool!");
			}
		}

		// to deprecate
		/*
		* Graphics cards can offer different types of memory to allocate from. Each type of memory varies in
		terms of allowed operations and performance characteristics. We need to combine the requirements of
		the buffer and our own application requirements to find the right type of memory to use.
		*/
		uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags props)
		{
			VkPhysicalDeviceMemoryProperties2 mem_props{};
			mem_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;

			vkGetPhysicalDeviceMemoryProperties2(pd, &mem_props);

			/*
			* The VkPhysicalDeviceMemoryProperties structure has two arrays memoryTypes and memoryHeaps.
			Memory heaps are distinct memory resources like dedicated VRAM and swap space in RAM for when VRAM
			runs out. The different types of memory exist within these heaps.

			we're not just interested in a memory type that is suitable for the vertex buffer. We also need to
			be able to write our vertex data to that memory. The memoryTypes array consists of VkMemoryType
			structs that specify the heap and properties of each type of memory. The properties define special
			features of the memory, like being able to map it so we can write to it from the CPU. This property
			is indicated with VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, but we also need to use the
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT property.
			*/
			for (uint32_t i = 0; i < mem_props.memoryProperties.memoryTypeCount; i++)
			{
				/*
				* We may have more than one desirable property, so we should check if the result of the bitwise
				AND is not just non-zero, but equal to the desired properties bit field. If there is a memory
				type suitable for the buffer that also has all of the properties we need, then we return its
				index, otherwise we throw an exception.
				*/
				if ((type_filter & (1 << i)) &&
					(mem_props.memoryProperties.memoryTypes[i].propertyFlags & props) == props)
					return i;
			}

			throw std::runtime_error("Suitable memory type not found!");
		}

		void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, 
			VkMemoryPropertyFlags props,
			VkBuffer& buffer, VkDeviceMemory& dev_mem)
		{
			VkBufferCreateInfo b_info{};
			b_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			b_info.size = size;
			/*
			* usage indicates which purpose the data in the buffer is going to be used.
			It is possible to specify multiple purposes using a bitwise or.
			*/
			b_info.usage = usage;
			/*
			* Just like the images in the swap chain, buffers can also be owned by a specific
			queue family or be shared between multiple at the same time. The buffer will only
			be used from the graphics queue, so we can stick to exclusive access.

			The flags parameter is used to configure sparse buffer memory, which is not relevant
			right now. We'll leave it at the default value of 0.
			*/
			b_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			if (!OP_SUCCESS(vkCreateBuffer(dev,  &b_info, nullptr, &buffer)))
			{
				throw std::runtime_error("Buffer creation failed!");
			}

			/*
			* The buffer has been created, but it doesn't actually have any memory assigned to it yet.
			The first step of allocating memory for the buffer is to query its memory requirements
			using the aptly named vkGetBufferMemoryRequirements function.

			The VkMemoryRequirements struct has three fields:

				- size: The size of the required amount of memory in bytes, may differ from bufferInfo.size.

				- alignment: The offset in bytes where the buffer begins in the allocated region of memory,
				depends on bufferInfo.usage and bufferInfo.flags.

				- memoryTypeBits: Bit field of the memory types that are suitable for the buffer.
			*/
			VkMemoryRequirements2 mem_req{};
			mem_req.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

			VkBufferMemoryRequirementsInfo2 bmr_info{};
			bmr_info.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2;
			bmr_info.buffer = buffer;

			vkGetBufferMemoryRequirements2(dev, &bmr_info, &mem_req);

			/*
			* Graphics cards can offer different types of memory to allocate from. Each type of memory varies in
			terms of allowed operations and performance characteristics. We need to combine the requirements of
			the buffer and our own application requirements to find the right type of memory to use.
			*/
			auto find_mem_type = [](uint32_t type_filter, VkMemoryPropertyFlags props)
			{
				VkPhysicalDeviceMemoryProperties2 mem_props{};
				mem_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;

				vkGetPhysicalDeviceMemoryProperties2(pd, &mem_props);

				/*
				* The VkPhysicalDeviceMemoryProperties structure has two arrays memoryTypes and memoryHeaps.
				Memory heaps are distinct memory resources like dedicated VRAM and swap space in RAM for when VRAM
				runs out. The different types of memory exist within these heaps.

				we're not just interested in a memory type that is suitable for the vertex buffer. We also need to
				be able to write our vertex data to that memory. The memoryTypes array consists of VkMemoryType
				structs that specify the heap and properties of each type of memory. The properties define special
				features of the memory, like being able to map it so we can write to it from the CPU. This property
				is indicated with VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, but we also need to use the
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT property.
				*/
				for (uint32_t i = 0; i < mem_props.memoryProperties.memoryTypeCount; i++)
				{
					/*
					* We may have more than one desirable property, so we should check if the result of the bitwise
					AND is not just non-zero, but equal to the desired properties bit field. If there is a memory
					type suitable for the buffer that also has all of the properties we need, then we return its
					index, otherwise we throw an exception.
					*/
					if ((type_filter & (1 << i)) &&
						(mem_props.memoryProperties.memoryTypes[i].propertyFlags & props) == props)
						return i;
				}

				throw std::runtime_error("Suitable memory type not found!");
			};

			// memory allocation
			VkMemoryAllocateInfo ma_info{};
			ma_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			ma_info.allocationSize = mem_req.memoryRequirements.size;
			ma_info.memoryTypeIndex = find_mem_type(mem_req.memoryRequirements.memoryTypeBits,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			/*
			It should be noted that in a real world application, you're not supposed to actually call vkAllocateMemory 
			for every individual buffer. The maximum number of simultaneous memory allocations is limited by the 
			maxMemoryAllocationCount physical device limit, which may be as low as 4096 even on high end hardware 
			like an NVIDIA GTX 1080. The right way to allocate memory for a large number of objects at the same time is 
			to create a custom allocator that splits up a single allocation among many different objects by using the 
			offset parameters that we've seen in many functions.

			You can either implement such an allocator yourself, or use the VulkanMemoryAllocator library provided by 
			the GPUOpen initiative. 
			*/
			if (!OP_SUCCESS(vkAllocateMemory(dev, &ma_info, nullptr, &dev_mem)))
			{
				throw std::runtime_error("vertex buffer memory allocation failed!");
			}

			VkBindBufferMemoryInfo bbm_info{};
			bbm_info.sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO;
			bbm_info.buffer = buffer;
			bbm_info.memory = dev_mem;
			bbm_info.memoryOffset = 0;

			vkBindBufferMemory2(dev, 1, &bbm_info);
		}

		void copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size)
		{
			/*
			* Memory transfer operations are executed using command buffers, just like drawing commands. 
			Therefore we must first allocate a temporary command buffer. You may wish to create a separate 
			command pool for these kinds of short-lived buffers, because the implementation may be able to 
			apply memory allocation optimizations. You should use the VK_COMMAND_POOL_CREATE_TRANSIENT_BIT 
			flag during command pool generation in that case.
			*/

			VkCommandBufferAllocateInfo cba_info{};
			cba_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cba_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			cba_info.commandPool = cmd_pool;
			cba_info.commandBufferCount = 1;

			VkCommandBuffer cmd_buffer;
			vkAllocateCommandBuffers(dev, &cba_info, &cmd_buffer);

			VkCommandBufferBeginInfo begin{};
			begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			/*
			going to use the command buffer once and wait with returning from the function until the copy operation 
			has finished executing. It's good practice to tell the driver about our intent using 
			VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT.
			*/
			begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			vkBeginCommandBuffer(cmd_buffer, &begin);

			/*
			Contents of buffers are transferred using the vkCmdCopyBuffer command. It takes the source and destination 
			buffers as arguments, and an array of regions to copy. The regions are defined in VkBufferCopy structs and 
			consist of a source buffer offset, destination buffer offset and size. It is not possible to specify 
			VK_WHOLE_SIZE here, unlike the vkMapMemory command.
			*/
			VkBufferCopy copy_region{};
			copy_region.srcOffset = 0;
			copy_region.dstOffset = 0;
			copy_region.size = size;

			vkCmdCopyBuffer(cmd_buffer, src, dst, 1, &copy_region);

			vkEndCommandBuffer(cmd_buffer);

			/*
			Unlike the draw commands, there are no events we need to wait on this time. We just want to execute the transfer 
			on the buffers immediately. There are again two possible ways to wait on this transfer to complete. We could use 
			a fence and wait with vkWaitForFences, or simply wait for the transfer queue to become idle with vkQueueWaitIdle. 
			A fence would allow you to schedule multiple transfers simultaneously and wait for all of them complete, instead 
			of executing one at a time. That may give the driver more opportunities to optimize.
			*/
			VkSubmitInfo submit{};
			submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit.commandBufferCount = 1;
			submit.pCommandBuffers = &cmd_buffer;
			
			vkQueueSubmit(graphics_queue, 1, &submit, VK_NULL_HANDLE);
			vkQueueWaitIdle(graphics_queue);

			vkFreeCommandBuffers(dev, cmd_pool, 1, &cmd_buffer);
		}

		void create_vertex_buffer()
		{
			VkDeviceSize buffer_size = sizeof(vertex) * vertices.size();
			
			/*
				using a new stagingBuffer with stagingBufferMemory for mapping and copying the vertex data.

				use two buffer usage flags:

					- VK_BUFFER_USAGE_TRANSFER_SRC_BIT: Buffer can be used as source in a memory transfer operation.
					- VK_BUFFER_USAGE_TRANSFER_DST_BIT: Buffer can be used as destination in a memory transfer operation.
			*/
			VkBuffer			staging_buffer;
			VkDeviceMemory		staging_buffer_memory;

			/*
				use a host visible buffer as temporary buffer and use a device local one as actual vertex buffer.
			*/
			create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				staging_buffer, staging_buffer_memory);

			/*
			* memcpy the vertex data to the mapped memory and unmap it again using vkUnmapMemory. Unfortunately
			the driver may not immediately copy the data into the buffer memory, for example because of caching.
			It is also possible that writes to the buffer are not visible in the mapped memory yet. There are
			two ways to deal with that problem:

				- Use a memory heap that is host coherent, indicated with VK_MEMORY_PROPERTY_HOST_COHERENT_BIT

				- Call vkFlushMappedMemoryRanges after writing to the mapped memory, and call
				vkInvalidateMappedMemoryRanges before reading from the mapped memory

			We went for the first approach, which ensures that the mapped memory always matches the contents of
			the allocated memory. Do keep in mind that this may lead to slightly worse performance than explicit
			flushing.
			*/
			/*
				vertexBuffer is now allocated from a memory type that is device local, which generally means that we're
				not able to use vkMapMemory.

				we can copy data from the stagingBuffer to the vertexBuffer.

				We have to indicate that we intend to do that by specifying the transfer source flag for the stagingBuffer
				and the transfer destination flag for the vertexBuffer, along with the vertex buffer usage flag.
			*/
			void* data;
			vkMapMemory(dev, staging_buffer_memory, 0, buffer_size, 0, &data);
			memcpy(data, vertices.data(), static_cast<size_t>(buffer_size));
			vkUnmapMemory(dev, staging_buffer_memory);

			create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_buffer, vtx_buffer_mem);

			copy_buffer(staging_buffer, vertex_buffer, buffer_size);

			vkDestroyBuffer(dev, staging_buffer, nullptr);
			vkFreeMemory(dev, staging_buffer_memory, nullptr);
		}
				
		void create_cmd_buffers()
		{
			cmd_buffers.resize(sc_framebuffers.size());

			VkCommandBufferAllocateInfo alloc_info{};
			alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			alloc_info.commandPool = cmd_pool;
			/*
			The level parameter specifies if the allocated command buffers are primary or secondary command buffers.

				- VK_COMMAND_BUFFER_LEVEL_PRIMARY: Can be submitted to a queue for execution, but cannot be called from other command buffers.
				- VK_COMMAND_BUFFER_LEVEL_SECONDARY: Cannot be submitted directly, but can be called from primary command buffers.
			*/
			alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			alloc_info.commandBufferCount = static_cast<unsigned>(cmd_buffers.size());

			if (!OP_SUCCESS(vkAllocateCommandBuffers(dev,&alloc_info, cmd_buffers.data())))
			{
				throw std::runtime_error("Failed to allocate command buffers!");
			}

			// command buffer recording
			for (size_t i = 0; i < cmd_buffers.size(); i++)
			{
				VkCommandBufferBeginInfo begin_info{};
				begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				/*
				The flags parameter specifies how we're going to use the command buffer. The following values are available:

					- VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: The command buffer will be rerecorded right after executing it once.
					- VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT: This is a secondary command buffer that will be entirely within a single render pass.
					- VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT: The command buffer can be resubmitted while it is also already pending execution.
				
				None of these flags are applicable for us right now.
				*/
				begin_info.flags = 0;
				/*
				The pInheritanceInfo parameter is only relevant for secondary command buffers. It specifies which state to 
				inherit from the calling primary command buffers.
				*/
				begin_info.pInheritanceInfo = nullptr;

				if (!OP_SUCCESS(vkBeginCommandBuffer(cmd_buffers[i], &begin_info)))
				{
					throw std::runtime_error("Command buffer recording failed!");
				}

				/* Note: If the command buffer was already recorded once, then a call to vkBeginCommandBuffer will implicitly reset it. 
				 It's not possible to append commands to a buffer at a later time. */

				VkRenderPassBeginInfo rpi{};
				rpi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				/*
				The first parameters are the render pass itself and the attachments to bind. We created a framebuffer for each swap chain 
				image that specifies it as color attachment.
				*/
				rpi.renderPass = render_pass;
				rpi.framebuffer = sc_framebuffers[i];
				/*
				* The next two parameters define the size of the render area. The render area defines where shader loads and stores will 
				take place. The pixels outside this region will have undefined values. It should match the size of the attachments for 
				best performance.
				*/
				rpi.renderArea.offset = { 0,0 };
				rpi.renderArea.extent = sc_extent;
				/*
				The last two parameters define the clear values to use for VK_ATTACHMENT_LOAD_OP_CLEAR, which we used as load operation 
				for the color attachment.
				*/
				VkClearValue clear_color = { 0.f,0.f,0.f,1.f };
				rpi.clearValueCount = 1;
				rpi.pClearValues = &clear_color;

				VkSubpassBeginInfo spi{};
				spi.sType = VK_STRUCTURE_TYPE_SUBPASS_BEGIN_INFO;
				spi.contents = VK_SUBPASS_CONTENTS_INLINE;
				
				/*
				The render pass can now begin. All of the functions that record commands can be recognized by their vkCmd prefix. 
				They all return void, so there will be no error handling until we've finished recording.

				The final parameter controls how the drawing commands within the render pass will be provided. It can have one of two values:

					- VK_SUBPASS_CONTENTS_INLINE: The render pass commands will be embedded in the primary command buffer itself and no 
					secondary command buffers will be executed.
					
					- VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: The render pass commands will be executed from secondary command buffers.
				*/
				vkCmdBeginRenderPass2(cmd_buffers[i], &rpi, &spi);

				vkCmdBindPipeline(cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

				VkBuffer vtx_buffers[] = { vertex_buffer };
				VkDeviceSize offsets[] = { 0 };

				vkCmdBindVertexBuffers(cmd_buffers[i], 0, 1, vtx_buffers, offsets);

				vkCmdDraw(cmd_buffers[i], static_cast<uint32_t>(vertices.size()), 1, 0, 0);

				vkCmdEndRenderPass(cmd_buffers[i]);

				if (!OP_SUCCESS(vkEndCommandBuffer(cmd_buffers[i])))
				{
					throw std::runtime_error("Command buffer recording failed!");
				}
			}
		}

		void create_syncs()
		{
			image_semaphores.resize(max_frames_in_flight);
			rp_semaphores.resize(max_frames_in_flight);
			in_flight_fences.resize(max_frames_in_flight);
			images_in_flight.resize(sc_images.size(), VK_NULL_HANDLE);

			VkSemaphoreCreateInfo sem_info{};
			sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			VkFenceCreateInfo fence_info{};
			fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			for (size_t i = 0; i < max_frames_in_flight; i++)
			{
				if (!OP_SUCCESS(vkCreateSemaphore(dev, &sem_info, nullptr, &image_semaphores[i])) ||
					!OP_SUCCESS(vkCreateSemaphore(dev, &sem_info, nullptr, &rp_semaphores[i])) ||
					!OP_SUCCESS(vkCreateFence(dev, &fence_info, nullptr, &in_flight_fences[i])))
				{
					throw std::runtime_error("Sync objects creation failed!");
				}
			}
			
		}
		
		void wait_for_device_completion()
		{
			vkDeviceWaitIdle(dev);
		}

		void destroy_resources()
		{
			for (size_t i = 0; i < vulkan::max_frames_in_flight; i++)
			{
				vkDestroySemaphore(dev, image_semaphores[i], nullptr);
				vkDestroySemaphore(dev, rp_semaphores[i], nullptr);
				vkDestroyFence(dev, in_flight_fences[i], nullptr);
			}

			KHR::clean_swap_chain();

			vkDestroyBuffer(dev, vertex_buffer, nullptr);
			vkFreeMemory(dev, vtx_buffer_mem, nullptr);

			vkDestroyCommandPool(dev, cmd_pool, nullptr);
			vkDestroyDevice(dev, nullptr);

			if (debug::enable_validation_layers)
			{
				debug::DestroyDebugUtilsMessengerEXT(vulkan::instance, vulkan::debug::debug_messenger, nullptr);
			}

			vkDestroySurfaceKHR(instance, surface, nullptr);

			vkDestroyInstance(instance, nullptr);
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
		vulkan::create_framebuffers();
		vulkan::create_cmd_pool();
		vulkan::create_vertex_buffer();
		vulkan::create_cmd_buffers();
		vulkan::create_syncs();
	}

	void app::app_loop()
	{
		while (!glfwWindowShouldClose(glfw::window))
		{
			glfwPollEvents();
			vulkan::KHR::draw_frame();
		}

		vulkan::wait_for_device_completion();
	}

	void app::cleanup()
	{
		vulkan::destroy_resources();
		glfw::destroy_resources();
	}
}