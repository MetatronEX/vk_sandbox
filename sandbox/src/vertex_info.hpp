#ifndef MODEL_INFO_HPP
#define MODEL_INFO_HPP

#include <vector>
#include <cstddef> // for offsetof

namespace vertex_info
{
	enum class component
	{
		position,
		normal,
		uv,
		color,
		tangent,
		joint,
		weight
	};

	struct component_description
	{
		component	component_type;
		uint32_t	binding;
		uint32_t	location;
		uint32_t	format;
		uint32_t	offset;
	};
}

#endif
