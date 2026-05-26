#include "renderer_runtime.h"

#include <algorithm>
#include <iostream>
#include <string>

#include <immintrin.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "app_input.h"
#include "camera.h"
#include "renderer.h"
#include "imgui_layer.h"
#include "loading_screen.h"
#include "scene_async_loader.h"
#include "scene_gpu_resources_runtime.h"
#include "scene_runtime.h"

namespace {
    constexpr float SCENE_FILE_PROGRESS_WEIGHT = 0.55f;
    constexpr float FRAMEBUFFER_PROGRESS_WEIGHT = 0.07f;
    constexpr float GPU_RESOURCE_PROGRESS_WEIGHT = 0.38f;
    constexpr double EXPECTED_SCENE_FILE_LOAD_SECONDS = 8.0;

    static chr::LoadingScreenState make_progress_loading_state(
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

    static void render_loading_frame(
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
        Renderer* renderer,
        SceneGPUResources* scene_gpu_resources) {
        render_loading_frame(
            window,
            make_progress_loading_state("Preparing Renderer", "Creating frame buffers...", SCENE_FILE_PROGRESS_WEIGHT),
            framebuffer_width,
            framebuffer_height);

        if (renderer->init(*framebuffer_width, *framebuffer_height) != 0) {
            renderer->clear();
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
            renderer->clear();
            return false;
        }

        return true;
    }

    void frame_scene_camera(Camera* camera, const SceneFrame& scene_frame) {
        const glm::vec3 scene_center = scene_frame.center * scene_frame.scale;
        const float camera_distance = glm::max(scene_frame.radius * 1.8f, 25.0f);
        camera->set_position(scene_center + glm::vec3(0.0f, camera_distance * 0.35f, camera_distance));
        camera->set_lookat(scene_center);
    }

    void run_render_loop(
        GLFWwindow* window,
        Camera* camera,
        const SceneFrame& scene_frame,
        Renderer* renderer,
        SceneGPUResources* scene_gpu_resources,
        imgui_layer::RendererOverlayStats overlay_stats) {
        DebugViewMode debug_view_mode = DebugViewMode::Final;
        bool show_light_markers = true;
        int framebuffer_width = renderer->width;
        int framebuffer_height = renderer->height;
        float last_time = static_cast<float>(glfwGetTime());
        uint64_t frames = 0;
        double previous_cpu_frame_ms = 0.0;

        while (!glfwWindowShouldClose(window)) {
            float cur_time = static_cast<float>(glfwGetTime());
            float delta_time = cur_time - last_time;
            if (delta_time < 1.0f / 60.0f) {
                _mm_pause();
                continue;
            }

            last_time = cur_time;
            app_input::process_input(window);
            if (app_input::consume_toggle_debug_views_requested()) {
                const int next_debug_view =
                    (static_cast<int>(debug_view_mode) + 1) % static_cast<int>(DebugViewMode::Count);
                debug_view_mode = static_cast<DebugViewMode>(next_debug_view);
            }
            if (app_input::consume_toggle_light_markers_requested()) {
                show_light_markers = !show_light_markers;
            }

            int current_framebuffer_width = 0;
            int current_framebuffer_height = 0;
            glfwGetFramebufferSize(window, &current_framebuffer_width, &current_framebuffer_height);
            if (current_framebuffer_width <= 0 || current_framebuffer_height <= 0) {
                glfwPollEvents();
                continue;
            }

            if (current_framebuffer_width != framebuffer_width || current_framebuffer_height != framebuffer_height) {
                if (renderer->resize(current_framebuffer_width, current_framebuffer_height) != 0) {
                    break;
                }
                framebuffer_width = current_framebuffer_width;
                framebuffer_height = current_framebuffer_height;
            }

            const double frame_cpu_start_time = glfwGetTime();
            imgui_layer::begin_frame();

            SceneDrawParams draw_params{};
            draw_params.mat_projection = camera->get_projection_matrix(
                static_cast<float>(framebuffer_width) / framebuffer_height);
            draw_params.mat_view = camera->get_view_matrix();
            draw_params.mat_model = glm::scale(glm::mat4(1.0f), glm::vec3(scene_frame.scale));

            renderer->bind_for_shadow_pass();
            glClear(GL_DEPTH_BUFFER_BIT);
            render_scene_gpu_resources_shadow(
                *scene_gpu_resources,
                draw_params.mat_model,
                renderer->get_directional_light_view_projection());

            renderer->bind_for_geometry_pass();
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            render_scene_gpu_resources(*scene_gpu_resources, draw_params);
            renderer->draw_lit_frame(draw_params.mat_projection, draw_params.mat_view);
            if (show_light_markers) {
                renderer->draw_light_markers(draw_params.mat_projection, draw_params.mat_view);
            }
            if (debug_view_mode != DebugViewMode::Final) {
                renderer->draw_debug_view(debug_view_mode);
            }

            overlay_stats.fps = delta_time > 0.0f ? 1.0 / static_cast<double>(delta_time) : 0.0;
            overlay_stats.cpu_frame_ms = previous_cpu_frame_ms;
            overlay_stats.frame_index = frames;
            imgui_layer::draw_overlay(
                &debug_view_mode,
                &show_light_markers,
                &renderer->tone_mapping_pass.exposure,
                overlay_stats);
            imgui_layer::end_frame();

            glfwSwapBuffers(window);
            glfwPollEvents();

            previous_cpu_frame_ms = (glfwGetTime() - frame_cpu_start_time) * 1000.0;
            ++frames;
        }
    }

}
