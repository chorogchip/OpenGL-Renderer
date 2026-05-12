#include "gltf_scene_loader.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <utility>

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace chr {

    namespace {

        constexpr uint32_t INVALID_MATERIAL_INDEX = std::numeric_limits<uint32_t>::max();

        glm::mat4 to_glm(const fastgltf::math::fmat4x4& matrix) {
            return glm::make_mat4(matrix.data());
        }

        glm::vec3 safe_normalize(const glm::vec3& value) {
            const float length = glm::length(value);
            if (length <= 0.000001f) {
                return glm::vec3(0.0f);
            }

            return value / length;
        }

        template <typename OptionalTextureInfoType>
        std::string get_texture_image_path(
            const fastgltf::Asset& asset,
            const std::filesystem::path& model_dir,
            const OptionalTextureInfoType& texture_info) {
            if (!texture_info.has_value() || texture_info->textureIndex >= asset.textures.size()) {
                return {};
            }

            const fastgltf::Texture& texture = asset.textures[texture_info->textureIndex];
            if (!texture.imageIndex.has_value() || *texture.imageIndex >= asset.images.size()) {
                return {};
            }

            const fastgltf::Image& image = asset.images[*texture.imageIndex];
            const auto* uri_source = std::get_if<fastgltf::sources::URI>(&image.data);
            if (uri_source == nullptr || !uri_source->uri.isLocalPath()) {
                return {};
            }

            std::filesystem::path texture_path = uri_source->uri.fspath();
            if (texture_path.is_absolute()) {
                return texture_path.string();
            }

            return (model_dir / texture_path).lexically_normal().string();
        }

        SceneRaw::Material convert_material(
            const fastgltf::Asset& asset,
            const fastgltf::Material& material,
            const std::filesystem::path& model_dir) {
            SceneRaw::Material out_material{};

            out_material.texture_diffuse = get_texture_image_path(
                asset, model_dir, material.pbrData.baseColorTexture);
            out_material.texture_normal = get_texture_image_path(
                asset, model_dir, material.normalTexture);
            out_material.texture_roughness = get_texture_image_path(
                asset, model_dir, material.pbrData.metallicRoughnessTexture);
            out_material.texture_metallic = out_material.texture_roughness;
            out_material.texture_occlusion = get_texture_image_path(
                asset, model_dir, material.occlusionTexture);
            out_material.texture_emissive = get_texture_image_path(
                asset, model_dir, material.emissiveTexture);

            if (material.alphaMode == fastgltf::AlphaMode::Mask ||
                material.alphaMode == fastgltf::AlphaMode::Blend) {
                out_material.texture_alpha_mask = out_material.texture_diffuse;
            }

            return out_material;
        }

        void append_gltf_primitive(
            const fastgltf::Asset& asset,
            const fastgltf::Primitive& primitive,
            const glm::mat4& transform,
            SceneRaw* result) {
            if (primitive.type != fastgltf::PrimitiveType::Triangles) {
                return;
            }

            const auto position_it = primitive.findAttribute("POSITION");
            if (position_it == primitive.attributes.end() ||
                position_it->accessorIndex >= asset.accessors.size()) {
                return;
            }

            const fastgltf::Accessor& position_accessor = asset.accessors[position_it->accessorIndex];
            const glm::mat3 normal_matrix = glm::inverseTranspose(glm::mat3(transform));

            SceneRaw::Mesh out_mesh{};
            out_mesh.material_index = primitive.materialIndex.has_value()
                ? static_cast<uint32_t>(*primitive.materialIndex)
                : INVALID_MATERIAL_INDEX;
            out_mesh.vertices.resize(position_accessor.count);

            fastgltf::iterateAccessorWithIndex<glm::vec3>(
                asset, position_accessor,
                [&](const glm::vec3 position, const std::size_t index) {
                    out_mesh.vertices[index].position = glm::vec3(transform * glm::vec4(position, 1.0f));
                });

            const auto texcoord_it = primitive.findAttribute("TEXCOORD_0");
            if (texcoord_it != primitive.attributes.end() &&
                texcoord_it->accessorIndex < asset.accessors.size()) {
                const fastgltf::Accessor& texcoord_accessor = asset.accessors[texcoord_it->accessorIndex];
                fastgltf::iterateAccessorWithIndex<glm::vec2>(
                    asset, texcoord_accessor,
                    [&](const glm::vec2 tex_coord, const std::size_t index) {
                        out_mesh.vertices[index].tex_coord = tex_coord;
                    });
            }

            const auto normal_it = primitive.findAttribute("NORMAL");
            if (normal_it != primitive.attributes.end() &&
                normal_it->accessorIndex < asset.accessors.size()) {
                const fastgltf::Accessor& normal_accessor = asset.accessors[normal_it->accessorIndex];
                fastgltf::iterateAccessorWithIndex<glm::vec3>(
                    asset, normal_accessor,
                    [&](const glm::vec3 normal, const std::size_t index) {
                        out_mesh.vertices[index].normal = safe_normalize(normal_matrix * normal);
                    });
            }

            const auto tangent_it = primitive.findAttribute("TANGENT");
            if (tangent_it != primitive.attributes.end() &&
                tangent_it->accessorIndex < asset.accessors.size()) {
                const fastgltf::Accessor& tangent_accessor = asset.accessors[tangent_it->accessorIndex];
                fastgltf::iterateAccessorWithIndex<glm::vec4>(
                    asset, tangent_accessor,
                    [&](const glm::vec4 tangent, const std::size_t index) {
                        SceneRaw::Mesh::Vertex& vertex = out_mesh.vertices[index];
                        vertex.tangent = safe_normalize(normal_matrix * glm::vec3(tangent));
                        vertex.bitangent = safe_normalize(glm::cross(vertex.normal, vertex.tangent) * tangent.w);
                    });
            }

            if (primitive.indicesAccessor.has_value() &&
                *primitive.indicesAccessor < asset.accessors.size()) {
                const fastgltf::Accessor& indices_accessor = asset.accessors[*primitive.indicesAccessor];
                out_mesh.indices.reserve(indices_accessor.count);
                fastgltf::iterateAccessor<uint32_t>(
                    asset, indices_accessor,
                    [&](const uint32_t index) {
                        out_mesh.indices.push_back(index);
                    });
            }
            else {
                out_mesh.indices.reserve(out_mesh.vertices.size());
                for (uint32_t i = 0; i < out_mesh.vertices.size(); ++i) {
                    out_mesh.indices.push_back(i);
                }
            }

            result->meshes.push_back(std::move(out_mesh));
        }

        void append_scene_meshes(const fastgltf::Asset& asset, SceneRaw* result) {
            const std::size_t scene_index = asset.defaultScene.value_or(0);
            if (scene_index >= asset.scenes.size()) {
                return;
            }

            fastgltf::iterateSceneNodes(
                asset,
                scene_index,
                fastgltf::math::fmat4x4(),
                [&](const fastgltf::Node& node, fastgltf::math::fmat4x4 matrix) {
                    if (!node.meshIndex.has_value() || *node.meshIndex >= asset.meshes.size()) {
                        return;
                    }

                    const glm::mat4 transform = to_glm(matrix);
                    const fastgltf::Mesh& mesh = asset.meshes[*node.meshIndex];
                    for (const fastgltf::Primitive& primitive : mesh.primitives) {
                        append_gltf_primitive(asset, primitive, transform, result);
                    }
                });
        }

        void log_scene_summary(const std::filesystem::path& path, const SceneRaw& scene) {
            std::cout << "Successfully loaded: " << path.string() << std::endl;
            std::cout << "Total Meshes: " << scene.meshes.size() << std::endl;
            std::cout << "Total Materials: " << scene.materials.size() << std::endl;

            size_t total_vertices = 0;
            size_t total_indices = 0;
            for (const auto& mesh : scene.meshes) {
                total_vertices += mesh.vertices.size();
                total_indices += mesh.indices.size();
            }

            std::cout << "Total Vertices: " << total_vertices << std::endl;
            std::cout << "Total Indices: " << total_indices << std::endl;
        }

    }

    bool is_gltf_scene_path(const std::filesystem::path& path) {
        const std::string extension = path.extension().string();
        return extension == ".gltf" || extension == ".glb" ||
            extension == ".GLTF" || extension == ".GLB";
    }

    SceneRaw load_gltf_scene(const std::filesystem::path& path) {
        SceneRaw result;
        const std::filesystem::path model_dir = path.parent_path();

        auto gltf_data = fastgltf::GltfDataBuffer::FromPath(path);
        if (gltf_data.error() != fastgltf::Error::None) {
            std::cerr << "fastgltf file error: " << fastgltf::getErrorMessage(gltf_data.error()) << std::endl;
            return result;
        }

        fastgltf::Parser parser;
        auto asset_result = parser.loadGltf(
            gltf_data.get(),
            model_dir,
            fastgltf::Options::LoadExternalBuffers | fastgltf::Options::GenerateMeshIndices,
            fastgltf::Category::OnlyRenderable);
        if (asset_result.error() != fastgltf::Error::None) {
            std::cerr << "fastgltf parse error: " << fastgltf::getErrorMessage(asset_result.error()) << std::endl;
            return result;
        }

        const fastgltf::Asset& asset = asset_result.get();

        result.materials.reserve(asset.materials.size());
        for (const fastgltf::Material& material : asset.materials) {
            result.materials.push_back(convert_material(asset, material, model_dir));
        }

        result.meshes.reserve(asset.meshes.size());
        append_scene_meshes(asset, &result);

        log_scene_summary(path, result);
        return result;
    }

}
