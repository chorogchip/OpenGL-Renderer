#pragma once

struct GLFWwindow;

namespace chr {
    class Camera;
}

// Input handling: keyboard, mouse, window events
// WASD: camera movement
// Mouse: camera rotation (click to capture, ESC to release)
// P/O/R: toggle debug views / light markers / reload shaders
namespace app_input {
    void initialize(GLFWwindow* window, chr::Camera* camera, int screen_width, int screen_height);
    void process_input(GLFWwindow* window, float delta_time);
    bool consume_toggle_debug_views_requested();
    bool consume_toggle_light_markers_requested();
    bool consume_reload_shaders_requested();
}
