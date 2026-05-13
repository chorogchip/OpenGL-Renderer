#pragma once

struct GLFWwindow;

namespace chr {
    struct GBufferResources;
    struct SceneGPUResources;
    struct SceneRaw;

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
        GBufferResources* g_buffer_resources,
        SceneGPUResources* scene_gpu_resources);
}
