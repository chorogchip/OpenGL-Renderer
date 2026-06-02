#include "app_input.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <imgui.h>

#include "camera.h"

namespace {
    constexpr float CAMERA_MOVE_SPEED = 15.0f;
    constexpr float MOUSE_SENSITIVITY = 0.08f;

    chr::Camera* g_camera = nullptr;
    bool g_toggle_debug_views_requested = false;
    bool g_toggle_light_markers_requested = false;
    bool g_reload_shaders_requested = false;
    bool g_mouse_captured = false;
    bool g_prev_escape_pressed = false;
    bool g_prev_p_pressed = false;
    bool g_prev_o_pressed = false;
    bool g_prev_r_pressed = false;
    bool g_has_mouse_origin = false;
    double g_last_mouse_x = 0.0;
    double g_last_mouse_y = 0.0;

    void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
        glViewport(0, 0, width, height);
    }

    void set_mouse_captured(GLFWwindow* window, const bool captured) {
        g_mouse_captured = captured;
        g_has_mouse_origin = false;
        glfwSetInputMode(window, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }

    void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
        (void)window;

        if (g_camera == nullptr || !g_mouse_captured) {
            return;
        }

        if (!g_has_mouse_origin) {
            g_last_mouse_x = xpos;
            g_last_mouse_y = ypos;
            g_has_mouse_origin = true;
            return;
        }

        const float delta_x = static_cast<float>(xpos - g_last_mouse_x);
        const float delta_y = static_cast<float>(g_last_mouse_y - ypos);
        g_last_mouse_x = xpos;
        g_last_mouse_y = ypos;

        g_camera->move_rotation(delta_x * MOUSE_SENSITIVITY, delta_y * MOUSE_SENSITIVITY);
    }

    void window_focus_callback(GLFWwindow* window, int focused) {
        if (!focused) {
            set_mouse_captured(window, false);
        }
    }

    void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
        (void)mods;

        if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS || g_mouse_captured) {
            return;
        }

        if (ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse) {
            return;
        }

        set_mouse_captured(window, true);
    }
}

namespace app_input {

    void initialize(GLFWwindow* window, chr::Camera* camera, int screen_width, int screen_height) {
        (void)screen_width;
        (void)screen_height;

        g_camera = camera;
        g_toggle_debug_views_requested = false;
        g_toggle_light_markers_requested = false;
        g_reload_shaders_requested = false;
        g_mouse_captured = false;
        g_prev_escape_pressed = false;
        g_prev_p_pressed = false;
        g_prev_o_pressed = false;
        g_prev_r_pressed = false;
        g_has_mouse_origin = false;

        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwSetCursorPosCallback(window, cursor_position_callback);
        glfwSetWindowFocusCallback(window, window_focus_callback);
        glfwSetMouseButtonCallback(window, mouse_button_callback);
        set_mouse_captured(window, true);
        if (glfwRawMouseMotionSupported()) {
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        }
    }

    void process_input(GLFWwindow* window, const float delta_time) {
        if (g_camera == nullptr) {
            return;
        }

        const bool wants_keyboard = ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureKeyboard;

        const bool is_escape_pressed = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
        if (is_escape_pressed && !g_prev_escape_pressed) {
            if (g_mouse_captured) {
                set_mouse_captured(window, false);
            } else {
                glfwSetWindowShouldClose(window, true);
            }
        }
        g_prev_escape_pressed = is_escape_pressed;

        const bool is_p_pressed = !wants_keyboard && glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
        if (is_p_pressed && !g_prev_p_pressed) {
            g_toggle_debug_views_requested = true;
        }
        g_prev_p_pressed = is_p_pressed;

        const bool is_o_pressed = !wants_keyboard && glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS;
        if (is_o_pressed && !g_prev_o_pressed) {
            g_toggle_light_markers_requested = true;
        }
        g_prev_o_pressed = is_o_pressed;

        const bool is_r_pressed = !wants_keyboard && glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
        if (is_r_pressed && !g_prev_r_pressed) {
            g_reload_shaders_requested = true;
        }
        g_prev_r_pressed = is_r_pressed;

        if (wants_keyboard) {
            return;
        }

        const float move_distance = CAMERA_MOVE_SPEED * delta_time;
        const glm::vec3 world_up = glm::vec3(0.0f, 1.0f, 0.0f);

        glm::vec3 front_xz = glm::vec3(g_camera->dir_front.x, 0.0f, g_camera->dir_front.z);
        if (glm::length(front_xz) > 0.0001f) {
            front_xz = glm::normalize(front_xz);
        }
        const glm::vec3 right_xz = glm::normalize(glm::cross(front_xz, world_up));

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            g_camera->position += move_distance * front_xz;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            g_camera->position -= move_distance * front_xz;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            g_camera->position -= right_xz * move_distance;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            g_camera->position += right_xz * move_distance;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            g_camera->position += move_distance * world_up;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            g_camera->position -= move_distance * world_up;
    }

    bool consume_toggle_debug_views_requested() {
        const bool requested = g_toggle_debug_views_requested;
        g_toggle_debug_views_requested = false;
        return requested;
    }

    bool consume_toggle_light_markers_requested() {
        const bool requested = g_toggle_light_markers_requested;
        g_toggle_light_markers_requested = false;
        return requested;
    }

    bool consume_reload_shaders_requested() {
        const bool requested = g_reload_shaders_requested;
        g_reload_shaders_requested = false;
        return requested;
    }

}
