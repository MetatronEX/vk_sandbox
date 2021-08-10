#include "deferred_shading.hpp"

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

		}

		void deferred_shading::setup_drawcommands()
		{

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

		}

		void deferred_shading::setup_deferred_pass() 
		{
		
		}

		void deferred_shading::setup_deferred_commands() 
		{
		
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
		
		}

		void deferred_shading::prepare_uniform_buffers() 
		{
		
		}

		void deferred_shading::update_uniform_buffers() 
		{
		
		}
	}
}