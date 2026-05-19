#pragma once

#include <cstddef>
#include <cstdint>

struct GLFWwindow;

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
        bool* show_debug_views,
        bool* show_light_markers,
        float* exposure,
        const RendererOverlayStats& stats);
    void end_frame();
}
