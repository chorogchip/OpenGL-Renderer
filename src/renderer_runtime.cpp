#include "renderer_runtime.h"

#include <algorithm>
#include <iostream>
#include <string>

#include <GLFW/glfw3.h>

#include "g_buffer_resources.h"
#include "imgui_layer.h"
#include "loading_screen.h"
#include "scene_async_loader.h"
#include "scene_gpu_resources_runtime.h"

namespace {
    constexpr float SCENE_FILE_PROGRESS_WEIGHT = 0.55f;
    constexpr float FRAMEBUFFER_PROGRESS_WEIGHT = 0.07f;
    constexpr float GPU_RESOURCE_PROGRESS_WEIGHT = 0.38f;
    constexpr double EXPECTED_SCENE_FILE_LOAD_SECONDS = 8.0;

    chr::LoadingScreenState make_progress_loading_state(
        const std::string& title,
        const std::string& message,
        const float progress) {
        chr::LoadingScreenState state{};
        state.status = chr::SceneLoadStatus::Loading;
        state.title = title;
        state.message = message;
        state.progress = std::clamp(progress, 0.0f, 1.0f);
        return state;
    }

    void render_loading_frame(
        GLFWwindow* window,
        const chr::LoadingScreenState& state,
        int* framebuffer_width,
        int* framebuffer_height) {
        int current_framebuffer_width = 0;
        int current_framebuffer_height = 0;
        glfwGetFramebufferSize(window, &current_framebuffer_width, &current_framebuffer_height);
        if (current_framebuffer_width <= 0 || current_framebuffer_height <= 0) {
            glfwPollEvents();
            return;
        }

        *framebuffer_width = current_framebuffer_width;
        *framebuffer_height = current_framebuffer_height;

        imgui_layer::begin_frame();
        chr::draw_loading_screen(state, *framebuffer_width, *framebuffer_height);
        imgui_layer::end_frame();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

namespace chr {

    bool load_scene_with_loading_screen(
        GLFWwindow* window,
        const char* scene_path,
        int* framebuffer_width,
        int* framebuffer_height,
        SceneRaw* scene_raw) {
        SceneAsyncLoader scene_loader;
        if (!scene_loader.start(scene_path)) {
            std::cout << "Failed to start scene loading: " << scene_path << std::endl;
            return false;
        }

        bool load_failed = false;
        double load_failed_time = 0.0;
        const double scene_load_start_time = glfwGetTime();
        SceneLoadSnapshot load_snapshot = scene_loader.snapshot();
        while (!glfwWindowShouldClose(window)) {
            scene_loader.poll();
            load_snapshot = scene_loader.snapshot();

            LoadingScreenState loading_state = make_loading_screen_state(load_snapshot);
            if (load_snapshot.status == SceneLoadStatus::Loading) {
                const double elapsed = glfwGetTime() - scene_load_start_time;
                const float scene_file_progress = static_cast<float>(
                    std::min(elapsed / EXPECTED_SCENE_FILE_LOAD_SECONDS, 0.97));
                loading_state.progress = scene_file_progress * SCENE_FILE_PROGRESS_WEIGHT;
            }
            else if (load_snapshot.status == SceneLoadStatus::Ready) {
                loading_state.progress = SCENE_FILE_PROGRESS_WEIGHT;
            }
            render_loading_frame(window, loading_state, framebuffer_width, framebuffer_height);

            if (load_snapshot.status == SceneLoadStatus::Ready) {
                break;
            }

            if (load_snapshot.status == SceneLoadStatus::Failed) {
                if (!load_failed) {
                    load_failed = true;
                    load_failed_time = glfwGetTime();
                    std::cout << "Failed to load scene data from: " << scene_path
                        << " (" << load_snapshot.error_message << ")" << std::endl;
                }
                else if (glfwGetTime() - load_failed_time >= 2.0) {
                    return false;
                }
            }

            glfwWaitEventsTimeout(1.0 / 60.0);
        }

        if (load_snapshot.status != SceneLoadStatus::Ready || !scene_loader.has_result()) {
            return false;
        }

        *scene_raw = scene_loader.take_result();
        if (scene_raw->meshes.empty()) {
            std::cout << "Failed to load scene data from: " << scene_path << std::endl;
            return false;
        }

        return true;
    }

    bool init_render_resources_with_loading_screen(
        GLFWwindow* window,
        const SceneRaw& scene_raw,
        int* framebuffer_width,
        int* framebuffer_height,
        GBufferResources* g_buffer_resources,
        SceneGPUResources* scene_gpu_resources) {
        render_loading_frame(
            window,
            make_progress_loading_state("Preparing Renderer", "Creating frame buffers...", SCENE_FILE_PROGRESS_WEIGHT),
            framebuffer_width,
            framebuffer_height);

        if (g_buffer_resources->init(*framebuffer_width, *framebuffer_height) != 0) {
            g_buffer_resources->clear();
            return false;
        }
        render_loading_frame(
            window,
            make_progress_loading_state(
                "Preparing Renderer",
                "Frame buffers ready.",
                SCENE_FILE_PROGRESS_WEIGHT + FRAMEBUFFER_PROGRESS_WEIGHT),
            framebuffer_width,
            framebuffer_height);

        SceneGPUInitState scene_gpu_init_state;
        begin_scene_gpu_resources_init(scene_gpu_resources, scene_raw, &scene_gpu_init_state);
        while (!glfwWindowShouldClose(window) &&
            scene_gpu_init_state.phase != SceneGPUInitPhase::Complete) {
            if (!step_scene_gpu_resources_init(scene_gpu_resources, &scene_gpu_init_state)) {
                break;
            }

            const float total_progress = SCENE_FILE_PROGRESS_WEIGHT +
                FRAMEBUFFER_PROGRESS_WEIGHT +
                GPU_RESOURCE_PROGRESS_WEIGHT * scene_gpu_init_state.progress;
            render_loading_frame(
                window,
                make_progress_loading_state("Uploading Scene", scene_gpu_init_state.message, total_progress),
                framebuffer_width,
                framebuffer_height);
        }

        if (scene_gpu_init_state.phase != SceneGPUInitPhase::Complete) {
            clear_scene_gpu_resources(scene_gpu_resources);
            g_buffer_resources->clear();
            return false;
        }

        return true;
    }

}
