#include <iostream>
#include <immintrin.h>
#include <algorithm>
#include <limits>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "app_input.h"
#include "camera.h"
#include "g_buffer_resources.h"
#include "imgui_layer.h"
#include "loading_screen.h"
#include "scene_async_loader.h"
#include "scene_gpu_resources.h"
#include "scene_gpu_resources_runtime.h"

constexpr unsigned SCREEN_WIDTH = 800;
constexpr unsigned SCREEN_HEIGHT = 600;
#ifndef OPENGL_RENDERER_DEFAULT_SPONZA_SCENE
#define OPENGL_RENDERER_DEFAULT_SPONZA_SCENE "assets/main_sponza/NewSponza_Main_glTF_003.gltf"
#endif
constexpr const char* SPONZA_SCENE_RELATIVE_PATH = OPENGL_RENDERER_DEFAULT_SPONZA_SCENE;

chr::Camera camera{};
bool show_debug_views = false;
bool show_light_markers = true;

namespace {
    constexpr float SCENE_FILE_PROGRESS_WEIGHT = 0.55f;
    constexpr float FRAMEBUFFER_PROGRESS_WEIGHT = 0.07f;
    constexpr float GPU_RESOURCE_PROGRESS_WEIGHT = 0.38f;
    constexpr double EXPECTED_SCENE_FILE_LOAD_SECONDS = 8.0;

    struct SceneFrame {
        glm::vec3 center = glm::vec3(0.0f);
        float scale = 1.0f;
        float radius = 1.0f;
    };

    SceneFrame calculate_scene_frame(const chr::SceneRaw& scene_raw) {
        glm::vec3 min_bounds(std::numeric_limits<float>::max());
        glm::vec3 max_bounds(std::numeric_limits<float>::lowest());
        bool has_vertex = false;

        for (const auto& mesh : scene_raw.meshes) {
            for (const auto& vertex : mesh.vertices) {
                min_bounds = glm::min(min_bounds, vertex.position);
                max_bounds = glm::max(max_bounds, vertex.position);
                has_vertex = true;
            }
        }

        if (!has_vertex) {
            return {};
        }

        const glm::vec3 center = (min_bounds + max_bounds) * 0.5f;
        const glm::vec3 extent = max_bounds - min_bounds;
        const float max_extent = glm::max(glm::max(extent.x, extent.y), extent.z);
        const float scale = max_extent > 0.0001f ? 20.0f / max_extent : 1.0f;

        SceneFrame frame{};
        frame.center = center;
        frame.scale = scale;
        frame.radius = glm::length(extent) * 0.5f * scale;
        return frame;
    }

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

int main(int argc, char** argv) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "OpenGL-Renderer", NULL, NULL);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    app_input::initialize(window, &camera, SCREEN_WIDTH, SCREEN_HEIGHT);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    if (imgui_layer::init(window) != 0) {
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    int framebuffer_width = SCREEN_WIDTH;
    int framebuffer_height = SCREEN_HEIGHT;
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);

    const char* scene_path = argc > 1 ? argv[1] : SPONZA_SCENE_RELATIVE_PATH;

    chr::SceneAsyncLoader scene_loader;
    if (!scene_loader.start(scene_path)) {
        imgui_layer::shutdown();
        std::cout << "Failed to start scene loading: " << scene_path << std::endl;
        glfwTerminate();
        return -1;
    }

    bool load_failed = false;
    double load_failed_time = 0.0;
    const double scene_load_start_time = glfwGetTime();
    chr::SceneLoadSnapshot load_snapshot = scene_loader.snapshot();
    while (!glfwWindowShouldClose(window)) {
        scene_loader.poll();
        load_snapshot = scene_loader.snapshot();

        chr::LoadingScreenState loading_state = chr::make_loading_screen_state(load_snapshot);
        if (load_snapshot.status == chr::SceneLoadStatus::Loading) {
            const double elapsed = glfwGetTime() - scene_load_start_time;
            const float scene_file_progress = static_cast<float>(
                std::min(elapsed / EXPECTED_SCENE_FILE_LOAD_SECONDS, 0.97));
            loading_state.progress = scene_file_progress * SCENE_FILE_PROGRESS_WEIGHT;
        }
        else if (load_snapshot.status == chr::SceneLoadStatus::Ready) {
            loading_state.progress = SCENE_FILE_PROGRESS_WEIGHT;
        }
        render_loading_frame(window, loading_state, &framebuffer_width, &framebuffer_height);

        if (load_snapshot.status == chr::SceneLoadStatus::Ready) {
            break;
        }

        if (load_snapshot.status == chr::SceneLoadStatus::Failed) {
            if (!load_failed) {
                load_failed = true;
                load_failed_time = glfwGetTime();
                std::cout << "Failed to load scene data from: " << scene_path
                    << " (" << load_snapshot.error_message << ")" << std::endl;
            }
            else if (glfwGetTime() - load_failed_time >= 2.0) {
                imgui_layer::shutdown();
                glfwTerminate();
                return -1;
            }
        }

        glfwWaitEventsTimeout(1.0 / 60.0);
    }

    if (load_snapshot.status != chr::SceneLoadStatus::Ready || !scene_loader.has_result()) {
        imgui_layer::shutdown();
        glfwTerminate();
        return -1;
    }

    chr::SceneRaw scene_raw = scene_loader.take_result();
    if (scene_raw.meshes.empty()) {
        imgui_layer::shutdown();
        std::cout << "Failed to load scene data from: " << scene_path << std::endl;
        glfwTerminate();
        return -1;
    }

    render_loading_frame(
        window,
        make_progress_loading_state("Preparing Renderer", "Creating frame buffers...", SCENE_FILE_PROGRESS_WEIGHT),
        &framebuffer_width,
        &framebuffer_height);

    chr::GBufferResources g_buffer_resources;
    if (g_buffer_resources.init(framebuffer_width, framebuffer_height) != 0) {
        g_buffer_resources.clear();
        imgui_layer::shutdown();
        glfwTerminate();
        return -1;
    }
    render_loading_frame(
        window,
        make_progress_loading_state(
            "Preparing Renderer",
            "Frame buffers ready.",
            SCENE_FILE_PROGRESS_WEIGHT + FRAMEBUFFER_PROGRESS_WEIGHT),
        &framebuffer_width,
        &framebuffer_height);

    chr::SceneGPUResources scene_gpu_resources;
    const auto gpu_progress_callback =
        [&](const float gpu_progress, const char* message) {
            const float total_progress = SCENE_FILE_PROGRESS_WEIGHT +
                FRAMEBUFFER_PROGRESS_WEIGHT +
                GPU_RESOURCE_PROGRESS_WEIGHT * gpu_progress;
            render_loading_frame(
                window,
                make_progress_loading_state("Uploading Scene", message, total_progress),
                &framebuffer_width,
                &framebuffer_height);
        };
    if (chr::init_scene_gpu_resources(&scene_gpu_resources, scene_raw, gpu_progress_callback) != 0) {
        chr::clear_scene_gpu_resources(&scene_gpu_resources);
        g_buffer_resources.clear();
        imgui_layer::shutdown();
        glfwTerminate();
        return -1;
    }

    const SceneFrame scene_frame = calculate_scene_frame(scene_raw);
    const glm::vec3 scene_center = scene_frame.center * scene_frame.scale;
    const float camera_distance = glm::max(scene_frame.radius * 1.8f, 25.0f);
    camera.set_position(scene_center + glm::vec3(0.0f, camera_distance * 0.35f, camera_distance));
    camera.set_lookat(scene_center);

    float last_time = static_cast<float>(glfwGetTime());
    uint64_t frames = 0;
    while (!glfwWindowShouldClose(window)) {
        
        float cur_time = static_cast<float>(glfwGetTime());
        float delta_time = cur_time - last_time;
        if (delta_time < 1.0f / 60.0f) {
            _mm_pause();
            continue;
        } else if (delta_time > 2.0f / 60.0f) {
            last_time = cur_time;
        } else {
            last_time = cur_time;
        }

        app_input::process_input(window);
        if (app_input::consume_toggle_debug_views_requested()) {
            show_debug_views = !show_debug_views;
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
            if (g_buffer_resources.resize(current_framebuffer_width, current_framebuffer_height) != 0) {
                break;
            }
            framebuffer_width = current_framebuffer_width;
            framebuffer_height = current_framebuffer_height;
        }

        imgui_layer::begin_frame();

        chr::SceneDrawParams draw_params{};
        draw_params.mat_projection = camera.get_projection_matrix(
            static_cast<float>(framebuffer_width) / framebuffer_height);
        draw_params.mat_view = camera.get_view_matrix();
        draw_params.mat_model = glm::scale(glm::mat4(1.0f), glm::vec3(scene_frame.scale));

        g_buffer_resources.bind_for_shadow_pass();
        glClear(GL_DEPTH_BUFFER_BIT);
        chr::render_scene_gpu_resources_shadow(
            scene_gpu_resources,
            draw_params.mat_model,
            g_buffer_resources.get_directional_light_view_projection());

        g_buffer_resources.bind_for_geometry_pass();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        chr::render_scene_gpu_resources(scene_gpu_resources, draw_params);
        g_buffer_resources.draw_lighting_pass(draw_params.mat_projection, draw_params.mat_view);
        if (show_light_markers) {
            g_buffer_resources.draw_light_markers(draw_params.mat_projection, draw_params.mat_view);
        }
        if (show_debug_views) {
            g_buffer_resources.draw_debug_views();
        }
        imgui_layer::draw_overlay(&show_debug_views, &show_light_markers);
        imgui_layer::end_frame();

        glfwSwapBuffers(window);
        glfwPollEvents();

        ++frames;
    }

    chr::clear_scene_gpu_resources(&scene_gpu_resources);
    g_buffer_resources.clear();
    imgui_layer::shutdown();
    glfwTerminate();
    return 0;
}
