#ifndef VK_LOGICALDEVICE_HPP
#define VK_LOGICALDEVICE_HPP

#include "vk.hpp"
#include "vk_buffer.hpp"
#include "vk_drawcommand.hpp"
#include "vk_swapchain.hpp"

namespace vk
{
    struct depth_stencil
    {
        VkImage         image;
        VkDeviceMemory  memory;
        VkImageView     view;
    };

    struct GPU 
    {
        VkPhysicalDevice                    physical_device;
        VkDevice                            device;
        VkAllocationCallbacks*              allocation_callbacks;

        VkPhysicalDeviceProperties2         device_properties;
        VkPhysicalDeviceMemoryProperties2   device_memory_properties;
        VkPhysicalDeviceFeatures2           device_features;
        VkPhysicalDeviceFeatures2           enabled_features;
        VkCommandPool                       commandpool;
        VkRenderPass                        renderpass;
        VkDescriptorPool                    descriptorpool { VK_NULL_HANDLE };
        VkPipelineCache                     pipeline_cache { VK_NULL_HANDLE };
        queue_family_indices                queue_indices;
        depth_stencil                       depthstencil;
        draw_command                        draw_commands;

        std::vector<VkFramebuffer>          framebuffers;
        std::vector<VkShaderModule>         shader_modules;
        std::vector<const char*>            supported_device_extensions;
        std::vector<const char*>            enable_device_extensions;

        void*                               features_chain{ nullptr };

        swap_chain*                         swapchain{ nullptr };
        VkFormat                            depth_format;
        VkQueueFlags                        requested_queue { VK_QUEUE_GRAPHICS_BIT };
        VkPipelineStageFlags                submit_pipeline_stages { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        
        uint32_t                            width;
        uint32_t                            height;
        
        bool                                headless_rendering {false};

        VkResult                            create();
        VkFormat                            query_for_depth_format_support(const bool check_sampling_support);
        bool                                query_extension_availability(const char* extension);
        uint32_t                            query_memory_type(uint32_t type_bits, VkMemoryPropertyFlags properties, bool *found = nullptr);
        VkCommandPool                       create_commandpool(const uint32_t queue_familiy_index, VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        VkCommandBuffer                     create_commandbuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin = false);
        VkCommandBuffer                     create_commandbuffer(VkCommandBufferLevel level, bool begin = false);
        VkShaderModule                      load_shader_module(const char* working_path);
        void                                copy_buffer(buffer& dst, buffer& src, VkQueue queue, VkBufferCopy* copy_region = nullptr);
        void                                flush_command_buffer(VkCommandBuffer commandbuffer, VkQueue queue, VkCommandPool pool, bool free = true);
	    void                                flush_command_buffer(VkCommandBuffer commandbuffer, VkQueue queue, bool free = true);
        VkResult                            create_buffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags property, const VkDeviceSize size, VkBuffer* buffer, VkDeviceMemory* memory, void* data = nullptr);
        VkResult                            create_buffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags property, buffer* buffer, const VkDeviceSize size, void* data = nullptr);
    
        void                                destroy_shader_modules();
        void                                destroy_depth_stencil();
        void                                destroy_framebuffers();
        void                                destroy_renderpass();
        void                                destroy_buffer(buffer& buffer);
        void                                destroy();


        void                                setup_default_depth_stencil();
        void                                setup_default_renderpass();
        void                                setup_default_framebuffers();
        
    };  
}

#endif