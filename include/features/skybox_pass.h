#pragma once

#include <cstdint>
#include <string>

#include <glm/glm.hpp>

#include "core/graphics.h"

// HDR environment mapping and IBL preprocessing
// - Loads equirectangular HDR and converts to cubemap
// - Pre-computes irradiance cubemap (diffuse IBL)
// - Pre-computes specular prefiltered maps (specular IBL)
// - Generates BRDF lookup table
namespace chr {

    struct SkyboxPass {
        uint32_t equirectangular_texture = 0;
        uint32_t environment_cubemap = 0;
        uint32_t irradiance_cubemap = 0;
        uint32_t prefiltered_environment_cubemap = 0;
        uint32_t brdf_lut_texture = 0;
        uint32_t cube_vao = 0;
        uint32_t cube_vbo = 0;
        uint32_t brdf_quad_vao = 0;
        uint32_t brdf_quad_vbo = 0;
        uint32_t equirectangular_shader_program = 0;
        uint32_t irradiance_shader_program = 0;
        uint32_t prefilter_shader_program = 0;
        uint32_t brdf_lut_shader_program = 0;
        uint32_t skybox_shader_program = 0;
        int uniform_equirectangular_map = -1;
        int uniform_capture_projection = -1;
        int uniform_capture_view = -1;
        int uniform_irradiance_environment_map = -1;
        int uniform_irradiance_projection = -1;
        int uniform_irradiance_view = -1;
        int uniform_prefilter_environment_map = -1;
        int uniform_prefilter_projection = -1;
        int uniform_prefilter_view = -1;
        int uniform_prefilter_roughness = -1;
        int uniform_skybox_map = -1;
        int uniform_projection = -1;
        int uniform_view = -1;
        int cubemap_size = 512;
        int irradiance_size = 32;
        int prefilter_size = 128;
        int prefilter_mip_levels = 5;
        int brdf_lut_size = 512;
        std::string hdr_environment_path;

        int init(const std::string& hdr_environment_path);
        int reload_shaders();
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
