#pragma once

#include <cstdint>
#include <vector>
#include <string>

#include <glm/glm.hpp>

namespace chr {

    struct SceneRaw {
        enum class AlphaMode {
            Opaque,
            Mask,
            Blend
        };

        struct Mesh {
            struct Vertex {
                glm::vec3 position;
                glm::vec2 tex_coord;
                glm::vec3 normal;
                glm::vec3 tangent;
                glm::vec3 bitangent;
            };
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
            uint32_t material_index;
        };
        struct Material {
            std::string texture_diffuse;
            std::string texture_normal;
            std::string texture_alpha_mask;
            std::string texture_metallic;
            std::string texture_roughness;
            std::string texture_occlusion;
            std::string texture_emissive;
            glm::vec4 base_color_factor = glm::vec4(1.0f);
            glm::vec3 emissive_factor = glm::vec3(0.0f);
            float metallic_factor = 1.0f;
            float roughness_factor = 1.0f;
            float alpha_cutoff = 0.5f;
            AlphaMode alpha_mode = AlphaMode::Opaque;
            bool double_sided = false;
        };
        std::vector<Mesh> meshes;
        std::vector<Material> materials;
    };

}
