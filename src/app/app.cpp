#include "app/app.h"

#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "app/app_log.h"
#include "app/app_input.h"
#include "core/camera.h"
#include "app/renderer.h"
#include "app/imgui_layer.h"
#include "app/renderer_runtime.h"
#include "scene/scene_gpu_resources.h"
#include "scene/scene_gpu_resources_runtime.h"
#include "scene/scene_runtime.h"

constexpr unsigned SCREEN_WIDTH = 800;
constexpr unsigned SCREEN_HEIGHT = 600;

#ifndef OPENGL_RENDERER_DEFAULT_SPONZA_SCENE
#define OPENGL_RENDERER_DEFAULT_SPONZA_SCENE "assets/main_sponza/NewSponza_Main_glTF_003.gltf"
#endif

namespace app {

namespace {
    chr::Camera g_camera;

    bool init_glfw(GLFWwindow** out_window) {
        if (!glfwInit()) {
            return false;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

        *out_window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "OpenGL-Renderer", NULL, NULL);
        if (*out_window == nullptr) {
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(*out_window);
        return true;
    }

    bool init_graphics() {
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            return false;
        }

        glEnable(GL_DEPTH_TEST);
        return true;
    }

    void cleanup(GLFWwindow* window) {
        imgui_layer::shutdown();
        if (window) glfwTerminate();
        app_log::info("Application terminated successfully");
        app_log::close();
    }
}

int run(int argc, char** argv) {
    app_log::init("run.log");
    app_log::info("Application started");

    GLFWwindow* window = nullptr;
    if (!init_glfw(&window)) {
        app_log::error("Failed to initialize GLFW");
        app_log::close();
        return -1;
    }

    app_input::initialize(window, &g_camera, SCREEN_WIDTH, SCREEN_HEIGHT);

    if (!init_graphics()) {
        app_log::error("Failed to initialize graphics");
        cleanup(window);
        return -1;
    }

    if (imgui_layer::init(window) != 0) {
        app_log::error("Failed to initialize ImGui");
        cleanup(window);
        return -1;
    }

    int framebuffer_width = SCREEN_WIDTH;
    int framebuffer_height = SCREEN_HEIGHT;
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);

    const char* scene_path = argc > 1 ? argv[1] : OPENGL_RENDERER_DEFAULT_SPONZA_SCENE;

    chr::SceneRaw scene_raw;
    if (!chr::load_scene_with_loading_screen(window, scene_path, &framebuffer_width, &framebuffer_height, &scene_raw)) {
        app_log::error("Failed to load scene");
        cleanup(window);
        return -1;
    }

    chr::Renderer renderer;
    chr::SceneGPUResources scene_gpu_resources;
    if (!chr::init_render_resources_with_loading_screen(
        window, scene_raw, &framebuffer_width, &framebuffer_height, &renderer, &scene_gpu_resources)) {
        app_log::error("Failed to initialize render resources");
        cleanup(window);
        return -1;
    }

    const chr::SceneFrame scene_frame = chr::calculate_scene_frame(scene_raw);
    imgui_layer::RendererOverlayStats overlay_stats = chr::calculate_scene_overlay_stats(scene_raw, scene_path);
    chr::frame_scene_camera(&g_camera, scene_frame);

    chr::run_render_loop(window, &g_camera, scene_frame, &renderer, &scene_gpu_resources, overlay_stats);

    chr::clear_scene_gpu_resources(&scene_gpu_resources);
    renderer.clear();
    cleanup(window);
    return 0;
}

}
