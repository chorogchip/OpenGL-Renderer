#pragma once

#include <glm/glm.hpp>

#include "app/imgui_layer.h"
#include "scene/scene_raw.h"

// Scene runtime calculations
// - Bounding sphere (for camera framing)
// - Statistics display (mesh count, vertex count, etc.)
namespace chr {

    struct SceneFrame {
        glm::vec3 center = glm::vec3(0.0f);
        float scale = 1.0f;
        float radius = 1.0f;
    };

    SceneFrame calculate_scene_frame(const SceneRaw& scene_raw);
    imgui_layer::RendererOverlayStats calculate_scene_overlay_stats(
        const SceneRaw& scene_raw,
        const char* scene_path);

}
