#pragma once

#include <cstdint>

// Final post-processing: tone mapping (HDR → SDR) + FXAA anti-aliasing
// Pipeline: tone map to intermediate texture, then apply FXAA
namespace chr {

    struct ToneMappingPass {
        uint32_t tone_map_shader = 0;
        uint32_t fxaa_shader = 0;
        uint32_t framebuffer_tone_mapped = 0;
        uint32_t texture_tone_mapped = 0;
        int uniform_scene_color_tone_map = -1;
        int uniform_exposure_tone_map = -1;
        int uniform_scene_color_fxaa = -1;
        float exposure = 1.0f;
        bool enable_fxaa = true;
        int width = 0;
        int height = 0;

        int init();
        int reload_shaders();
        void clear();
        void render(
            uint32_t texture_scene_color,
            uint32_t fullscreen_quad_vao,
            int width,
            int height);

    private:
        int init_tone_map_resources(int width, int height);
        void clear_tone_map_resources();
    };

}
