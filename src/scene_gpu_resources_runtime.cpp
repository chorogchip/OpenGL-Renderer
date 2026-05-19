#include "scene_gpu_resources_runtime.h"

#include <chrono>
#include <cstddef>
#include <future>
#include <iostream>

#include <glad/glad.h>

#include "graphics_util.h"

namespace {
    constexpr const char* VERTEX_SHADER_PATH = "assets/shaders/scene.vert";
    constexpr const char* FRAGMENT_SHADER_PATH = "assets/shaders/scene.frag";
    constexpr const char* SHADOW_VERTEX_SHADER_PATH = "assets/shaders/shadow_scene.vert";
    constexpr const char* SHADOW_FRAGMENT_SHADER_PATH = "assets/shaders/shadow_scene.frag";
    constexpr float GPU_PROGRESS_SHADER_WEIGHT = 0.10f;
    constexpr float GPU_PROGRESS_MESH_WEIGHT = 0.30f;
    constexpr float GPU_PROGRESS_MATERIAL_WEIGHT = 0.60f;

    static uint32_t create_fallback_texture(const unsigned char rgba[4]) {
        uint32_t texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA,
            1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
        return texture;
    }

    static void report_progress(
        const chr::SceneGPUProgressCallback& progress_callback,
        const float progress,
        const char* message) {
        if (progress_callback) {
            progress_callback(progress, message);
        }
    }

    static float calculate_mesh_progress(const chr::SceneGPUInitState& state) {
        if (state.scene_raw == nullptr || state.scene_raw->meshes.empty()) {
            return GPU_PROGRESS_SHADER_WEIGHT + GPU_PROGRESS_MESH_WEIGHT;
        }

        return GPU_PROGRESS_SHADER_WEIGHT +
            GPU_PROGRESS_MESH_WEIGHT *
            (static_cast<float>(state.mesh_index) / static_cast<float>(state.scene_raw->meshes.size()));
    }

    static float calculate_material_progress(const chr::SceneGPUInitState& state) {
        if (state.scene_raw == nullptr || state.scene_raw->materials.empty()) {
            return 1.0f;
        }

        constexpr float MATERIAL_TEXTURE_STEPS = 7.0f;
        const float completed_steps =
            static_cast<float>(state.material_index) * MATERIAL_TEXTURE_STEPS +
            static_cast<float>(state.material_texture_step);
        const float total_steps =
            static_cast<float>(state.scene_raw->materials.size()) * MATERIAL_TEXTURE_STEPS;
        return GPU_PROGRESS_SHADER_WEIGHT + GPU_PROGRESS_MESH_WEIGHT +
            GPU_PROGRESS_MATERIAL_WEIGHT * (completed_steps / total_steps);
    }

    static std::string make_texture_cache_key(
        const std::string& path,
        const graphics_util::TextureColorSpace color_space) {
        const char* color_space_name =
            color_space == graphics_util::TextureColorSpace::Srgb ? "srgb" : "linear";
        return path + "\n" + color_space_name;
    }

    static bool init_scene_gpu_shaders(chr::SceneGPUResources* resources) {

        unsigned shader_program = graphics_util::create_shader_program_from_files(
            VERTEX_SHADER_PATH, FRAGMENT_SHADER_PATH);
        if (shader_program == 0) {
            return false;
        }
        resources->shader_program = shader_program;
        resources->uniform_model = glGetUniformLocation(resources->shader_program, "model");
        resources->uniform_view = glGetUniformLocation(resources->shader_program, "view");
        resources->uniform_projection = glGetUniformLocation(resources->shader_program, "projection");
        resources->uniform_texture_diffuse = glGetUniformLocation(resources->shader_program, "uTexture");
        resources->uniform_texture_normal = glGetUniformLocation(resources->shader_program, "uNormalTexture");
        resources->uniform_texture_alpha_mask = glGetUniformLocation(resources->shader_program, "uAlphaMaskTexture");
        resources->uniform_texture_metallic_roughness =
            glGetUniformLocation(resources->shader_program, "uMetallicRoughnessTexture");
        resources->uniform_texture_occlusion = glGetUniformLocation(resources->shader_program, "uOcclusionTexture");
        resources->uniform_texture_emissive = glGetUniformLocation(resources->shader_program, "uEmissiveTexture");
        resources->uniform_base_color_factor = glGetUniformLocation(resources->shader_program, "uBaseColorFactor");
        resources->uniform_metallic_factor = glGetUniformLocation(resources->shader_program, "uMetallicFactor");
        resources->uniform_roughness_factor = glGetUniformLocation(resources->shader_program, "uRoughnessFactor");
        resources->uniform_emissive_factor = glGetUniformLocation(resources->shader_program, "uEmissiveFactor");
        resources->uniform_alpha_cutoff = glGetUniformLocation(resources->shader_program, "uAlphaCutoff");
        resources->uniform_normal_scale = glGetUniformLocation(resources->shader_program, "uNormalScale");
        resources->uniform_occlusion_strength = glGetUniformLocation(resources->shader_program, "uOcclusionStrength");

        resources->shadow_shader_program = graphics_util::create_shader_program_from_files(
            SHADOW_VERTEX_SHADER_PATH, SHADOW_FRAGMENT_SHADER_PATH);
        if (resources->shadow_shader_program == 0) {
            return false;
        }
        resources->uniform_shadow_model = glGetUniformLocation(resources->shadow_shader_program, "model");
        resources->uniform_shadow_light_view_projection =
            glGetUniformLocation(resources->shadow_shader_program, "uLightViewProjection");
        resources->uniform_shadow_texture_alpha_mask =
            glGetUniformLocation(resources->shadow_shader_program, "uAlphaMaskTexture");

        constexpr unsigned char white_pixel[] = { 255, 255, 255, 255 };
        constexpr unsigned char black_pixel[] = { 0, 0, 0, 255 };
        constexpr unsigned char flat_normal_pixel[] = { 128, 128, 255, 255 };
        resources->fallback_texture_diffuse = create_fallback_texture(white_pixel);
        resources->fallback_texture_normal = create_fallback_texture(flat_normal_pixel);
        resources->fallback_texture_black = create_fallback_texture(black_pixel);
        return true;
    }

    static void upload_scene_mesh(
        chr::SceneGPUResources* resources,
        const chr::SceneRaw::Mesh& mesh_raw) {
        unsigned VAO, VBO, EBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        resources->meshes.push_back({ VAO, VBO, EBO,
            static_cast<uint32_t>(mesh_raw.indices.size()),
            mesh_raw.material_index });

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, mesh_raw.vertices.size() * sizeof(mesh_raw.vertices[0]),
            mesh_raw.vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh_raw.indices.size() * sizeof(mesh_raw.indices[0]),
            mesh_raw.indices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(chr::SceneRaw::Mesh::Vertex),
            (void*)offsetof(chr::SceneRaw::Mesh::Vertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(chr::SceneRaw::Mesh::Vertex),
            (void*)offsetof(chr::SceneRaw::Mesh::Vertex, tex_coord));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(chr::SceneRaw::Mesh::Vertex),
            (void*)offsetof(chr::SceneRaw::Mesh::Vertex, normal));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(chr::SceneRaw::Mesh::Vertex),
            (void*)offsetof(chr::SceneRaw::Mesh::Vertex, tangent));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(chr::SceneRaw::Mesh::Vertex),
            (void*)offsetof(chr::SceneRaw::Mesh::Vertex, bitangent));
        glBindVertexArray(0);
    }

    static bool upload_texture_when_decoded(
        chr::SceneGPUResources* resources,
        chr::SceneGPUInitState* state,
        const std::string& path,
        const graphics_util::TextureColorSpace color_space,
        uint32_t* texture) {
        if (path.empty()) {
            *texture = 0;
            return true;
        }

        const std::string cache_key = make_texture_cache_key(path, color_space);
        const auto cached_texture = state->texture_cache.find(cache_key);
        if (cached_texture != state->texture_cache.end()) {
            *texture = cached_texture->second;
            return true;
        }

        if (!state->pending_texture_decode.valid()) {
            state->pending_texture_path = path;
            state->pending_texture_decode = std::async(
                std::launch::async,
                [path]() {
                    return graphics_util::load_texture_image(path);
                });
            state->message = "Decoding material texture...";
            return false;
        }

        if (state->pending_texture_decode.wait_for(std::chrono::seconds(0)) !=
            std::future_status::ready) {
            state->message = "Decoding material texture...";
            return false;
        }

        graphics_util::TextureImage image = state->pending_texture_decode.get();
        state->pending_texture_path.clear();
        *texture = graphics_util::create_texture_2d(image, color_space);
        if (*texture != 0) {
            state->texture_cache[cache_key] = *texture;
            resources->owned_texture_handles.push_back(*texture);
        }
        state->message = "Uploading material textures...";
        return true;
    }
}

namespace chr {

    void begin_scene_gpu_resources_init(
        SceneGPUResources* resources,
        const SceneRaw& scene_raw,
        SceneGPUInitState* state) {
        clear_scene_gpu_resources(resources);
        *state = {};
        state->scene_raw = &scene_raw;
    }

    bool step_scene_gpu_resources_init(SceneGPUResources* resources, SceneGPUInitState* state) {
        if (state == nullptr || state->scene_raw == nullptr) {
            return false;
        }

        const SceneRaw& scene_raw = *state->scene_raw;

        if (state->phase == SceneGPUInitPhase::Shaders) {
            state->message = "Compiling scene shaders...";
            if (!init_scene_gpu_shaders(resources)) {
                clear_scene_gpu_resources(resources);
                state->phase = SceneGPUInitPhase::Failed;
                state->message = "Failed to create scene shaders.";
                return false;
            }

            state->phase = scene_raw.meshes.empty()
                ? SceneGPUInitPhase::Materials
                : SceneGPUInitPhase::Meshes;
            state->progress = GPU_PROGRESS_SHADER_WEIGHT;
            state->message = "Uploading mesh buffers...";
            return true;
        }

        if (state->phase == SceneGPUInitPhase::Meshes) {
            if (state->mesh_index < scene_raw.meshes.size()) {
                upload_scene_mesh(resources, scene_raw.meshes[state->mesh_index]);
                ++state->mesh_index;
                state->progress = calculate_mesh_progress(*state);
                state->message = "Uploading mesh buffers...";
                return true;
            }

            state->phase = SceneGPUInitPhase::Materials;
            state->progress = GPU_PROGRESS_SHADER_WEIGHT + GPU_PROGRESS_MESH_WEIGHT;
            state->message = "Uploading material textures...";
            return true;
        }

        if (state->phase == SceneGPUInitPhase::Materials) {
            if (state->material_index >= scene_raw.materials.size()) {
                state->phase = SceneGPUInitPhase::Complete;
                state->progress = 1.0f;
                state->message = "Scene ready.";
                return true;
            }

            const SceneRaw::Material& material_raw = scene_raw.materials[state->material_index];
            if (state->material_texture_step == 0) {
                if (!upload_texture_when_decoded(
                    resources,
                    state,
                    material_raw.texture_diffuse,
                    graphics_util::TextureColorSpace::Srgb,
                    &state->pending_texture_diffuse)) {
                    state->progress = calculate_material_progress(*state);
                    return true;
                }
                state->material_texture_step = 1;
            }
            else if (state->material_texture_step == 1) {
                if (!upload_texture_when_decoded(
                    resources,
                    state,
                    material_raw.texture_normal,
                    graphics_util::TextureColorSpace::Linear,
                    &state->pending_texture_normal)) {
                    state->progress = calculate_material_progress(*state);
                    return true;
                }
                state->material_texture_step = 2;
            }
            else if (state->material_texture_step == 2) {
                if (!upload_texture_when_decoded(
                    resources,
                    state,
                    material_raw.texture_alpha_mask,
                    graphics_util::TextureColorSpace::Linear,
                    &state->pending_texture_alpha_mask)) {
                    state->progress = calculate_material_progress(*state);
                    return true;
                }
                state->material_texture_step = 3;
            }
            else if (state->material_texture_step == 3) {
                if (!upload_texture_when_decoded(
                    resources,
                    state,
                    material_raw.texture_metallic_roughness,
                    graphics_util::TextureColorSpace::Linear,
                    &state->pending_texture_metallic_roughness)) {
                    state->progress = calculate_material_progress(*state);
                    return true;
                }
                state->material_texture_step = 4;
            }
            else if (state->material_texture_step == 4) {
                if (!upload_texture_when_decoded(
                    resources,
                    state,
                    material_raw.texture_occlusion,
                    graphics_util::TextureColorSpace::Linear,
                    &state->pending_texture_occlusion)) {
                    state->progress = calculate_material_progress(*state);
                    return true;
                }
                state->material_texture_step = 5;
            }
            else if (state->material_texture_step == 5) {
                if (!upload_texture_when_decoded(
                    resources,
                    state,
                    material_raw.texture_emissive,
                    graphics_util::TextureColorSpace::Srgb,
                    &state->pending_texture_emissive)) {
                    state->progress = calculate_material_progress(*state);
                    return true;
                }
                state->material_texture_step = 6;
            }
            else {
                resources->materials.push_back({
                    material_raw.base_color_factor,
                    material_raw.metallic_factor,
                    material_raw.roughness_factor,
                    material_raw.emissive_factor,
                    material_raw.alpha_cutoff,
                    static_cast<uint32_t>(material_raw.alpha_mode),
                    material_raw.double_sided ? 1u : 0u,
                    material_raw.normal_scale,
                    material_raw.occlusion_strength,
                    state->pending_texture_diffuse != 0
                        ? state->pending_texture_diffuse
                        : resources->fallback_texture_diffuse,
                    state->pending_texture_normal != 0
                        ? state->pending_texture_normal
                        : resources->fallback_texture_normal,
                    state->pending_texture_alpha_mask != 0
                        ? state->pending_texture_alpha_mask
                        : resources->fallback_texture_diffuse,
                    state->pending_texture_metallic_roughness != 0
                        ? state->pending_texture_metallic_roughness
                        : resources->fallback_texture_diffuse,
                    state->pending_texture_occlusion != 0
                        ? state->pending_texture_occlusion
                        : resources->fallback_texture_diffuse,
                    state->pending_texture_emissive != 0
                        ? state->pending_texture_emissive
                        : resources->fallback_texture_black
                });
                state->pending_texture_diffuse = 0;
                state->pending_texture_normal = 0;
                state->pending_texture_alpha_mask = 0;
                state->pending_texture_metallic_roughness = 0;
                state->pending_texture_occlusion = 0;
                state->pending_texture_emissive = 0;
                state->material_texture_step = 0;
                ++state->material_index;
            }

            state->progress = calculate_material_progress(*state);
            state->message = "Uploading material textures...";
            return true;
        }

        return state->phase == SceneGPUInitPhase::Complete;
    }

    int init_scene_gpu_resources(
        SceneGPUResources* resources,
        const SceneRaw& scene_raw,
        const SceneGPUProgressCallback& progress_callback) {
        SceneGPUInitState state;
        begin_scene_gpu_resources_init(resources, scene_raw, &state);

        while (state.phase != SceneGPUInitPhase::Complete) {
            if (!step_scene_gpu_resources_init(resources, &state)) {
                return -1;
            }
            report_progress(progress_callback, state.progress, state.message);
        }

        return 0;
    }

    void clear_scene_gpu_resources(SceneGPUResources* resources) {
        for (const auto& mesh : resources->meshes) {
            glDeleteVertexArrays(1, &mesh.VAO);
            glDeleteBuffers(1, &mesh.VBO);
            glDeleteBuffers(1, &mesh.EBO);
        }
        resources->meshes.clear();

        resources->materials.clear();
        for (const uint32_t texture : resources->owned_texture_handles) {
            if (texture != 0) {
                glDeleteTextures(1, &texture);
            }
        }
        resources->owned_texture_handles.clear();

        if (resources->fallback_texture_diffuse != 0) {
            glDeleteTextures(1, &resources->fallback_texture_diffuse);
            resources->fallback_texture_diffuse = 0;
        }
        if (resources->fallback_texture_normal != 0) {
            glDeleteTextures(1, &resources->fallback_texture_normal);
            resources->fallback_texture_normal = 0;
        }
        if (resources->fallback_texture_black != 0) {
            glDeleteTextures(1, &resources->fallback_texture_black);
            resources->fallback_texture_black = 0;
        }

        if (resources->shader_program != 0) {
            glDeleteProgram(resources->shader_program);
            resources->shader_program = 0;
        }
        if (resources->shadow_shader_program != 0) {
            glDeleteProgram(resources->shadow_shader_program);
            resources->shadow_shader_program = 0;
        }

        resources->uniform_model = -1;
        resources->uniform_view = -1;
        resources->uniform_projection = -1;
        resources->uniform_texture_diffuse = -1;
        resources->uniform_texture_normal = -1;
        resources->uniform_texture_alpha_mask = -1;
        resources->uniform_texture_metallic_roughness = -1;
        resources->uniform_texture_occlusion = -1;
        resources->uniform_texture_emissive = -1;
        resources->uniform_base_color_factor = -1;
        resources->uniform_metallic_factor = -1;
        resources->uniform_roughness_factor = -1;
        resources->uniform_emissive_factor = -1;
        resources->uniform_alpha_cutoff = -1;
        resources->uniform_normal_scale = -1;
        resources->uniform_occlusion_strength = -1;
        resources->uniform_shadow_model = -1;
        resources->uniform_shadow_light_view_projection = -1;
        resources->uniform_shadow_texture_alpha_mask = -1;
    }

    void render_scene_gpu_resources(
        const SceneGPUResources& resources,
        const SceneDrawParams& draw_params) {
        glUseProgram(resources.shader_program);
        glUniform1i(resources.uniform_texture_diffuse, 0);
        glUniform1i(resources.uniform_texture_normal, 1);
        glUniform1i(resources.uniform_texture_alpha_mask, 2);
        glUniform1i(resources.uniform_texture_metallic_roughness, 3);
        glUniform1i(resources.uniform_texture_occlusion, 4);
        glUniform1i(resources.uniform_texture_emissive, 5);
        glUniformMatrix4fv(
            resources.uniform_model,
            1, GL_FALSE, &draw_params.mat_model[0][0]);
        glUniformMatrix4fv(
            resources.uniform_view,
            1, GL_FALSE, &draw_params.mat_view[0][0]);
        glUniformMatrix4fv(
            resources.uniform_projection,
            1, GL_FALSE, &draw_params.mat_projection[0][0]);

        for (const auto& mesh : resources.meshes) {
            uint32_t texture_diffuse = resources.fallback_texture_diffuse;
            uint32_t texture_normal = resources.fallback_texture_normal;
            uint32_t texture_alpha_mask = resources.fallback_texture_diffuse;
            uint32_t texture_metallic_roughness = resources.fallback_texture_diffuse;
            uint32_t texture_occlusion = resources.fallback_texture_diffuse;
            uint32_t texture_emissive = resources.fallback_texture_black;
            glm::vec4 base_color_factor = glm::vec4(1.0f);
            float metallic_factor = 1.0f;
            float roughness_factor = 1.0f;
            glm::vec3 emissive_factor = glm::vec3(0.0f);
            float alpha_cutoff = 0.5f;
            float normal_scale = 1.0f;
            float occlusion_strength = 1.0f;
            if (mesh.material_index < resources.materials.size()) {
                const SceneGPUResources::Material& material = resources.materials[mesh.material_index];
                texture_diffuse = material.texture_diffuse;
                texture_normal = material.texture_normal;
                texture_alpha_mask = material.texture_alpha_mask;
                texture_metallic_roughness = material.texture_metallic_roughness;
                texture_occlusion = material.texture_occlusion;
                texture_emissive = material.texture_emissive;
                base_color_factor = material.base_color_factor;
                metallic_factor = material.metallic_factor;
                roughness_factor = material.roughness_factor;
                emissive_factor = material.emissive_factor;
                alpha_cutoff = material.alpha_cutoff;
                normal_scale = material.normal_scale;
                occlusion_strength = material.occlusion_strength;
            }

            glUniform4fv(resources.uniform_base_color_factor, 1, &base_color_factor[0]);
            glUniform1f(resources.uniform_metallic_factor, metallic_factor);
            glUniform1f(resources.uniform_roughness_factor, roughness_factor);
            glUniform3fv(resources.uniform_emissive_factor, 1, &emissive_factor[0]);
            glUniform1f(resources.uniform_alpha_cutoff, alpha_cutoff);
            glUniform1f(resources.uniform_normal_scale, normal_scale);
            glUniform1f(resources.uniform_occlusion_strength, occlusion_strength);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture_diffuse);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, texture_normal);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, texture_alpha_mask);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, texture_metallic_roughness);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, texture_occlusion);
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, texture_emissive);
            glBindVertexArray(mesh.VAO);
            glDrawElements(GL_TRIANGLES, mesh.index_count, GL_UNSIGNED_INT, nullptr);
        }

        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(0);
    }

    void render_scene_gpu_resources_shadow(
        const SceneGPUResources& resources,
        const glm::mat4& mat_model,
        const glm::mat4& mat_light_view_projection) {
        glUseProgram(resources.shadow_shader_program);
        glUniform1i(resources.uniform_shadow_texture_alpha_mask, 0);
        glUniformMatrix4fv(
            resources.uniform_shadow_model,
            1, GL_FALSE, &mat_model[0][0]);
        glUniformMatrix4fv(
            resources.uniform_shadow_light_view_projection,
            1, GL_FALSE, &mat_light_view_projection[0][0]);

        for (const auto& mesh : resources.meshes) {
            uint32_t texture_alpha_mask = resources.fallback_texture_diffuse;
            if (mesh.material_index < resources.materials.size()) {
                texture_alpha_mask = resources.materials[mesh.material_index].texture_alpha_mask;
            }

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture_alpha_mask);
            glBindVertexArray(mesh.VAO);
            glDrawElements(GL_TRIANGLES, mesh.index_count, GL_UNSIGNED_INT, nullptr);
        }

        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(0);
    }

}
