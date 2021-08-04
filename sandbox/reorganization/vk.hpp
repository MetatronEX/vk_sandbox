#ifndef VK_HPP
#define VK_HPP

#inlcude <vulkan/vulkan.h>

#include "win32.hpp"
#include "common.hpp"

#define VK_FLAGS_NONE 0

#define DEFAULT_FENCE_TIMEOUT 100000000000

#define OP_SUCCESS(f)																				                                      \
{																										                                  \
	VkResult res = (f);																					                                  \
	if (res != VK_SUCCESS)																				                                  \
	{																									                                  \
		std::cout << "Fatal : VkResult is \"" << vk::error_string(res) << "\" in " << __FILE__ << " at line " << __LINE__ << "\n"; \
		assert(res == VK_SUCCESS);																		                                  \
	}																									                                  \
}

namespace vk
{
    namespace info
    {
        inline VkMemoryAllocateInfo memory_allocate_info()
		{
			VkMemoryAllocateInfo memAllocInfo {};
			memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			return memAllocInfo;
		}

		inline VkMappedMemoryRange mapped_memory_range()
		{
			VkMappedMemoryRange mappedMemoryRange {};
			mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			return mappedMemoryRange;
		}

		inline VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool commandPool, VkCommandBufferLevel level, uint32_t bufferCount)
		{
			VkCommandBufferAllocateInfo commandBufferAllocateInfo {};
			commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			commandBufferAllocateInfo.commandPool = commandPool;
			commandBufferAllocateInfo.level = level;
			commandBufferAllocateInfo.commandBufferCount = bufferCount;
			return commandBufferAllocateInfo;
		}

		inline VkCommandPoolCreateInfo command_pool_create_info()
		{
			VkCommandPoolCreateInfo cmdPoolCreateInfo {};
			cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			return cmdPoolCreateInfo;
		}

		inline VkCommandBufferBeginInfo command_buffer_begin_info()
		{
			VkCommandBufferBeginInfo cmdBufferBeginInfo {};
			cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			return cmdBufferBeginInfo;
		}

		inline VkCommandBufferInheritanceInfo command_buffer_inheritance_info()
		{
			VkCommandBufferInheritanceInfo cmdBufferInheritanceInfo {};
			cmdBufferInheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
			return cmdBufferInheritanceInfo;
		}

		inline VkRenderPassBeginInfo renderpass_begin_info()
		{
			VkRenderPassBeginInfo renderPassBeginInfo {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			return renderPassBeginInfo;
		}

		inline VkRenderPassCreateInfo renderpass_create_info()
		{
			VkRenderPassCreateInfo renderPassCreateInfo {};
			renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			return renderPassCreateInfo;
		}

		/** @brief Initialize an image memory barrier with no image transfer ownership */
		inline VkImageMemoryBarrier image_memory_barrier()
		{
			VkImageMemoryBarrier imageMemoryBarrier {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			return imageMemoryBarrier;
		}

		/** @brief Initialize a buffer memory barrier with no image transfer ownership */
		inline VkBufferMemoryBarrier buffer_memory_barrier()
		{
			VkBufferMemoryBarrier bufferMemoryBarrier {};
			bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			return bufferMemoryBarrier;
		}

		inline VkMemoryBarrier memory_barrier()
		{
			VkMemoryBarrier memoryBarrier {};
			memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			return memoryBarrier;
		}

		inline VkImageCreateInfo image_create_info()
		{
			VkImageCreateInfo imageCreateInfo {};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			return imageCreateInfo;
		}

		inline VkSamplerCreateInfo sampler_create_info()
		{
			VkSamplerCreateInfo samplerCreateInfo {};
			samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerCreateInfo.maxAnisotropy = 1.0f;
			return samplerCreateInfo;
		}

		inline VkImageViewCreateInfo image_view_create_info()
		{
			VkImageViewCreateInfo imageViewCreateInfo {};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			return imageViewCreateInfo;
		}

		inline VkFramebufferCreateInfo framebuffer_create_info()
		{
			VkFramebufferCreateInfo framebufferCreateInfo {};
			framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			return framebufferCreateInfo;
		}

		inline VkSemaphoreCreateInfo semaphore_create_info()
		{
			VkSemaphoreCreateInfo semaphoreCreateInfo {};
			semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			return semaphoreCreateInfo;
		}

		inline VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags = 0)
		{
			VkFenceCreateInfo fenceCreateInfo {};
			fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceCreateInfo.flags = flags;
			return fenceCreateInfo;
		}

		inline VkEventCreateInfo event_create_info()
		{
			VkEventCreateInfo eventCreateInfo {};
			eventCreateInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
			return eventCreateInfo;
		}

		inline VkSubmitInfo submit_info()
		{
			VkSubmitInfo submitInfo {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			return submitInfo;
		}

        inline VkViewport viewport(
			float width,
			float height,
			float minDepth,
			float maxDepth)
		{
			VkViewport viewport {};
			viewport.width = width;
			viewport.height = height;
			viewport.minDepth = minDepth;
			viewport.maxDepth = maxDepth;
			return viewport;
		}

		inline VkRect2D rect2D(
			int32_t width,
			int32_t height,
			int32_t offsetX,
			int32_t offsetY)
		{
			VkRect2D rect2D {};
			rect2D.extent.width = width;
			rect2D.extent.height = height;
			rect2D.offset.x = offsetX;
			rect2D.offset.y = offsetY;
			return rect2D;
		}

		inline VkBufferCreateInfo buffer_create_info()
		{
			VkBufferCreateInfo bufCreateInfo {};
			bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			return bufCreateInfo;
		}

		inline VkBufferCreateInfo buffer_create_info(
			VkBufferUsageFlags usage,
			VkDeviceSize size)
		{
			VkBufferCreateInfo bufCreateInfo {};
			bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufCreateInfo.usage = usage;
			bufCreateInfo.size = size;
			return bufCreateInfo;
		}

		inline VkDescriptorPoolCreateInfo descriptor_pool_create_info(
			uint32_t poolSizeCount,
			VkDescriptorPoolSize* pPoolSizes,
			uint32_t maxSets)
		{
			VkDescriptorPoolCreateInfo descriptorPoolInfo {};
			descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			descriptorPoolInfo.poolSizeCount = poolSizeCount;
			descriptorPoolInfo.pPoolSizes = pPoolSizes;
			descriptorPoolInfo.maxSets = maxSets;
			return descriptorPoolInfo;
		}

		inline VkDescriptorPoolCreateInfo descriptor_pool_create_info(
			const std::vector<VkDescriptorPoolSize>& poolSizes,
			uint32_t maxSets)
		{
			VkDescriptorPoolCreateInfo descriptorPoolInfo{};
			descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
			descriptorPoolInfo.pPoolSizes = poolSizes.data();
			descriptorPoolInfo.maxSets = maxSets;
			return descriptorPoolInfo;
		}

		inline VkDescriptorPoolSize descriptor_pool_size(
			VkDescriptorType type,
			uint32_t descriptorCount)
		{
			VkDescriptorPoolSize descriptorPoolSize {};
			descriptorPoolSize.type = type;
			descriptorPoolSize.descriptorCount = descriptorCount;
			return descriptorPoolSize;
		}

		inline VkDescriptorSetLayoutBinding descriptor_set_layout_binding(
			VkDescriptorType type,
			VkShaderStageFlags stageFlags,
			uint32_t binding,
			uint32_t descriptorCount = 1)
		{
			VkDescriptorSetLayoutBinding setLayoutBinding {};
			setLayoutBinding.descriptorType = type;
			setLayoutBinding.stageFlags = stageFlags;
			setLayoutBinding.binding = binding;
			setLayoutBinding.descriptorCount = descriptorCount;
			return setLayoutBinding;
		}

		inline VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info(
			const VkDescriptorSetLayoutBinding* pBindings,
			uint32_t bindingCount)
		{
			VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {};
			descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorSetLayoutCreateInfo.pBindings = pBindings;
			descriptorSetLayoutCreateInfo.bindingCount = bindingCount;
			return descriptorSetLayoutCreateInfo;
		}

		inline VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info(
			const std::vector<VkDescriptorSetLayoutBinding>& bindings)
		{
			VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
			descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorSetLayoutCreateInfo.pBindings = bindings.data();
			descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
			return descriptorSetLayoutCreateInfo;
		}

		inline VkPipelineLayoutCreateInfo pipeline_layout_create_info(
			const VkDescriptorSetLayout* pSetLayouts,
			uint32_t setLayoutCount = 1)
		{
			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
			pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
			pipelineLayoutCreateInfo.pSetLayouts = pSetLayouts;
			return pipelineLayoutCreateInfo;
		}

		inline VkPipelineLayoutCreateInfo pipeline_layout_create_info(
			uint32_t setLayoutCount = 1)
		{
			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
			pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
			return pipelineLayoutCreateInfo;
		}

		inline VkDescriptorSetAllocateInfo descriptor_set_allocate_info(
			VkDescriptorPool descriptorPool,
			const VkDescriptorSetLayout* pSetLayouts,
			uint32_t descriptorSetCount)
		{
			VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {};
			descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descriptorSetAllocateInfo.descriptorPool = descriptorPool;
			descriptorSetAllocateInfo.pSetLayouts = pSetLayouts;
			descriptorSetAllocateInfo.descriptorSetCount = descriptorSetCount;
			return descriptorSetAllocateInfo;
		}

		inline VkDescriptorImageInfo descriptor_image_info(VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout)
		{
			VkDescriptorImageInfo descriptorImageInfo {};
			descriptorImageInfo.sampler = sampler;
			descriptorImageInfo.imageView = imageView;
			descriptorImageInfo.imageLayout = imageLayout;
			return descriptorImageInfo;
		}

		inline VkWriteDescriptorSet write_descriptor_set(
			VkDescriptorSet dstSet,
			VkDescriptorType type,
			uint32_t binding,
			VkDescriptorBufferInfo* bufferInfo,
			uint32_t descriptorCount = 1)
		{
			VkWriteDescriptorSet writeDescriptorSet {};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = dstSet;
			writeDescriptorSet.descriptorType = type;
			writeDescriptorSet.dstBinding = binding;
			writeDescriptorSet.pBufferInfo = bufferInfo;
			writeDescriptorSet.descriptorCount = descriptorCount;
			return writeDescriptorSet;
		}

		inline VkWriteDescriptorSet write_descriptor_set(
			VkDescriptorSet dstSet,
			VkDescriptorType type,
			uint32_t binding,
			VkDescriptorImageInfo *imageInfo,
			uint32_t descriptorCount = 1)
		{
			VkWriteDescriptorSet writeDescriptorSet {};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = dstSet;
			writeDescriptorSet.descriptorType = type;
			writeDescriptorSet.dstBinding = binding;
			writeDescriptorSet.pImageInfo = imageInfo;
			writeDescriptorSet.descriptorCount = descriptorCount;
			return writeDescriptorSet;
		}

		inline VkVertexInputBindingDescription vertex_input_binding_description(
			uint32_t binding,
			uint32_t stride,
			VkVertexInputRate inputRate)
		{
			VkVertexInputBindingDescription vInputBindDescription {};
			vInputBindDescription.binding = binding;
			vInputBindDescription.stride = stride;
			vInputBindDescription.inputRate = inputRate;
			return vInputBindDescription;
		}

		inline VkVertexInputAttributeDescription vertex_input_attribute_description(
			uint32_t binding,
			uint32_t location,
			VkFormat format,
			uint32_t offset)
		{
			VkVertexInputAttributeDescription vInputAttribDescription {};
			vInputAttribDescription.location = location;
			vInputAttribDescription.binding = binding;
			vInputAttribDescription.format = format;
			vInputAttribDescription.offset = offset;
			return vInputAttribDescription;
		}

		inline VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info()
		{
			VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo {};
			pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			return pipelineVertexInputStateCreateInfo;
		}

		inline VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info(
			const std::vector<VkVertexInputBindingDescription> &vertexBindingDescriptions,
			const std::vector<VkVertexInputAttributeDescription> &vertexAttributeDescriptions
		)
		{
			VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
			pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindingDescriptions.size());
			pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = vertexBindingDescriptions.data();
			pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptions.size());
			pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();
			return pipelineVertexInputStateCreateInfo;
		}

		inline VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info(
			VkPrimitiveTopology topology,
			VkPipelineInputAssemblyStateCreateFlags flags,
			VkBool32 primitiveRestartEnable)
		{
			VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo {};
			pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			pipelineInputAssemblyStateCreateInfo.topology = topology;
			pipelineInputAssemblyStateCreateInfo.flags = flags;
			pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = primitiveRestartEnable;
			return pipelineInputAssemblyStateCreateInfo;
		}

		inline VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info(
			VkPolygonMode polygonMode,
			VkCullModeFlags cullMode,
			VkFrontFace frontFace,
			VkPipelineRasterizationStateCreateFlags flags = 0)
		{
			VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo {};
			pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			pipelineRasterizationStateCreateInfo.polygonMode = polygonMode;
			pipelineRasterizationStateCreateInfo.cullMode = cullMode;
			pipelineRasterizationStateCreateInfo.frontFace = frontFace;
			pipelineRasterizationStateCreateInfo.flags = flags;
			pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
			pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
			return pipelineRasterizationStateCreateInfo;
		}

		inline VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_state(
			VkColorComponentFlags colorWriteMask,
			VkBool32 blendEnable)
		{
			VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState {};
			pipelineColorBlendAttachmentState.colorWriteMask = colorWriteMask;
			pipelineColorBlendAttachmentState.blendEnable = blendEnable;
			return pipelineColorBlendAttachmentState;
		}

		inline VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info(
			uint32_t attachmentCount,
			const VkPipelineColorBlendAttachmentState * pAttachments)
		{
			VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo {};
			pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			pipelineColorBlendStateCreateInfo.attachmentCount = attachmentCount;
			pipelineColorBlendStateCreateInfo.pAttachments = pAttachments;
			return pipelineColorBlendStateCreateInfo;
		}

		inline VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info(
			VkBool32 depthTestEnable,
			VkBool32 depthWriteEnable,
			VkCompareOp depthCompareOp)
		{
			VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo {};
			pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			pipelineDepthStencilStateCreateInfo.depthTestEnable = depthTestEnable;
			pipelineDepthStencilStateCreateInfo.depthWriteEnable = depthWriteEnable;
			pipelineDepthStencilStateCreateInfo.depthCompareOp = depthCompareOp;
			pipelineDepthStencilStateCreateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
			return pipelineDepthStencilStateCreateInfo;
		}

		inline VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info(
			uint32_t viewportCount,
			uint32_t scissorCount,
			VkPipelineViewportStateCreateFlags flags = 0)
		{
			VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo {};
			pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			pipelineViewportStateCreateInfo.viewportCount = viewportCount;
			pipelineViewportStateCreateInfo.scissorCount = scissorCount;
			pipelineViewportStateCreateInfo.flags = flags;
			return pipelineViewportStateCreateInfo;
		}

		inline VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info(
			VkSampleCountFlagBits rasterizationSamples,
			VkPipelineMultisampleStateCreateFlags flags = 0)
		{
			VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo {};
			pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			pipelineMultisampleStateCreateInfo.rasterizationSamples = rasterizationSamples;
			pipelineMultisampleStateCreateInfo.flags = flags;
			return pipelineMultisampleStateCreateInfo;
		}

		inline VkPipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info(
			const VkDynamicState * pDynamicStates,
			uint32_t dynamicStateCount,
			VkPipelineDynamicStateCreateFlags flags = 0)
		{
			VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo {};
			pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			pipelineDynamicStateCreateInfo.pDynamicStates = pDynamicStates;
			pipelineDynamicStateCreateInfo.dynamicStateCount = dynamicStateCount;
			pipelineDynamicStateCreateInfo.flags = flags;
			return pipelineDynamicStateCreateInfo;
		}

		inline VkPipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info(
			const std::vector<VkDynamicState>& pDynamicStates,
			VkPipelineDynamicStateCreateFlags flags = 0)
		{
			VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
			pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			pipelineDynamicStateCreateInfo.pDynamicStates = pDynamicStates.data();
			pipelineDynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(pDynamicStates.size());
			pipelineDynamicStateCreateInfo.flags = flags;
			return pipelineDynamicStateCreateInfo;
		}

		inline VkPipelineTessellationStateCreateInfo pipeline_tessellation_state_create_info(uint32_t patchControlPoints)
		{
			VkPipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo {};
			pipelineTessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
			pipelineTessellationStateCreateInfo.patchControlPoints = patchControlPoints;
			return pipelineTessellationStateCreateInfo;
		}

		inline VkGraphicsPipelineCreateInfo pipeline_create_info(
			VkPipelineLayout layout,
			VkRenderPass renderPass,
			VkPipelineCreateFlags flags = 0)
		{
			VkGraphicsPipelineCreateInfo pipelineCreateInfo {};
			pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineCreateInfo.layout = layout;
			pipelineCreateInfo.renderPass = renderPass;
			pipelineCreateInfo.flags = flags;
			pipelineCreateInfo.basePipelineIndex = -1;
			pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
			return pipelineCreateInfo;
		}

		inline VkGraphicsPipelineCreateInfo pipeline_create_info()
		{
			VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
			pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineCreateInfo.basePipelineIndex = -1;
			pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
			return pipelineCreateInfo;
		}

		inline VkComputePipelineCreateInfo compute_pipeline_create_info(
			VkPipelineLayout layout, 
			VkPipelineCreateFlags flags = 0)
		{
			VkComputePipelineCreateInfo computePipelineCreateInfo {};
			computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			computePipelineCreateInfo.layout = layout;
			computePipelineCreateInfo.flags = flags;
			return computePipelineCreateInfo;
		}

		inline VkPushConstantRange push_constant_range(
			VkShaderStageFlags stageFlags,
			uint32_t size,
			uint32_t offset)
		{
			VkPushConstantRange pushConstantRange {};
			pushConstantRange.stageFlags = stageFlags;
			pushConstantRange.offset = offset;
			pushConstantRange.size = size;
			return pushConstantRange;
		}

		inline VkBindSparseInfo bind_sparse_info()
		{
			VkBindSparseInfo bindSparseInfo{};
			bindSparseInfo.sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
			return bindSparseInfo;
		}

		/** @brief Initialize a map entry for a shader specialization constant */
		inline VkSpecializationMapEntry specialization_map_entry(uint32_t constantID, uint32_t offset, size_t size)
		{
			VkSpecializationMapEntry specializationMapEntry{};
			specializationMapEntry.constantID = constantID;
			specializationMapEntry.offset = offset;
			specializationMapEntry.size = size;
			return specializationMapEntry;
		}

		/** @brief Initialize a specialization constant info structure to pass to a shader stage */
		inline VkSpecializationInfo specialization_info(uint32_t mapEntryCount, const VkSpecializationMapEntry* mapEntries, size_t dataSize, const void* data)
		{
			VkSpecializationInfo specializationInfo{};
			specializationInfo.mapEntryCount = mapEntryCount;
			specializationInfo.pMapEntries = mapEntries;
			specializationInfo.dataSize = dataSize;
			specializationInfo.pData = data;
			return specializationInfo;
		}

		/** @brief Initialize a specialization constant info structure to pass to a shader stage */
		inline VkSpecializationInfo specialization_info(const std::vector<VkSpecializationMapEntry> &mapEntries, size_t dataSize, const void* data)
		{
			VkSpecializationInfo specializationInfo{};
			specializationInfo.mapEntryCount = static_cast<uint32_t>(mapEntries.size());
			specializationInfo.pMapEntries = mapEntries.data();
			specializationInfo.dataSize = dataSize;
			specializationInfo.pData = data;
			return specializationInfo;
		}

		// Ray tracing related
		inline VkAccelerationStructureGeometryKHR acceleration_structure_geometry_KHR()
		{
			VkAccelerationStructureGeometryKHR accelerationStructureGeometryKHR{};
			accelerationStructureGeometryKHR.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
			return accelerationStructureGeometryKHR;
		}

		inline VkAccelerationStructureBuildGeometryInfoKHR acceleration_structure_build_geometry_Info_KHR()
		{
			VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfoKHR{};
			accelerationStructureBuildGeometryInfoKHR.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
			return accelerationStructureBuildGeometryInfoKHR;
		}

		inline VkAccelerationStructureBuildSizesInfoKHR acceleration_structure_build_sizes_info_KHR()
		{
			VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfoKHR{};
			accelerationStructureBuildSizesInfoKHR.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
			return accelerationStructureBuildSizesInfoKHR;
		}

		inline VkRayTracingShaderGroupCreateInfoKHR ray_tracing_shader_group_create_info_KHR()
		{
			VkRayTracingShaderGroupCreateInfoKHR rayTracingShaderGroupCreateInfoKHR{};
			rayTracingShaderGroupCreateInfoKHR.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			return rayTracingShaderGroupCreateInfoKHR;
		}

		inline VkRayTracingPipelineCreateInfoKHR ray_tracing_pipeline_create_info_KHR()
		{
			VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfoKHR{};
			rayTracingPipelineCreateInfoKHR.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
			return rayTracingPipelineCreateInfoKHR;
		}

		inline VkWriteDescriptorSetAccelerationStructureKHR write_descriptor_set_acceleration_structure_KHR()
		{
			VkWriteDescriptorSetAccelerationStructureKHR writeDescriptorSetAccelerationStructureKHR{};
			writeDescriptorSetAccelerationStructureKHR.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
			return writeDescriptorSetAccelerationStructureKHR;
		}
    }

    struct queue_family_indices
    {
        std::optional<uint32_t> graphics;
        std::optional<uint32_t> present;
        std::optional<uint32_t> compute;
        std::optional<uint32_t> transfer;
        std::optional<uint32_t> sparse_binding;

        bool complete()
        {
            return graphics.has_value() && present.has_value();
        }
    };

    const char* error_string(VkResult error_code)
    {
        switch (error_code)
		{
#define STR(r) case VK_ ##r: return #r
            STR(NOT_READY);
            STR(TIMEOUT);
            STR(EVENT_SET);
            STR(EVENT_RESET);
            STR(INCOMPLETE);
            STR(ERROR_OUT_OF_HOST_MEMORY);
            STR(ERROR_OUT_OF_DEVICE_MEMORY);
            STR(ERROR_INITIALIZATION_FAILED);
            STR(ERROR_DEVICE_LOST);
            STR(ERROR_MEMORY_MAP_FAILED);
            STR(ERROR_LAYER_NOT_PRESENT);
            STR(ERROR_EXTENSION_NOT_PRESENT);
            STR(ERROR_FEATURE_NOT_PRESENT);
            STR(ERROR_INCOMPATIBLE_DRIVER);
            STR(ERROR_TOO_MANY_OBJECTS);
            STR(ERROR_FORMAT_NOT_SUPPORTED);
            STR(ERROR_SURFACE_LOST_KHR);
            STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
            STR(SUBOPTIMAL_KHR);
            STR(ERROR_OUT_OF_DATE_KHR);
            STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
            STR(ERROR_VALIDATION_FAILED_EXT);
            STR(ERROR_INVALID_SHADER_NV);
#undef STR
            default:
                return "UNKNOWN_ERROR";
		}
    }

    

    VkSurfaceCapabilitiesKHR query_surface_capabilities(VkPhysicalDevice _pd, VkSurfaceKHR _s)
    {
        VkSurfaceCapabilitiesKHR surf_caps;

        OP_SUCCESS(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_pd, _s, &surf_caps));

        return surf_caps;
    }

    std::vector<VkSurfaceFormatKHR> query_surface_formats(VkPhysicalDevice _pd, VkSurfaceKHR _s)
    {
        uint32_t format_count;
        OP_SUCCESS(vkGetPhysicalDeviceSurfaceFormatsKHR(_pd, _s, &format_count, nullptr));
        assert(format_count > 0);

        std::vector<VkSurfaceFormatKHR> surface_formats(format_count);
        OP_SUCCESS(vkGetPhysicalDeviceSurfaceFormatsKHR(_pd, _s, &format_count, surface_formats.data()));

        return surface_formats;
    }

    std::vector<VkPresentModeKHR> query_surface_present_modes(VkPhysicalDevice _pd, VkSurfaceKHR _s)
    {
        uint32_t present_mode_count;
        OP_SUCCESS(vkGetPhysicalDeviceSurfacePresentModesKHR(_pd, _s, &present_mode_count, nullptr));
        assert(present_mode_count > 0);

        std::vector<VkPresentModeKHR> present_modes;
        OP_SUCCESS(vkGetPhysicalDeviceSurfacePresentModesKHR(_pd, _s, &present_mode_count, present_modes.data()));

        return present_modes;
    }

    bool support_present_to_surface(VkPhysicalDevice _pd, VkSurfaceKHR _s, const uint32_t index)
    {
        VkBool32 support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(_pd, index, _s, &support);
        return support ? true : false;
    }

    queue_family_indices find_queue_families(VkPhysicalDevice _pd, VkSurfaceKHR _s)
    {
        queue_family_indices indices;

        auto queue_families = query_queue_family_properties(_pd);

        for (size_t i = 0; i < queue_families.size(); i++)
        {
            if(!indices.graphics.has_value() && 
            queue_families[i].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                indices.graphics = i;
            
            if(!indices.compute.has_value() &&
            queue_families[i].queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT)
                indices.compute = i;

            if(!indices.transfer.has_value() &&
            queue_families[i].queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT)
                indices.transfer = i;

            if(!indices.sparse_binding.has_value() &&
            queue_families[i].queueFamilyProperties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
                indices.sparse_binding = i;
        }

        bool support_present = support_present_to_surface(_pd,_s,indices.graphics.value());

        if (support_present)
            indices.present = indices.graphics.value();
        else
        {
            for (uint32_t i = 0; i < queue_families.size(); i++)
            {
                bool support = support_present_to_surface(_pd, _s, i);

                if (support)
                {
                    indices.present = i;
                    break;
                }
            }
        }

        return indices;
    }

    std::vector<VkQueueFamilyProperties2> query_queue_family_properties(VkPhysicalDevice _pd)
    {
        uint32_t family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties2(_pd, &family_count, nullptr);
        assert(family_count >= 1);

        std::vector<VkQueueFamilyProperties2> queue_families(family_count);

        for (auto& F : queue_families)
            F.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
        
        vkGetPhysicalDeviceQueueFamilyProperties2(_pd, &family_count, queue_families.data());

        return queue_families;
    }

    uint32_t query_memory_type(VkPhysicalDevice _pd, const uint32_t type_filter, const VkMemoryPropertyFlags flags, bool* found)
    {
        VkPhysicalDeviceMemoryProperties2 MP{};
        MP.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
        vkGetPhysicalDeviceMemoryProperties2(_pd, &MP);

        for (uint32_t i = 0; i < MP.memoryProperties.memoryTypeCount; i++)
        {
            if((type_filter & (1 << i)) &&
            (MP.memoryProperties.memoryTypes[i].propertyFlags & flags) == flags)
            {
                if(found)
                    found = true;
            
                return i;
            }            
        }

        if(found)
            found = false;

        return 0;
    }

    std::vector<VkExtensionProperties> query_available_extensions(VkPhysicalDevice _pd)
    {
        uint32_t ext_count = 0;
        vkEnumerateDeviceExtensionProperties(_pd, nullptr, &ext_count, nullptr);

        std::vector<VkExtensionProperties> available_extensions(ext_count);
        vkEnumerateDeviceExtensionProperties(_pd, nullptr, &ext_count, available_extensions.data());

        return available_extensions;
    }

    bool check_extension_support(VkPhysicalDevice _pd, const std::vector<const char*>& device_extensions)
    {
        auto available_extensions = query_available_extensions(_pd);
        std::set<const char*> required_extensions(device_extensions.begin(), device_extensions.end());

        for (const auto& e : available_extensions)
            required_extensions.erase(e.extensionName);

        return required_extensions.empty();
    }

    bool is_device_suitable(VkPhysicalDevice _pd, VkSurfaceKHR _s)
    {
        VkPhysicalDeviceProperties2 dev_props{};
        dev_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        VkPhysicalDeviceFeatures2 dev_feats{};
        dev_feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

        vkGetPhysicalDeviceProperties2(_pd, &dev_props);
		vkGetPhysicalDeviceFeatures2(_pd, &dev_feats);

        auto indices = find_queue_families(_pd, _s);

        bool extensions_supported = check_extension_support(_pd);

        bool adequate = false;

        if (extensions_supported)
        {
            auto formats = query_surface_formats(_pd,_s);
            auto present_modes = query_surface_present_modes(_pd,_s);

            adequate = !formats.empty() && !present_modes.empty();
        }

       return  dev_props.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
        dev_feats.features.tessellationShader &&
        indices.complete() &&
        extensions_supported &&
        dev_feats.features.samplerAnisotropy &&
        adequate; 
    }

    bool query_depth_format_support(VkPhysicalDevice _pd, VkFormat& _f, const bool check_sample_support = false)
    {
        std::array<VkFormat, 5> formats - {
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D24_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM
        };

        for (auto& f : formats)
        {
            VkFormatProperties2 FP{};
            FP.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
            vkGetPhysicalDeviceFormatProperties2(_pd, f, &FP);

            if(FP.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                if(check_sample_support)
                    if(!(VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT & FP.formatProperties.optimalTilingFeatures))
                        continue;

                _f = f;
                return true;
            }
        }

        return false;
    }

    bool is_format_filterable(VkPhysicalDevice _pd, const VkFormat _f, const VKImageTiling _t)
    {
        VkFormatProperties2 FP{};
        FP.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
        vkGetPhysicalDeviceFormatProperties2(_pd, f, &FP);

        if (VK_IMAGE_TILING_OPTIMAL == _t) 
            return FP.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
        if (VK_IMAGE_TILING_LINEAR == _t)
            return FP.formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;

        return false;
    }

    namespace command
    {
        VkCommandPool create_commandpool(VkDevice device, const uint32_t queue_familiy_index, const VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
        {
            VkCommandPool CP;
            auto CPC = info::command_pool_create_info();
            CPC.queueFamilyIndex = queue_familiy_index;
            CPC.flags = flags;
            OP_SUCCESS(vkCreateCommandPool(device,&CPC,allocation_callbacks,&CP));
            return CP;
        }

        void set_image_layout(VkCommandBuffer commandbuffer, VkImage image, VkImageAspectFlags aspect_mask, VkImageLayout old_layout, VkImageLayout new_layout,
                                VkPipelineStageFlags src_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 
                                VkPipelineStageFlags dst_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT)
        {
            VkImageSubresourceRange ISR{};
            ISR.aspectMask = aspect_mask;
            ISR.baseMipLevel = 0;
            ISR.levelCount = 1;
            ISR.layerCount = 1;
            set_image_layout(commandbuffer, image, old_layout, new_layout, ISR, src_mask, dst_mask);
        }

        void set_image_layout(VkCommandBuffer commandbuffer, VkImage image, VkImageAspectFlags aspect_mask, VkImageLayout old_layout, VkImageLayout new_layout, VkImageSubresourceRange& subresource_range,
                            VkPipelineStageFlags src_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 
                            VkPipelineStageFlags dst_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT)
        {
            auto IMB = info::image_memory_barrier();
            IMB.oldLayout = old_layout;
            IMB.newLayout = new_layout;
            IMB.image = image;
            IMB.subresourceRange = subresource_range;

            switch (old_layout)
            {
                case VK_IMAGE_LAYOUT_UNDEFINED:
                    IMB.srcAccessMask = 0;
                    break;

                case VK_IMAGE_LAYOUT_PREINITIALIZED:
                    IMB.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
                    break;

                case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                    IMB.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    break;

                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                    IMB.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                    break;

                case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                    IMB.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                    break;

                case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                    IMB.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    break;

                case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                    IMB.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    break;
                default:
                    break;
            }

            switch (new_layout)
            {
                case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                    IMB.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    break;

                case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                    IMB.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                    break;

                case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                    IMB.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    break;

                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                    IMB.dstAccessMask = IMB.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                    break;

                case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                    if (IMB.srcAccessMask == 0)
                        IMB.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
                    
                    IMB.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    break;
                default:
                    break;
            }

            vkCmsPipelineBarrier(commandbuffer, src_mask, dst_mask, 0, 0, nullptr, 0, nullptr, 1, &IMB);
        }

        void insertImageMemoryBarrier(VkCommandBuffer commandbuffer, VkImage image, VkAccessFlags src_access, VkAccessFlags dst_access, VkImageLayout old_layout, VkImageLayout new_layout,
                                VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage, VkImageSubresourceRange subresource_range)
        {
            auto IMB = info::image_memory_barrier();
            IMB.srcAccessMask = src_access;
            IMB.dstAccessMask = dst_access;
            IMB.oldLayout = old_layout;
            IMB.newLayout = new_layout;
            IMB.image = image;
            IMB.subresourceRange = subresource_range;

            vkCmdPipelineBarrier(commandbuffer, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &IMB);
        }
    }
}

#endif