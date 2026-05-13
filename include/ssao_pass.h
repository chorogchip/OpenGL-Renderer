#pragma once

#include <array>
#include <cstdint>

#include <glm/glm.hpp>

namespace chr {

    struct SSAOPass {
        uint32_t framebuffer = 0;
        uint32_t blur_framebuffer = 0;
        uint32_t texture_ssao = 0;
        uint32_t texture_ssao_blur = 0;
        uint32_t texture_noise = 0;
        uint32_t shader_program = 0;
        uint32_t blur_shader_program = 0;
        int width = 0;
        int height = 0;
        int uniform_g_normal = -1;
        int uniform_g_depth = -1;
        int uniform_noise_texture = -1;
        int uniform_projection = -1;
        int uniform_inverse_projection = -1;
        int uniform_noise_scale = -1;
        std::array<int, 16> uniform_samples = {
            -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1
        };
        int uniform_blur_texture = -1;

        int init(int width, int height);
        int resize(int width, int height);
        void clear();
        void render(
            uint32_t texture_normal,
            uint32_t texture_depth,
            uint32_t fullscreen_quad_vao,
            const glm::mat4& mat_projection);
        uint32_t blurred_texture() const;
    };

}
