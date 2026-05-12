#pragma once

#include <vector>
#include <string>

#include <glm/glm.hpp>

namespace chr {

    struct SceneRaw {
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
        };
        std::vector<Mesh> meshes;
        std::vector<Material> materials;
    };

}
