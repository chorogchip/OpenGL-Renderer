#pragma once

#include <array>
#include <cstdint>

#include <glm/glm.hpp>

#include "render_lights.h"

namespace chr {

    struct DeferredLightingInputs {
        uint32_t framebuffer = 0;
        uint32_t texture_albedo = 0;
        uint32_t texture_normal = 0;
        uint32_t texture_material = 0;
        uint32_t texture_depth = 0;
        uint32_t texture_shadow_depth = 0;
        uint32_t texture_ssao = 0;
        uint32_t fullscreen_quad_vao = 0;
        int width = 0;
        int height = 0;
        glm::mat4 mat_projection = glm::mat4(1.0f);
        glm::mat4 mat_view = glm::mat4(1.0f);
        glm::mat4 mat_light_view_projection = glm::mat4(1.0f);
        std::array<PointLightDesc, 5> point_lights = {};
    };

    struct DeferredLightingPass {
        uint32_t shader_program = 0;
        int uniform_g_albedo = -1;
        int uniform_g_normal = -1;
        int uniform_g_material = -1;
        int uniform_g_depth = -1;
        int uniform_shadow_map = -1;
        int uniform_ssao_map = -1;
        int uniform_inverse_projection = -1;
        int uniform_inverse_view = -1;
        int uniform_light_view_projection = -1;
        int uniform_light_direction = -1;
        int uniform_light_color = -1;
        int uniform_ambient_strength = -1;
        int uniform_diffuse_strength = -1;
        int uniform_point_light_count = -1;
        std::array<int, 5> uniform_point_light_positions = { -1, -1, -1, -1, -1 };
        std::array<int, 5> uniform_point_light_colors = { -1, -1, -1, -1, -1 };
        std::array<int, 5> uniform_point_light_intensities = { -1, -1, -1, -1, -1 };
        std::array<int, 5> uniform_point_light_ranges = { -1, -1, -1, -1, -1 };

        int init();
        void clear();
        void render(const DeferredLightingInputs& inputs);
    };

}
