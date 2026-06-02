#include "scene/scene_runtime.h"

#include <limits>

namespace chr {

    SceneFrame calculate_scene_frame(const SceneRaw& scene_raw) {
        glm::vec3 min_bounds(std::numeric_limits<float>::max());
        glm::vec3 max_bounds(std::numeric_limits<float>::lowest());
        bool has_vertex = false;

        for (const auto& mesh : scene_raw.meshes) {
            for (const auto& vertex : mesh.vertices) {
                min_bounds = glm::min(min_bounds, vertex.position);
                max_bounds = glm::max(max_bounds, vertex.position);
                has_vertex = true;
            }
        }

        if (!has_vertex) {
            return {};
        }

        const glm::vec3 center = (min_bounds + max_bounds) * 0.5f;
        const glm::vec3 extent = max_bounds - min_bounds;
        const float max_extent = glm::max(glm::max(extent.x, extent.y), extent.z);
        const float scale = max_extent > 0.0001f ? 20.0f / max_extent : 1.0f;

        SceneFrame frame{};
        frame.center = center;
        frame.scale = scale;
        frame.radius = glm::length(extent) * 0.5f * scale;
        return frame;
    }

    imgui_layer::RendererOverlayStats calculate_scene_overlay_stats(
        const SceneRaw& scene_raw,
        const char* scene_path) {
        imgui_layer::RendererOverlayStats stats{};
        stats.scene_path = scene_path;
        stats.mesh_count = scene_raw.meshes.size();
        stats.material_count = scene_raw.materials.size();

        for (const auto& mesh : scene_raw.meshes) {
            stats.vertex_count += mesh.vertices.size();
            stats.index_count += mesh.indices.size();
            stats.triangle_count += mesh.indices.size() / 3;
        }

        for (const auto& material : scene_raw.materials) {
            stats.texture_slot_count += !material.texture_diffuse.empty() ? 1 : 0;
            stats.texture_slot_count += !material.texture_normal.empty() ? 1 : 0;
            stats.texture_slot_count += !material.texture_alpha_mask.empty() ? 1 : 0;
            stats.texture_slot_count += !material.texture_metallic.empty() ? 1 : 0;
            stats.texture_slot_count += !material.texture_roughness.empty() ? 1 : 0;
            stats.texture_slot_count += !material.texture_occlusion.empty() ? 1 : 0;
            stats.texture_slot_count += !material.texture_emissive.empty() ? 1 : 0;
        }

        return stats;
    }

}
