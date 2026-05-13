#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "app_input.h"
#include "camera.h"
#include "renderer.h"
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

    chr::Renderer renderer;
    chr::SceneGPUResources scene_gpu_resources;
    if (!chr::init_render_resources_with_loading_screen(
        window,
        scene_raw,
        &framebuffer_width,
        &framebuffer_height,
        &renderer,
        &scene_gpu_resources)) {
        imgui_layer::shutdown();
        glfwTerminate();
        return -1;
    }

    const chr::SceneFrame scene_frame = chr::calculate_scene_frame(scene_raw);
    imgui_layer::RendererOverlayStats overlay_stats =
        chr::calculate_scene_overlay_stats(scene_raw, scene_path);
    chr::frame_scene_camera(&camera, scene_frame);
    chr::run_render_loop(
        window,
        &camera,
        scene_frame,
        &renderer,
        &scene_gpu_resources,
        overlay_stats);

    chr::clear_scene_gpu_resources(&scene_gpu_resources);
    renderer.clear();
    imgui_layer::shutdown();
    glfwTerminate();
    return 0;
}
