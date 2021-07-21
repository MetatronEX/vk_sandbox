#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <vulkan/vulkan.h>

namespace sandbox
{
	namespace vk
	{
		struct image
		{
			VkImage			I;
			VkDeviceMemory DM;
		};
	}
}
#endif // !IMAGE_HPP

