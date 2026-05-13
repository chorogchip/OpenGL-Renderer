#include "imgui_layer.h"

#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace {
    constexpr const char* IMGUI_GLSL_VERSION = "#version 330";
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
        bool* show_debug_views,
        bool* show_light_markers,
        const RendererOverlayStats& stats) {
        ImGui::Begin("Renderer");
        ImGui::Text("Render Controls");
        ImGui::Separator();
        ImGui::Checkbox("Debug G-buffer Views", show_debug_views);
        ImGui::Checkbox("Point Light Markers", show_light_markers);
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
        ImGui::TextUnformatted("P/O: keyboard toggles");
        ImGui::End();
    }

    void end_frame() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

}
