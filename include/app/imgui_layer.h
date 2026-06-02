#pragma once

#include <cstddef>
#include <cstdint>
#include <array>

#include <glm/glm.hpp>

#include "features/deferred_lighting_pass.h"

struct GLFWwindow;

namespace chr {
    enum class DebugViewMode : int;
}

// ImGui debug overlay UI
// Shows: render controls, feature toggles, lighting controls, performance stats
namespace imgui_layer {
    struct RendererOverlayStats {
        const char* scene_path = nullptr;
        double fps = 0.0;
        double cpu_frame_ms = 0.0;
        uint64_t frame_index = 0;
        std::size_t mesh_count = 0;
        std::size_t material_count = 0;
        std::size_t vertex_count = 0;
        std::size_t index_count = 0;
        std::size_t triangle_count = 0;
        std::size_t texture_slot_count = 0;
    };

    int init(GLFWwindow* window);
    void shutdown();
    void begin_frame();
    void draw_overlay(
        chr::DebugViewMode* debug_view_mode,
        bool* show_light_markers,
        float* exposure,
        chr::RenderFeatures* render_features,
        float* directional_light_intensity,
        glm::vec3* directional_light_color,
        float* ambient_intensity,
        std::array<float, 5>* point_light_intensities,
        const RendererOverlayStats& stats);
    void end_frame();
}
