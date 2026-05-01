#include "imgui_layer.h"

#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace {
    constexpr const char* IMGUI_GLSL_VERSION = "#version 330";
    bool g_show_demo_window = true;
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

    void draw_overlay() {
        ImGui::Begin("Renderer");
        ImGui::Text("Dear ImGui integrated.");
        ImGui::Checkbox("Show Demo Window", &g_show_demo_window);
        ImGui::End();

        if (g_show_demo_window) {
            ImGui::ShowDemoWindow(&g_show_demo_window);
        }
    }

    void end_frame() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

}
