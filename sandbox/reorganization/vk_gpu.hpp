#ifndef VK_LOGICALDEVICE_HPP
#define VK_LOGICALDEVICE_HPP

#include "vk.hpp"
#include "vk_buffer.hpp"

namespace vk
{
    struct GPU 
    {
        VkPhysicalDevice                    physical_device;
        VkDevice                            device;

        VkPhysicalDeviceProperties2         device_properties;
        VkPhysicalDeviceMemoryProperties2   device_memory_properties;
        VkPhysicalDeviceFeatures2           device_features;
        VkPhysicalDeviceFeatures2           enabled_features;

        VkCommandPool                       commandpool;

        VkAllocationCallbacks*              allocation_callbacks

        queue_familiy_indices               queue_indices;

        std::vector<const char*>            enabled_device_extensions;

        VkResult        create();
        VkFormat        query_depth_format_support(const bool check_sampling_support);
        bool            query_extension_availability(const char* extension);
        uint32_t        query_memory_type(uint32_t type_bits, VkMemoryPropertyFlags properties, bool *found = nullptr);
        VkCommandPool   create_commandpool(const uint32_t queue_familiy_index, const VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        VkCommandBuffer create_commandbuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin = false);
        VkCommandBuffer create_commandbuffer(VkCommandBufferLevel level, bool begin = false);
        VkShaderModule  load_shader_module(const char* working_path);
        void            copy_buffer(buffer& dst, buffer& src, VkQueue queue, VkBufferCopy* copy_region = nullptr);
        void            flush_command_buffer(VkCommandBuffer commandbuffer, VkQueue queue, VkCommandPool pool, bool free = true);
	    void            flush_command_buffer(VkCommandBuffer commandbuffer, VkQueue queue, bool free = true);
        VkResult        create_buffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags property, const VkDeviceSize size, VkBuffer* buffer, VkDeviceMemory* memory, void* data = nullptr);
        VkResult        create_buffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags property, buffer* buffer, const VkDeviceSize size, void* data = nullptr);
    
        void            destroy_buffer(buffer& buffer);
        void            destroy();
    };  
}

#endif