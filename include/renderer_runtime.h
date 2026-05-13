#pragma once

struct GLFWwindow;

namespace chr {
    class Camera;
    struct Renderer;
    struct SceneGPUResources;
    struct SceneRaw;
    struct SceneFrame;
}

namespace imgui_layer {
    struct RendererOverlayStats;
}

namespace chr {

    bool load_scene_with_loading_screen(
        GLFWwindow* window,
        const char* scene_path,
        int* framebuffer_width,
        int* framebuffer_height,
        SceneRaw* scene_raw);

    bool init_render_resources_with_loading_screen(
        GLFWwindow* window,
        const SceneRaw& scene_raw,
        int* framebuffer_width,
        int* framebuffer_height,
        Renderer* renderer,
        SceneGPUResources* scene_gpu_resources);
    void frame_scene_camera(Camera* camera, const SceneFrame& scene_frame);
    void run_render_loop(
        GLFWwindow* window,
        Camera* camera,
        const SceneFrame& scene_frame,
        Renderer* renderer,
        SceneGPUResources* scene_gpu_resources,
        imgui_layer::RendererOverlayStats overlay_stats);
}
