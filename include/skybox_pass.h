#pragma once

#include <cstdint>
#include <string>

#include <glm/glm.hpp>

#include "graphics_util.h"

namespace chr {

    struct SkyboxPass {
        uint32_t equirectangular_texture = 0;
        uint32_t environment_cubemap = 0;
        uint32_t irradiance_cubemap = 0;
        uint32_t cube_vao = 0;
        uint32_t cube_vbo = 0;
        uint32_t equirectangular_shader_program = 0;
        uint32_t irradiance_shader_program = 0;
        uint32_t skybox_shader_program = 0;
        int uniform_equirectangular_map = -1;
        int uniform_capture_projection = -1;
        int uniform_capture_view = -1;
        int uniform_irradiance_environment_map = -1;
        int uniform_irradiance_projection = -1;
        int uniform_irradiance_view = -1;
        int uniform_skybox_map = -1;
        int uniform_projection = -1;
        int uniform_view = -1;
        int cubemap_size = 512;
        int irradiance_size = 32;

        int init(const std::string& hdr_environment_path);
        void clear();
        bool is_ready() const;
        void render(
            uint32_t framebuffer,
            int width,
            int height,
            const glm::mat4& projection,
            const glm::mat4& view);
    };

}
