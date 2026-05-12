#include "loading_screen.h"

#include <algorithm>

#include <glad/glad.h>
#include <imgui.h>

namespace chr {

    LoadingScreenState make_loading_screen_state(const SceneLoadSnapshot& snapshot) {
        LoadingScreenState state{};
        state.status = snapshot.status;
        state.message = snapshot.path;

        if (snapshot.status == SceneLoadStatus::Failed) {
            state.title = "Scene Load Failed";
            state.message = snapshot.error_message;
            state.progress = 1.0f;
        }
        else if (snapshot.status == SceneLoadStatus::Ready) {
            state.title = "Scene Ready";
            state.progress = 1.0f;
        }

        return state;
    }

    void draw_loading_screen(const LoadingScreenState& state, int framebuffer_width, int framebuffer_height) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, framebuffer_width, framebuffer_height);
        glClearColor(0.02f, 0.025f, 0.03f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(420.0f, 132.0f), ImGuiCond_Always);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings;

        ImGui::Begin("LoadingScreen", nullptr, flags);
        ImGui::TextUnformatted(state.title.c_str());
        ImGui::Separator();
        if (!state.message.empty()) {
            ImGui::TextWrapped("%s", state.message.c_str());
        }
        else {
            ImGui::TextUnformatted("Preparing scene data...");
        }

        const float progress = std::clamp(state.progress, 0.0f, 1.0f);
        if (state.progress >= 0.0f) {
            ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f));
        }
        else {
            ImGui::ProgressBar(-1.0f * static_cast<float>(ImGui::GetTime()), ImVec2(-1.0f, 0.0f));
        }
        ImGui::End();
    }

}
