#include "imgui_layer.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>
#include <array>

#include "debug_preview_pass.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace {
    constexpr const char* IMGUI_GLSL_VERSION = "#version 330";
    constexpr const char* DEBUG_VIEW_LABELS[] = {
        "Final",
        "Albedo",
        "Normal",
        "Depth",
        "SSAO",
        "Metallic",
        "Roughness",
        "AO",
        "Emissive",
        "Source HDR",
        "Environment Cubemap",
        "Irradiance",
        "Prefilter Mip 0",
        "Prefilter Mip 1",
        "Prefilter Mip 2",
        "Prefilter Mip 3",
        "Prefilter Mip 4",
        "BRDF LUT"
    };
}

namespace imgui_layer {

    int init(GLFWwindow* window) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui::StyleColorsDark();

        if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
            return -1;
        }
        if (!ImGui_ImplOpenGL3_Init(IMGUI_GLSL_VERSION)) {
            ImGui_ImplGlfw_Shutdown();
            return -1;
        }

        return 0;
    }

    void shutdown() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void begin_frame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void draw_overlay(
        chr::DebugViewMode* debug_view_mode,
        bool* show_light_markers,
        float* exposure,
        bool* enable_fxaa,
        float* directional_light_intensity,
        glm::vec3* directional_light_color,
        float* ambient_intensity,
        std::array<float, 5>* point_light_intensities,
        const RendererOverlayStats& stats) {
        ImGui::Begin("Renderer");
        ImGui::Text("Render Controls");
        ImGui::Separator();
        int current_debug_view = static_cast<int>(*debug_view_mode);
        if (ImGui::Combo(
            "Debug View",
            &current_debug_view,
            DEBUG_VIEW_LABELS,
            static_cast<int>(chr::DebugViewMode::Count))) {
            *debug_view_mode = static_cast<chr::DebugViewMode>(current_debug_view);
        }
        ImGui::Checkbox("Point Light Markers", show_light_markers);
        ImGui::Checkbox("FXAA", enable_fxaa);
        ImGui::SliderFloat("Exposure", exposure, 0.1f, 5.0f, "%.2f");
        ImGui::Separator();
        ImGui::Text("Directional Light");
        ImGui::SliderFloat("Dir Light Intensity", directional_light_intensity, 0.0f, 5.0f, "%.2f");
        ImGui::ColorEdit3("Dir Light Color", &directional_light_color->x);
        ImGui::SliderFloat("Ambient Intensity", ambient_intensity, 0.0f, 1.0f, "%.2f");
        ImGui::Separator();
        ImGui::Text("Point Lights");
        for (int i = 0; i < 5; ++i) {
            ImGui::SliderFloat(("Point Light " + std::to_string(i+1)).c_str(), &(*point_light_intensities)[i], 0.0f, 20.0f, "%.2f");
        }
        ImGui::Separator();
        ImGui::Text("Frame: %llu", static_cast<unsigned long long>(stats.frame_index));
        ImGui::Text("FPS: %.1f", stats.fps);
        ImGui::Text("CPU Frame: %.2f ms", stats.cpu_frame_ms);
        ImGui::Separator();
        ImGui::Text("Scene: %s", stats.scene_path != nullptr ? stats.scene_path : "(none)");
        ImGui::Text("Meshes: %zu", stats.mesh_count);
        ImGui::Text("Materials: %zu", stats.material_count);
        ImGui::Text("Vertices: %zu", stats.vertex_count);
        ImGui::Text("Indices: %zu", stats.index_count);
        ImGui::Text("Triangles: %zu", stats.triangle_count);
        ImGui::Text("Texture Slots: %zu", stats.texture_slot_count);
        ImGui::Separator();
        ImGui::TextUnformatted("W/A/S/D: move");
        ImGui::TextUnformatted("Left drag: rotate camera");
        ImGui::TextUnformatted("P: cycle debug view");
        ImGui::TextUnformatted("O: toggle light markers");
        ImGui::TextUnformatted("R: reload shaders");
        ImGui::End();
    }

    void end_frame() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

}
