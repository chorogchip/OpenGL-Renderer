#pragma once

#include <vector>
#include <cstdint>

#include <glm/glm.hpp>

#include "scene_raw.h"

namespace chr {

    struct SceneDrawParams {
        glm::mat4 mat_model;
        glm::mat4 mat_view;
        glm::mat4 mat_projection;
    };

    struct SceneGPUResources {
        struct Mesh {
            uint32_t VAO;
            uint32_t VBO;
            uint32_t EBO;
            uint32_t index_count;
            uint32_t material_index;
        };
        struct Material {
            glm::vec4 base_color_factor = glm::vec4(1.0f);
            float metallic_factor = 1.0f;
            float roughness_factor = 1.0f;
            glm::vec3 emissive_factor = glm::vec3(0.0f);
            float alpha_cutoff = 0.5f;
            uint32_t alpha_mode = 0;
            uint32_t double_sided = 0;
            float normal_scale = 1.0f;
            float occlusion_strength = 1.0f;
            uint32_t texture_diffuse;
            uint32_t texture_normal;
            uint32_t texture_alpha_mask;
            uint32_t texture_metallic_roughness;
            uint32_t texture_occlusion;
            uint32_t texture_emissive;
        };
        std::vector<Mesh> meshes;
        std::vector<Material> materials;

        uint32_t shader_program = 0;
        uint32_t shadow_shader_program = 0;
        uint32_t fallback_texture_diffuse = 0;
        uint32_t fallback_texture_normal = 0;
        uint32_t fallback_texture_black = 0;
        int uniform_model = -1;
        int uniform_view = -1;
        int uniform_projection = -1;
        int uniform_texture_diffuse = -1;
        int uniform_texture_normal = -1;
        int uniform_texture_alpha_mask = -1;
        int uniform_texture_metallic_roughness = -1;
        int uniform_texture_occlusion = -1;
        int uniform_texture_emissive = -1;
        int uniform_base_color_factor = -1;
        int uniform_metallic_factor = -1;
        int uniform_roughness_factor = -1;
        int uniform_emissive_factor = -1;
        int uniform_alpha_cutoff = -1;
        int uniform_normal_scale = -1;
        int uniform_occlusion_strength = -1;
        int uniform_shadow_model = -1;
        int uniform_shadow_light_view_projection = -1;
        int uniform_shadow_texture_alpha_mask = -1;
    };

}
