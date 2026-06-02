#pragma once

#include <cstdint>

namespace chr {

    struct ToneMappingPass {
        uint32_t shader_program = 0;
        int uniform_scene_color = -1;
        int uniform_exposure = -1;
        float exposure = 1.0f;

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
