#ifndef VK_PIPELINE_HPP
#define VK_PIPELINE_HPP

#include "vk.hpp"
#include "vk_gpu.hpp"

namespace vk
{
    template <typename draw_policy>
    struct pipeline : public draw_policy
    {
        void  setup_pipeline_queried_features()
        {
            setup_queried_features();
        }

        void  prime_pipeline()
        {
            prime();
        }

        void  setup_pipeline_depth_stencil()
        {
            setup_depth_stencil();
        }

        void  setup_pipeline_drawcommands()
        {
            setup_drawcommands();
        }

        void  setup_pipeline_renderpass()
        {
            setup_renderpass();
        }

        void  setup_pipeline_framebuffers()
        {
            setup_framebuffers();
        }

        void  prapare_pipeline()
        {
            prepare();
        }

        void  update_pipeline()
        {
            update();
        }

        void  spin_pipeline()
        {
            render();
        }

        void  pipeline_cleanup()
        {
            cleanup();
        }

    private:

        using draw_policy::setup_queried_features;
        using draw_policy::prime;
        using draw_policy::setup_depth_stencil;
        using draw_policy::setup_drawcommands;
        using draw_policy::setup_renderpass;
        using draw_policy::setup_framebuffers;
        using draw_policy::prepare;
        using draw_policy::update;
        using draw_policy::render;
        using draw_policy::cleanup;
    };
}

#endif