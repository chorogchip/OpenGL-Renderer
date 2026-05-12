#include "scene_gpu_resources_runtime.h"

#include <cstddef>
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

    uint32_t create_fallback_texture(const unsigned char rgba[4]) {
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

    void report_progress(
        const chr::SceneGPUProgressCallback& progress_callback,
        const float progress,
        const char* message) {
        if (progress_callback) {
            progress_callback(progress, message);
        }
    }

    float calculate_mesh_progress(const chr::SceneGPUInitState& state) {
        if (state.scene_raw == nullptr || state.scene_raw->meshes.empty()) {
            return GPU_PROGRESS_SHADER_WEIGHT + GPU_PROGRESS_MESH_WEIGHT;
        }

        return GPU_PROGRESS_SHADER_WEIGHT +
            GPU_PROGRESS_MESH_WEIGHT *
            (static_cast<float>(state.mesh_index) / static_cast<float>(state.scene_raw->meshes.size()));
    }

    float calculate_material_progress(const chr::SceneGPUInitState& state) {
        if (state.scene_raw == nullptr || state.scene_raw->materials.empty()) {
            return 1.0f;
        }

        constexpr float MATERIAL_TEXTURE_STEPS = 4.0f;
        const float completed_steps =
            static_cast<float>(state.material_index) * MATERIAL_TEXTURE_STEPS +
            static_cast<float>(state.material_texture_step);
        const float total_steps =
            static_cast<float>(state.scene_raw->materials.size()) * MATERIAL_TEXTURE_STEPS;
        return GPU_PROGRESS_SHADER_WEIGHT + GPU_PROGRESS_MESH_WEIGHT +
            GPU_PROGRESS_MATERIAL_WEIGHT * (completed_steps / total_steps);
    }

    bool init_scene_gpu_shaders(chr::SceneGPUResources* resources) {

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
        constexpr unsigned char flat_normal_pixel[] = { 128, 128, 255, 255 };
        resources->fallback_texture_diffuse = create_fallback_texture(white_pixel);
        resources->fallback_texture_normal = create_fallback_texture(flat_normal_pixel);
        return true;
    }

    void upload_scene_mesh(
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
                state->pending_texture_diffuse =
                    graphics_util::load_texture_2d(material_raw.texture_diffuse);
                state->material_texture_step = 1;
            }
            else if (state->material_texture_step == 1) {
                state->pending_texture_normal =
                    graphics_util::load_texture_2d(material_raw.texture_normal);
                state->material_texture_step = 2;
            }
            else if (state->material_texture_step == 2) {
                state->pending_texture_alpha_mask =
                    graphics_util::load_texture_2d(material_raw.texture_alpha_mask);
                state->material_texture_step = 3;
            }
            else {
                resources->materials.push_back({
                    state->pending_texture_diffuse != 0
                        ? state->pending_texture_diffuse
                        : resources->fallback_texture_diffuse,
                    state->pending_texture_normal != 0
                        ? state->pending_texture_normal
                        : resources->fallback_texture_normal,
                    state->pending_texture_alpha_mask != 0
                        ? state->pending_texture_alpha_mask
                        : resources->fallback_texture_diffuse
                });
                state->pending_texture_diffuse = 0;
                state->pending_texture_normal = 0;
                state->pending_texture_alpha_mask = 0;
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

        for (const auto& material : resources->materials) {
            if (material.texture_diffuse != 0 &&
                material.texture_diffuse != resources->fallback_texture_diffuse) {
                glDeleteTextures(1, &material.texture_diffuse);
            }
            if (material.texture_normal != 0 &&
                material.texture_normal != resources->fallback_texture_normal &&
                material.texture_normal != material.texture_diffuse &&
                material.texture_normal != material.texture_alpha_mask) {
                glDeleteTextures(1, &material.texture_normal);
            }
            if (material.texture_alpha_mask != 0 &&
                material.texture_alpha_mask != resources->fallback_texture_diffuse &&
                material.texture_alpha_mask != material.texture_diffuse) {
                glDeleteTextures(1, &material.texture_alpha_mask);
            }
        }
        resources->materials.clear();

        if (resources->fallback_texture_diffuse != 0) {
            glDeleteTextures(1, &resources->fallback_texture_diffuse);
            resources->fallback_texture_diffuse = 0;
        }
        if (resources->fallback_texture_normal != 0) {
            glDeleteTextures(1, &resources->fallback_texture_normal);
            resources->fallback_texture_normal = 0;
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
            if (mesh.material_index < resources.materials.size()) {
                texture_diffuse = resources.materials[mesh.material_index].texture_diffuse;
                texture_normal = resources.materials[mesh.material_index].texture_normal;
                texture_alpha_mask = resources.materials[mesh.material_index].texture_alpha_mask;
            }

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture_diffuse);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, texture_normal);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, texture_alpha_mask);
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
