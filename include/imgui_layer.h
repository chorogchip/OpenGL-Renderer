#pragma once

struct GLFWwindow;

namespace imgui_layer {
    int init(GLFWwindow* window);
    void shutdown();
    void begin_frame();
    void draw_overlay();
    void end_frame();
}
