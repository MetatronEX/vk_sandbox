#ifndef VK_VERTEX_HPP
#define VK_VERTEX_HPP

#include "vk.hpp"
#include "vertex_info.hpp"

namespace vk
{
	struct vertex_descripton
	{
		uint32_t	      binding;
		size_t			  stride;
		VkVertexInputRate input_rate;

		std::vector<vertex_info::component_description>	components;

		std::vector<VkVertexInputAttributeDescription> query_attribute_descriptions()
		{
			std::vector<VkVertexInputAttributeDescription> descriptions(components.size());

			for (const auto& c : components)
			{
				VkVertexInputAttributeDescription description{};
				description.binding = c.binding;
				description.location = c.location;
				description.format = static_cast<VkFormat>(c.format);
				description.offset = c.offset;

				descriptions.push_back(description);
			}

			return descriptions;
		}

		VkVertexInputBindingDescription query_binding_descriptions()
		{
			return info::vertex_input_binding_description(binding, stride, input_rate);
		}
	};
}

#endif
