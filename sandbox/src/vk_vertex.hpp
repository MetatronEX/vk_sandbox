#ifndef VK_VERTEX_HPP
#define VK_VERTEX_HPP

#include "vk.hpp"
#include "vertex_info.hpp"

namespace vk
{
	struct vertex_descripton
	{
		std::vector<vertex_info::component>	components;

		VkVertexInputBindingDescription query_bind_description()
		{
			VkVertexInputBindingDescription description;
		}
	};
}

#endif
