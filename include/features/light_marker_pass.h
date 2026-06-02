#pragma once

#include <array>
#include <cstdint>

#include <glm/glm.hpp>

#include "features/render_lights.h"

// Visualizes point light positions as small spheres (debug)
// Drawn as wireframe to avoid occluding scene
namespace chr {

    struct LightMarkerPass {
        uint32_t vao = 0;
        uint32_t vbo = 0;
        uint32_t shader_program = 0;
        int uniform_model = -1;
        int uniform_view = -1;
        int uniform_projection = -1;
        int uniform_color = -1;

        int init();
        int reload_shaders();
        void clear();
        void render(
            const std::array<PointLightDesc, 5>& point_lights,
            const glm::mat4& mat_projection,
            const glm::mat4& mat_view,
            int width,
            int height);
    };

}
