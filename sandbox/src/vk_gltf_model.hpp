#ifndef GLFT_MODEL_HPP
#define GLTF_MODEL_HPP

#include "vk_vertex.hpp"
#include "math.hpp"

namespace vk
{
	namespace gltf
	{
		struct vertex
		{
			glm::vec3 position;
			glm::vec3 normal;
			glm::vec2 uv;
			glm::vec4 color;
			glm::vec4 joint0;
			glm::vec4 weight0;
			glm::vec4 tangent;
		};

		struct model
		{
			static vertex_descripton	vtx_description;

			static void define_model_components_description()
			{
				vertex_info::component_description pos;
				pos.component_type = vertex_info::component::position;
				pos.binding = 0;
				pos.location = 0;
				pos.format = VK_FORMAT_R32G32B32_SFLOAT;
				pos.offset = offsetof(vertex, position);
				vtx_description.components.push_back(pos);

				vertex_info::component_description nrm;
				nrm.component_type = vertex_info::component::normal;
				nrm.binding = 0;
				nrm.location = 1;
				nrm.format = VK_FORMAT_R32G32B32_SFLOAT;
				nrm.offset = offsetof(vertex, normal);
				vtx_description.components.push_back(nrm);

				vertex_info::component_description tex;
				tex.component_type = vertex_info::component::uv;
				tex.binding = 0;
				tex.location = 2;
				tex.format = VK_FORMAT_R32G32_SFLOAT;
				tex.offset = offsetof(vertex, uv);
				vtx_description.components.push_back(tex);

				vertex_info::component_description clr;
				clr.component_type = vertex_info::component::color;
				clr.binding = 0;
				clr.location = 3;
				clr.format = VK_FORMAT_R32G32B32A32_SFLOAT;
				clr.offset = offsetof(vertex, color);
				vtx_description.components.push_back(clr);

				vertex_info::component_description joint;
				joint.component_type = vertex_info::component::joint;
				joint.binding = 0;
				joint.location = 4;
				joint.format = VK_FORMAT_R32G32B32A32_SFLOAT;
				joint.offset = offsetof(vertex, joint0);
				vtx_description.components.push_back(joint);

				vertex_info::component_description weight;
				weight.component_type = vertex_info::component::weight;
				weight.binding = 0;
				weight.location = 5;
				weight.format = VK_FORMAT_R32G32B32A32_SFLOAT;
				weight.offset = offsetof(vertex, weight0);
				vtx_description.components.push_back(weight);

				vertex_info::component_description tan;
				tan.component_type = vertex_info::component::tangent;
				tan.binding = 0;
				tan.location = 6;
				tan.format = VK_FORMAT_R32G32B32A32_SFLOAT;
				tan.offset = offsetof(vertex, tangent);
				vtx_description.components.push_back(tan);
			}
			static void define_model_vertex_binding_description(const VkVertexInputRate input_rate)
			{
				vtx_description.binding = 0;
				vtx_description.stride = sizeof(vertex);
				vtx_description.input_rate = input_rate;
			}
		};



		
	}
}


#endif
