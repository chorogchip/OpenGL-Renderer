#pragma once

#include <glm/glm.hpp>

#include "imgui_layer.h"
#include "scene_raw.h"

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
