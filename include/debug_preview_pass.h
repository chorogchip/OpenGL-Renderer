#pragma once

#include <cstdint>

namespace chr {

    enum class DebugViewMode : int {
        Final = 0,
        Albedo,
        Normal,
        Depth,
        Ssao,
        Metallic,
        Roughness,
        Ao,
        Emissive,
        Environment,
        Irradiance,
        Prefilter,
        BrdfLut,
        Count
    };

    struct DebugPreviewTextures {
        uint32_t albedo = 0;
        uint32_t normal = 0;
        uint32_t depth = 0;
        uint32_t ssao = 0;
        uint32_t material = 0;
        uint32_t emissive = 0;
        uint32_t environment = 0;
        uint32_t irradiance = 0;
        uint32_t prefilter = 0;
        uint32_t brdf_lut = 0;
    };

    struct DebugPreviewPass {
        uint32_t shader_program = 0;
        int uniform_texture = -1;
        int uniform_cubemap_texture = -1;
        int uniform_mode = -1;

        int init();
        void clear();
        void render(
            const DebugPreviewTextures& textures,
            DebugViewMode mode,
            uint32_t fullscreen_quad_vao,
            int width,
            int height);
    };

}
