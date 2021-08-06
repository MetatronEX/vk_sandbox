#ifndef VK_PIPELINE_HPP
#define VK_PIPELINE_HPP

#include "vk.hpp"

namespace vk
{
    struct pipeline
    {
        GPU*    gpu;

        void    setup_queried_features();

        void    prime();

        void    setup_depth_stencil();
        void    setup_drawcommands();
        void    setup_renderpass();
        void    setup_framebuffers();

        void    prapare();
        void    update();
        void    render();

        void    cleanup();
    };
}

#endif