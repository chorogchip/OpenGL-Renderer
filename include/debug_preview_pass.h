#pragma once

#include <cstdint>

namespace chr {

    struct DebugPreviewTextures {
        uint32_t albedo = 0;
        uint32_t normal = 0;
        uint32_t depth = 0;
        uint32_t ssao = 0;
        uint32_t material = 0;
    };

    struct DebugPreviewPass {
        uint32_t shader_program = 0;
        int uniform_texture = -1;
        int uniform_mode = -1;

        int init();
        void clear();
        void render(
            const DebugPreviewTextures& textures,
            uint32_t fullscreen_quad_vao,
            int width,
            int height);
    };

}
