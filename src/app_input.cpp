#include "app_input.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "camera.h"

namespace {
    constexpr float CAMERA_MOVE_SPEED = 0.25f;
    constexpr float MOUSE_SENSITIVITY = 0.08f;
    constexpr float SCROLL_SENSITIVITY = 2.0f;

    chr::Camera* g_camera = nullptr;
    bool g_is_mouse_captured = true;
    bool g_first_mouse_input = true;
    double g_last_mouse_x = 0.0;
    double g_last_mouse_y = 0.0;

    void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
        glViewport(0, 0, width, height);
    }

    void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
        if (!g_is_mouse_captured || g_camera == nullptr) {
            return;
        }

        if (g_first_mouse_input) {
            g_last_mouse_x = xpos;
            g_last_mouse_y = ypos;
            g_first_mouse_input = false;
            return;
        }

        const float delta_x = static_cast<float>(xpos - g_last_mouse_x);
        const float delta_y = static_cast<float>(g_last_mouse_y - ypos);
        g_last_mouse_x = xpos;
        g_last_mouse_y = ypos;

        g_camera->move_rotation(delta_x * MOUSE_SENSITIVITY, delta_y * MOUSE_SENSITIVITY);
    }

    void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
        if (g_camera == nullptr) {
            return;
        }

        g_camera->move_zoom(static_cast<float>(-yoffset) * SCROLL_SENSITIVITY);
    }

    void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !g_is_mouse_captured) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            g_is_mouse_captured = true;
            g_first_mouse_input = true;
        }
    }
}

namespace app_input {

    void initialize(GLFWwindow* window, chr::Camera* camera, int screen_width, int screen_height) {
        g_camera = camera;
        g_is_mouse_captured = true;
        g_first_mouse_input = true;
        g_last_mouse_x = static_cast<double>(screen_width) * 0.5;
        g_last_mouse_y = static_cast<double>(screen_height) * 0.5;

        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwSetCursorPosCallback(window, cursor_position_callback);
        glfwSetScrollCallback(window, scroll_callback);
        glfwSetMouseButtonCallback(window, mouse_button_callback);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    void process_input(GLFWwindow* window) {
        if (g_camera == nullptr) {
            return;
        }

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            if (g_is_mouse_captured) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                g_is_mouse_captured = false;
                g_first_mouse_input = true;
            }
        }

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            g_camera->position += CAMERA_MOVE_SPEED * g_camera->dir_front;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            g_camera->position -= CAMERA_MOVE_SPEED * g_camera->dir_front;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            g_camera->position -= glm::normalize(glm::cross(g_camera->dir_front, g_camera->dir_up)) * CAMERA_MOVE_SPEED;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            g_camera->position += glm::normalize(glm::cross(g_camera->dir_front, g_camera->dir_up)) * CAMERA_MOVE_SPEED;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            g_camera->position += CAMERA_MOVE_SPEED * g_camera->dir_up;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            g_camera->position -= CAMERA_MOVE_SPEED * g_camera->dir_up;
    }

}
