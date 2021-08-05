#include "vk_rendertarget.hpp"

namespace vk
{
    void render_target::update_descriptor()
    {
        descriptor.sampler = sampler;
        descriptor.imageView = view;
        descriptor.imageLayout = image_layout;
    }

    void render_target::destroy()
    {
        vkDestroyImageView(gpu->device, view, gpu->allocation_callbacks);
        vkDestroyImage(gpu->device, image, gpu->allocation_callbacks);
        if (sampler)
            vkDestroySampler(gpu->device, sampler, gpu->allocation_callbacks);
        vkFreeMemory(gpu->device, memory, gpu->allocation_callbacks);
    }

    void render_target::load_from_buffer(void* buffer, const VkDeviceSize size, const VkFormat format,
            const uint32_t buffer_width, const uint32_t buffer_height, VkQueue copy_queue,
            VkFilter filter = VK_FILTER_LINEAR,
            VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT,
            VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {

    }
}