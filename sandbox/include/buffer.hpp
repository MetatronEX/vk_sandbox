#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <vulkan/vulkan.h>

namespace sandbox
{
	namespace vk
	{
		struct buffer
		{
			VkBuffer		B;
			VkDeviceMemory DM;
		};
	}
}

#endif // !BUFFER_HPP

