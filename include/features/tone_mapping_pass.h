#pragma once

#include <cstdint>

// Final post-processing: tone mapping + FXAA anti-aliasing
// Converts HDR scene to SDR display range
// Applies FXAA for edge smoothing
namespace chr {

    struct ToneMappingPass {
        uint32_t shader_program = 0;
        int uniform_scene_color = -1;
        int uniform_exposure = -1;
        int uniform_enable_fxaa = -1;
        float exposure = 1.0f;
        bool enable_fxaa = true;

        int init();
        int reload_shaders();
        void clear();
        void render(
            uint32_t texture_scene_color,
            uint32_t fullscreen_quad_vao,
            int width,
            int height);
    };

}
