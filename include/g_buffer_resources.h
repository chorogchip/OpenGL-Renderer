#pragma once

#include <cstdint>

#include <glm/glm.hpp>

namespace chr {

    struct GBufferResources {
        uint32_t framebuffer = 0;
        uint32_t texture_albedo = 0;
        uint32_t texture_normal = 0;
        uint32_t texture_depth = 0;
        uint32_t quad_vao = 0;
        uint32_t quad_vbo = 0;
        uint32_t lighting_shader_program = 0;
        uint32_t debug_shader_program = 0;
        int width = 0;
        int height = 0;
        int uniform_g_albedo = -1;
        int uniform_g_normal = -1;
        int uniform_g_depth = -1;
        int uniform_inverse_projection = -1;
        int uniform_light_direction = -1;
        int uniform_light_color = -1;
        int uniform_ambient_strength = -1;
        int uniform_diffuse_strength = -1;
        int uniform_debug_texture = -1;
        int uniform_debug_mode = -1;

        int init(int width, int height);
        int resize(int width, int height);
        void clear();
        void bind_for_geometry_pass();
        void draw_lighting_pass(const glm::mat4& mat_projection, const glm::mat4& mat_view);
        void draw_debug_views();
        static void bind_default_framebuffer(int width, int height);
    };

}
