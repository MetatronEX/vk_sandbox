#include "deferred_shading.hpp"

#define SHADOW_DIMENSIONS 2048
#define SHADOWMAP_FORMAT  VK_FORMAT_D32_SFLOAT_S8_UINT
#define FRAMEBUFFER_DIMENSIONS 2048

namespace vk
{
	namespace policy
	{
		void deferred_shading::setup_queried_features()
		{
			if (gpu->device_features.features.geometryShader)
				gpu->device_features.features.geometryShader = VK_TRUE;
			else
				fatal_exit("Selected GPU does not support geometry shaders", VK_ERROR_FEATURE_NOT_PRESENT);

			if (gpu->device_features.features.samplerAnisotropy)
				gpu->enabled_features.features.samplerAnisotropy = VK_TRUE;

			if (gpu->device_features.features.textureCompressionBC)
				gpu->enabled_features.features.textureCompressionBC = VK_TRUE;
			else if (gpu->device_features.features.textureCompressionASTC_LDR)
				gpu->device_features.features.textureCompressionASTC_LDR = VK_TRUE;
		}

		void deferred_shading::prime()
		{

		}

		void deferred_shading::setup_depth_stencil()
		{
			gpu->setup_default_depth_stencil();
		}

		void deferred_shading::setup_drawcommands()
		{
			auto CBI = info::command_buffer_begin_info();

			std::array<VkClearValue, 2> clear_values;
			clear_values[0].color = { {0.f,0.f,0.f,0.f} };
			clear_values[1].depthStencil = { 1.f, 0 };

			auto RPB = info::renderpass_begin_info();
			RPB.renderPass = gpu->renderpass;
			RPB.renderArea.offset.x = 0;
			RPB.renderArea.offset.y = 0;
			RPB.renderArea.extent.width = gpu->width;
			RPB.renderArea.extent.height = gpu->height;
			RPB.clearValueCount = 2;
			RPB.pClearValues = clear_values.data();

			for (uint32_t i = 0; i < gpu->draw_commands.commandbuffers.size(); i++)
			{
				RPB.framebuffer = gpu->framebuffers[i];

				OP_SUCCESS(vkBeginCommandBuffer(gpu->draw_commands.commandbuffers[i], &CBI));

				vkCmdBeginRenderPass(gpu->draw_commands.commandbuffers[i], &RPB, VK_SUBPASS_CONTENTS_INLINE);

				VkViewport viewport = info::viewport(static_cast<float>(gpu->width), static_cast<uint32_t>(gpu->height), 0.f, 1.f);
				vkCmdSetViewport(gpu->draw_commands.commandbuffers[i], 0, 1, &viewport);

				VkRect2D scissor = info::rect2D(gpu->width, gpu->height, 0, 0);
				vkCmdSetScissor(gpu->draw_commands.commandbuffers[i], 0, 1, &scissor);

				vkCmdBindDescriptorSets(gpu->draw_commands.commandbuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);

				vkCmdBindPipeline(gpu->draw_commands.commandbuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.deferred);
				vkCmdDraw(gpu->draw_commands.commandbuffers[i], 3, 1, 0, 0);

				// draw UI

				vkCmdEndRenderPass(gpu->draw_commands.commandbuffers[i]);

				OP_SUCCESS(vkEndCommandBuffer(gpu->draw_commands.commandbuffers[i]));
			}
		}

		void deferred_shading::setup_renderpass()
		{

		}

		void deferred_shading::setup_framebuffers()
		{

		}

		void deferred_shading::prepare()
		{

		}

		void deferred_shading::update()
		{

		}

		void deferred_shading::render()
		{

		}

		void deferred_shading::cleanup()
		{

		}

		void deferred_shading::setup_shadow_pass() 
		{
			framebuffers.shadow.gpu = gpu;
			framebuffers.shadow.width = SHADOW_DIMENSIONS;
			framebuffers.shadow.height = SHADOW_DIMENSIONS;

			framebuffer::attachment_create_info AI{};
			AI.format = SHADOWMAP_FORMAT;
			AI.height = SHADOW_DIMENSIONS;
			AI.width = SHADOW_DIMENSIONS;
			AI.layer_count = 1;
			AI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

			framebuffers.shadow.add_attachment(AI);

			framebuffers.shadow.create_sampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
			framebuffers.shadow.create_renderpass();
		}

		void deferred_shading::setup_deferred_pass() 
		{
			framebuffers.deferred.gpu = gpu;
			framebuffers.deferred.width = FRAMEBUFFER_DIMENSIONS;
			framebuffers.deferred.height = FRAMEBUFFER_DIMENSIONS;

			framebuffer::attachment_create_info AI{};
			AI.width = FRAMEBUFFER_DIMENSIONS;
			AI.height = FRAMEBUFFER_DIMENSIONS;
			AI.layer_count = 1;
			AI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

			AI.format = VK_FORMAT_R16G16B16A16_SFLOAT; 
			framebuffers.deferred.add_attachment(AI); // world
			framebuffers.deferred.add_attachment(AI); // normals

			AI.format = VK_FORMAT_R8G8B8A8_UNORM;
			framebuffers.deferred.add_attachment(AI); // albedo

			VkFormat att_depth_fmt;
			bool valid_depth = query_depth_format_support(gpu->physical_device, att_depth_fmt);
			assert(valid_depth);

			AI.format = att_depth_fmt;
			AI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			framebuffers.deferred.add_attachment(AI);

			framebuffers.deferred.create_sampler(VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

			framebuffers.deferred.create_renderpass();
		}

		void deferred_shading::setup_deferred_commands() 
		{
			if (VK_NULL_HANDLE == command_buffers.deferred)
				command_buffers.deferred = gpu->create_commandbuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, false);

			auto SI = info::semaphore_create_info();
			vkCreateSemaphore(gpu->device, &SI, gpu->allocation_callbacks, &offscreen_semaphore);

			auto CBI = info::command_buffer_begin_info();
			
			auto RPB = info::renderpass_begin_info();
			std::array<VkClearValue, 4> clear_values{};
			VkViewport viewport;
			VkRect2D scissor;

			// shadow map pass
			clear_values[0] = { 1.f, 0.f };
			RPB.renderPass = framebuffers.shadow.renderpass;
			RPB.framebuffer = framebuffers.shadow.framebuffer;
			RPB.renderArea.extent.width = framebuffers.shadow.width;
			RPB.renderArea.extent.height = framebuffers.shadow.height;
			RPB.clearValueCount = 1;
			RPB.pClearValues = clear_values.data();

			vkBeginCommandBuffer(command_buffers.deferred, &CBI);

			viewport = info::viewport(framebuffers.shadow.width, framebuffers.shadow.height, 0.f, 1.f);
			vkCmdSetViewport(command_buffers.deferred, 0, 1, &viewport);

			scissor = info::rect2D(framebuffers.shadow.width, framebuffers.shadow.height, 0, 0);
			vkCmdSetScissor(command_buffers.deferred, 0, 1, &scissor);

			vkCmdSetDepthBias(command_buffers.deferred, depth_bias_constant, 0.f, depth_bias_slope);

			vkCmdBeginRenderPass(command_buffers.deferred, &RPB, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(command_buffers.deferred, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.shadow);
			render_scene(command_buffers.deferred, true);
			vkCmdEndRenderPass(command_buffers.deferred);

			// deferred pass 
			clear_values[0].color = { {0.f, 0.f, 0.f, 0.f} };
			clear_values[1].color = { {0.f, 0.f, 0.f, 0.f} };
			clear_values[2].color = { {0.f, 0.f, 0.f, 0.f} };
			clear_values[3].depthStencil = { 1.f, 0 };

			RPB.renderPass = framebuffers.deferred.renderpass;
			RPB.framebuffer = framebuffers.deferred.framebuffer;
			RPB.renderArea.extent.width = framebuffers.deferred.width;
			RPB.renderArea.extent.height = framebuffers.deferred.height;
			RPB.clearValueCount = static_cast<uint32_t>(clear_values.size());
			RPB.pClearValues = clear_values.data();

			vkCmdBeginRenderPass(command_buffers.deferred, &RPB, VK_SUBPASS_CONTENTS_INLINE);

			viewport = info::viewport(framebuffers.deferred.width, framebuffers.deferred.height, 0.f, 1.f);
			vkCmdSetViewport(command_buffers.deferred, 0, 1, &viewport);

			scissor = info::rect2D(framebuffers.deferred.width, framebuffers.deferred.height, 0, 0);
			vkCmdSetScissor(command_buffers.deferred, 0, 1, &scissor);

			vkCmdBindPipeline(command_buffers.deferred, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.offscreen);
			render_scene(command_buffers.deferred, false);
			vkCmdEndRenderPass(command_buffers.deferred);

			OP_SUCCESS(vkEndCommandBuffer(command_buffers.deferred));
		}

		void deferred_shading::setup_descriptor_pool() 
		{
		
		}

		void deferred_shading::setup_descriptor_layout() 
		{

		}

		void deferred_shading::setup_descriptor_set() 
		{
		
		}

		void deferred_shading::prepare_pipelines() 
		{
			auto input_assembly_state = info::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
			auto rasterizer_state = info::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
			auto blend_attachment = info::pipeline_color_blend_attachment_state(0xf, VK_FALSE);
			auto color_blend = info::pipeline_color_blend_state_create_info(1, &blend_attachment);
			auto depth_stencil_state = info::pipeline_depth_stencil_state_create_info(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
			auto viewport_state = info::pipeline_viewport_state_create_info(1, 1, 0);
			auto multi_sample_state = info::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);
			std::vector<VkDynamicState> enabled_dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
			auto dynamic_state = info::pipeline_dynamic_state_create_info(enabled_dynamic_states);
			std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;

			auto GPC = info::pipeline_create_info(pipeline_layout, gpu->renderpass);
			GPC.pInputAssemblyState = &input_assembly_state;
			GPC.pRasterizationState = &rasterizer_state;
			GPC.pColorBlendState = &color_blend;
			GPC.pMultisampleState = &multi_sample_state;
			GPC.pViewportState = &viewport_state;
			GPC.pDepthStencilState = &depth_stencil_state;
			GPC.pDynamicState = &dynamic_state;
			GPC.stageCount = static_cast<uint32_t>(shader_stages.size());
			GPC.pStages = shader_stages.data();

			rasterizer_state.cullMode = VK_CULL_MODE_FRONT_BIT;
			// shader loading

			auto empty_input_state = info::pipeline_vertex_input_state_create_info();
			GPC.pVertexInputState = &empty_input_state;
			OP_SUCCESS(vkCreateGraphicsPipelines(gpu->device, gpu->pipeline_cache, 1, &GPC, gpu->allocation_callbacks, &pipelines.deferred));


		}

		void deferred_shading::prepare_uniform_buffers() 
		{
		
		}

		void deferred_shading::update_uniform_buffers() 
		{
		
		}

		void deferred_shading::render_scene(VkCommandBuffer cmd_buffer, bool shadow)
		{

		}
	}
}