#include <immintrin.h>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "app_input.h"
#include "camera.h"
#include "g_buffer_resources.h"
#include "imgui_layer.h"
#include "renderer_runtime.h"
#include "scene_gpu_resources.h"
#include "scene_gpu_resources_runtime.h"
#include "scene_runtime.h"

constexpr unsigned SCREEN_WIDTH = 800;
constexpr unsigned SCREEN_HEIGHT = 600;
#ifndef OPENGL_RENDERER_DEFAULT_SPONZA_SCENE
#define OPENGL_RENDERER_DEFAULT_SPONZA_SCENE "assets/main_sponza/NewSponza_Main_glTF_003.gltf"
#endif
constexpr const char* SPONZA_SCENE_RELATIVE_PATH = OPENGL_RENDERER_DEFAULT_SPONZA_SCENE;

chr::Camera camera{};
bool show_debug_views = false;
bool show_light_markers = true;

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

    chr::SceneRaw scene_raw;
    if (!chr::load_scene_with_loading_screen(
        window,
        scene_path,
        &framebuffer_width,
        &framebuffer_height,
        &scene_raw)) {
        imgui_layer::shutdown();
        glfwTerminate();
        return -1;
    }

    chr::GBufferResources g_buffer_resources;
    chr::SceneGPUResources scene_gpu_resources;
    if (!chr::init_render_resources_with_loading_screen(
        window,
        scene_raw,
        &framebuffer_width,
        &framebuffer_height,
        &g_buffer_resources,
        &scene_gpu_resources)) {
        imgui_layer::shutdown();
        glfwTerminate();
        return -1;
    }

    const chr::SceneFrame scene_frame = chr::calculate_scene_frame(scene_raw);
    const glm::vec3 scene_center = scene_frame.center * scene_frame.scale;
    const float camera_distance = glm::max(scene_frame.radius * 1.8f, 25.0f);
    camera.set_position(scene_center + glm::vec3(0.0f, camera_distance * 0.35f, camera_distance));
    camera.set_lookat(scene_center);

    float last_time = static_cast<float>(glfwGetTime());
    uint64_t frames = 0;
    double previous_cpu_frame_ms = 0.0;
    imgui_layer::RendererOverlayStats overlay_stats =
        chr::calculate_scene_overlay_stats(scene_raw, scene_path);
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

        const double frame_cpu_start_time = glfwGetTime();
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
        overlay_stats.fps = delta_time > 0.0f ? 1.0 / static_cast<double>(delta_time) : 0.0;
        overlay_stats.cpu_frame_ms = previous_cpu_frame_ms;
        overlay_stats.frame_index = frames;
        imgui_layer::draw_overlay(&show_debug_views, &show_light_markers, overlay_stats);
        imgui_layer::end_frame();

        glfwSwapBuffers(window);
        glfwPollEvents();

        previous_cpu_frame_ms = (glfwGetTime() - frame_cpu_start_time) * 1000.0;
        ++frames;
    }

    chr::clear_scene_gpu_resources(&scene_gpu_resources);
    g_buffer_resources.clear();
    imgui_layer::shutdown();
    glfwTerminate();
    return 0;
}
