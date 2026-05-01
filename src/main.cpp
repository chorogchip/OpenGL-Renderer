#include <iostream>
#include <immintrin.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "app_input.h"
#include "camera.h"
#include "model_loader.h"
#include "scene_gpu_resources.h"

constexpr unsigned SCREEN_WIDTH = 800;
constexpr unsigned SCREEN_HEIGHT = 600;
constexpr const char* SPONZA_OBJ_RELATIVE_PATH = "assets/Sponza-master/sponza.obj";

chr::Camera camera{};

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Voxel Cone Tracing", NULL, NULL);
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

    glEnable(GL_DEPTH_TEST);

    chr::SceneRaw scene_raw = chr::load_obj(SPONZA_OBJ_RELATIVE_PATH);
    if (scene_raw.meshes.empty()) {
        std::cout << "Failed to load scene data from: " << SPONZA_OBJ_RELATIVE_PATH << std::endl;
        glfwTerminate();
        return -1;
    }

    chr::SceneGPUResources scene_gpu_resources;
    if (scene_gpu_resources.init(scene_raw) != 0) {
        scene_gpu_resources.clear();
        glfwTerminate();
        return -1;
    }

    camera.set_position(glm::vec3(0.0f, 20.0f, 50.0f));
    camera.set_lookat(glm::vec3(0.0f, 5.0f, 0.0f));

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

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        chr::SceneDrawParams draw_params{};
        draw_params.mat_projection = camera.get_projection_matrix(
            static_cast<float>(SCREEN_WIDTH) / SCREEN_HEIGHT);
        draw_params.mat_view = camera.get_view_matrix();
        draw_params.mat_model = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f));
        scene_gpu_resources.draw(draw_params);

        glfwSwapBuffers(window);
        glfwPollEvents();

        ++frames;
    }

    scene_gpu_resources.clear();
    glfwTerminate();
    return 0;
}
