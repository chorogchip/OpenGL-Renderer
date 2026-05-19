#pragma once

#include <functional>
#include <cstddef>
#include <future>
#include <string>
#include <unordered_map>

#include "graphics_util.h"
#include "scene_gpu_resources.h"

namespace chr {

    using SceneGPUProgressCallback = std::function<void(float progress, const char* message)>;

    enum class SceneGPUInitPhase {
        Shaders,
        Meshes,
        Materials,
        Complete,
        Failed
    };

    struct SceneGPUInitState {
        const SceneRaw* scene_raw = nullptr;
        SceneGPUInitPhase phase = SceneGPUInitPhase::Shaders;
        std::size_t mesh_index = 0;
        std::size_t material_index = 0;
        std::size_t material_texture_step = 0;
        std::future<graphics_util::TextureImage> pending_texture_decode;
        std::string pending_texture_path;
        std::unordered_map<std::string, uint32_t> texture_cache;
        uint32_t pending_texture_diffuse = 0;
        uint32_t pending_texture_normal = 0;
        uint32_t pending_texture_alpha_mask = 0;
        uint32_t pending_texture_metallic_roughness = 0;
        uint32_t pending_texture_occlusion = 0;
        uint32_t pending_texture_emissive = 0;
        float progress = 0.0f;
        const char* message = "Compiling scene shaders...";
    };

    void begin_scene_gpu_resources_init(
        SceneGPUResources* resources,
        const SceneRaw& scene_raw,
        SceneGPUInitState* state);
    bool step_scene_gpu_resources_init(SceneGPUResources* resources, SceneGPUInitState* state);

    int init_scene_gpu_resources(
        SceneGPUResources* resources,
        const SceneRaw& scene_raw,
        const SceneGPUProgressCallback& progress_callback = {});
    void clear_scene_gpu_resources(SceneGPUResources* resources);
    void render_scene_gpu_resources(const SceneGPUResources& resources, const SceneDrawParams& draw_params);
    void render_scene_gpu_resources_shadow(const SceneGPUResources& resources,
        const glm::mat4& mat_model, const glm::mat4& mat_light_view_projection);

}
