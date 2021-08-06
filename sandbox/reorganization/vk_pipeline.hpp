#ifndef VK_PIPELINE_HPP
#define VK_PIPELINE_HPP

#include "vk.hpp"

namespace vk
{
    struct pipeline
    {
        GPU*    gpu;

        void    prime_pipeline();
        void    try_enable_features();
        void    build_pipeline_commands();
        void    setup_descriptors();

        void    setup_uniform_buffers();
        void    update_uniform_buffers();

        void    render();

        void    cleanup();
    };
}

#endif