#include "model_loader.h"

#include <filesystem>
#include <initializer_list>
#include <iostream>
#include <utility>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>


namespace chr {

    namespace {

        glm::mat4 to_glm(const aiMatrix4x4& matrix) {
            return glm::transpose(glm::make_mat4(&matrix.a1));
        }

        glm::vec3 safe_normalize(const glm::vec3& value) {
            const float length = glm::length(value);
            if (length <= 0.000001f) {
                return glm::vec3(0.0f);
            }

            return value / length;
        }

        std::string get_texture_path(
            const aiScene* scene,
            aiMaterial* material,
            aiTextureType type,
            const std::filesystem::path& model_dir)
        {
            if (material == nullptr || material->GetTextureCount(type) == 0) {
                return {};
            }

            aiString path;
            if (material->GetTexture(type, 0, &path) != aiReturn_SUCCESS) {
                return {};
            }

            std::filesystem::path texture_path = path.C_Str();

            if (texture_path.is_absolute()) {
                return texture_path.string();
            }

            return (model_dir / texture_path).lexically_normal().string();
        }

        std::string get_first_texture_path(
            const aiScene* scene,
            aiMaterial* material,
            const std::initializer_list<aiTextureType> types,
            const std::filesystem::path& model_dir)
        {
            for (const aiTextureType type : types) {
                std::string texture_path = get_texture_path(scene, material, type, model_dir);
                if (!texture_path.empty()) {
                    return texture_path;
                }
            }

            return {};
        }

        void append_mesh(
            const aiMesh* mesh,
            const glm::mat4& transform,
            SceneRaw* result)
        {
            SceneRaw::Mesh out_mesh{};
            out_mesh.material_index = mesh->mMaterialIndex;

            const glm::mat3 normal_matrix = glm::inverseTranspose(glm::mat3(transform));

            out_mesh.vertices.reserve(mesh->mNumVertices);
            for (unsigned int j = 0; j < mesh->mNumVertices; ++j) {
                SceneRaw::Mesh::Vertex vertex{};

                const glm::vec4 position = transform * glm::vec4(
                    mesh->mVertices[j].x,
                    mesh->mVertices[j].y,
                    mesh->mVertices[j].z,
                    1.0f);
                vertex.position = glm::vec3(position);

                if (mesh->HasTextureCoords(0)) {
                    vertex.tex_coord = {
                        mesh->mTextureCoords[0][j].x,
                        mesh->mTextureCoords[0][j].y
                    };
                }
                else {
                    vertex.tex_coord = { 0.0f, 0.0f };
                }

                if (mesh->HasNormals()) {
                    vertex.normal = safe_normalize(normal_matrix * glm::vec3(
                        mesh->mNormals[j].x,
                        mesh->mNormals[j].y,
                        mesh->mNormals[j].z));
                }
                else {
                    vertex.normal = { 0.0f, 0.0f, 0.0f };
                }

                if (mesh->HasTangentsAndBitangents()) {
                    vertex.tangent = safe_normalize(normal_matrix * glm::vec3(
                        mesh->mTangents[j].x,
                        mesh->mTangents[j].y,
                        mesh->mTangents[j].z));
                    vertex.bitangent = safe_normalize(normal_matrix * glm::vec3(
                        mesh->mBitangents[j].x,
                        mesh->mBitangents[j].y,
                        mesh->mBitangents[j].z));
                }
                else {
                    vertex.tangent = { 0.0f, 0.0f, 0.0f };
                    vertex.bitangent = { 0.0f, 0.0f, 0.0f };
                }

                out_mesh.vertices.push_back(vertex);
            }

            out_mesh.indices.reserve(mesh->mNumFaces * 3);
            for (unsigned int j = 0; j < mesh->mNumFaces; ++j) {
                const aiFace& face = mesh->mFaces[j];

                for (unsigned int k = 0; k < face.mNumIndices; ++k) {
                    out_mesh.indices.push_back(face.mIndices[k]);
                }
            }

            result->meshes.push_back(std::move(out_mesh));
        }

        void append_node_meshes(
            const aiScene* scene,
            const aiNode* node,
            const glm::mat4& parent_transform,
            SceneRaw* result)
        {
            const glm::mat4 node_transform = parent_transform * to_glm(node->mTransformation);

            for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
                const unsigned int mesh_index = node->mMeshes[i];
                if (mesh_index < scene->mNumMeshes) {
                    append_mesh(scene->mMeshes[mesh_index], node_transform, result);
                }
            }

            for (unsigned int i = 0; i < node->mNumChildren; ++i) {
                append_node_meshes(scene, node->mChildren[i], node_transform, result);
            }
        }

    }

    SceneRaw load_scene(const char* filename) {
        SceneRaw result;
        Assimp::Importer importer;

        const aiScene* scene = importer.ReadFile(
            filename,
            aiProcess_Triangulate |
            aiProcess_FlipUVs |
            aiProcess_GenNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_JoinIdenticalVertices |
            aiProcess_ImproveCacheLocality);

        if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
            std::cerr << "Assimp Error: " << importer.GetErrorString() << std::endl;
            return result;
        }

        const std::filesystem::path model_path(filename);
        const std::filesystem::path model_dir = model_path.parent_path();

        result.materials.reserve(scene->mNumMaterials);
        for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
            aiMaterial* material = scene->mMaterials[i];

            SceneRaw::Material out_material{};

            out_material.texture_diffuse = get_first_texture_path(
                scene, material,
                { aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE },
                model_dir);

            out_material.texture_normal = get_first_texture_path(
                scene, material,
                { aiTextureType_NORMALS, aiTextureType_NORMAL_CAMERA, aiTextureType_HEIGHT },
                model_dir);

            out_material.texture_alpha_mask =
                get_texture_path(scene, material, aiTextureType_OPACITY, model_dir);

            out_material.texture_metallic =
                get_texture_path(scene, material, aiTextureType_METALNESS, model_dir);
            out_material.texture_roughness =
                get_texture_path(scene, material, aiTextureType_DIFFUSE_ROUGHNESS, model_dir);
            out_material.texture_occlusion =
                get_texture_path(scene, material, aiTextureType_AMBIENT_OCCLUSION, model_dir);
            out_material.texture_emissive =
                get_texture_path(scene, material, aiTextureType_EMISSIVE, model_dir);

            result.materials.push_back(std::move(out_material));
        }

        result.meshes.reserve(scene->mNumMeshes);
        append_node_meshes(scene, scene->mRootNode, glm::mat4(1.0f), &result);

        std::cout << "Successfully loaded: " << filename << std::endl;
        std::cout << "Total Meshes: " << result.meshes.size() << std::endl;
        std::cout << "Total Materials: " << result.materials.size() << std::endl;

        size_t total_vertices = 0;
        size_t total_indices = 0;
        for (const auto& mesh : result.meshes) {
            total_vertices += mesh.vertices.size();
            total_indices += mesh.indices.size();
        }

        std::cout << "Total Vertices: " << total_vertices << std::endl;
        std::cout << "Total Indices: " << total_indices << std::endl;

        return result;
    }

    SceneRaw load_obj(const char* filename) {
        return load_scene(filename);
    }

}
