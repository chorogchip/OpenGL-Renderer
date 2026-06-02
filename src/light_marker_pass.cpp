#include "light_marker_pass.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "graphics_util.h"

namespace {
    constexpr const char* LIGHT_MARKER_VERTEX_SHADER_PATH = "assets/shaders/light_marker.vert";
    constexpr const char* LIGHT_MARKER_FRAGMENT_SHADER_PATH = "assets/shaders/light_marker.frag";
    constexpr float LIGHT_MARKER_SCALE = 0.18f;

    static void create_marker_mesh(chr::LightMarkerPass* pass) {
        constexpr float tetra_vertices[] = {
             0.0f,  1.0f,  0.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,

             0.0f,  1.0f,  0.0f,
             1.0f, -1.0f,  1.0f,
             0.0f, -1.0f, -1.0f,

             0.0f,  1.0f,  0.0f,
             0.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f, -1.0f,  1.0f,
             0.0f, -1.0f, -1.0f,
             1.0f, -1.0f,  1.0f
        };

        glGenVertexArrays(1, &pass->vao);
        glGenBuffers(1, &pass->vbo);
        glBindVertexArray(pass->vao);
        glBindBuffer(GL_ARRAY_BUFFER, pass->vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(tetra_vertices), tetra_vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glBindVertexArray(0);
    }

    static void find_uniforms(chr::LightMarkerPass* pass) {
        pass->uniform_model = glGetUniformLocation(pass->shader_program, "model");
        pass->uniform_view = glGetUniformLocation(pass->shader_program, "view");
        pass->uniform_projection = glGetUniformLocation(pass->shader_program, "projection");
        pass->uniform_color = glGetUniformLocation(pass->shader_program, "uColor");
    }
}

namespace chr {

    int LightMarkerPass::init() {
        clear();

        create_marker_mesh(this);
        shader_program = graphics_util::create_shader_program_from_files(
            LIGHT_MARKER_VERTEX_SHADER_PATH,
            LIGHT_MARKER_FRAGMENT_SHADER_PATH);
        if (shader_program == 0) {
            clear();
            return -1;
        }

        find_uniforms(this);
        return 0;
    }

    int LightMarkerPass::reload_shaders() {
        const uint32_t new_shader_program = graphics_util::create_shader_program_from_files(
            LIGHT_MARKER_VERTEX_SHADER_PATH,
            LIGHT_MARKER_FRAGMENT_SHADER_PATH);
        if (new_shader_program == 0) {
            return -1;
        }

        const uint32_t old_shader_program = shader_program;
        shader_program = new_shader_program;
        find_uniforms(this);
        if (old_shader_program != 0) {
            glDeleteProgram(old_shader_program);
        }
        return 0;
    }

    void LightMarkerPass::clear() {
        if (shader_program != 0) {
            glDeleteProgram(shader_program);
            shader_program = 0;
        }
        if (vbo != 0) {
            glDeleteBuffers(1, &vbo);
            vbo = 0;
        }
        if (vao != 0) {
            glDeleteVertexArrays(1, &vao);
            vao = 0;
        }

        uniform_model = -1;
        uniform_view = -1;
        uniform_projection = -1;
        uniform_color = -1;
    }

    void LightMarkerPass::render(
        const std::array<PointLightDesc, 5>& point_lights,
        const glm::mat4& mat_projection,
        const glm::mat4& mat_view,
        int width,
        int height) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, width, height);
        glUseProgram(shader_program);
        glUniformMatrix4fv(uniform_view, 1, GL_FALSE, &mat_view[0][0]);
        glUniformMatrix4fv(uniform_projection, 1, GL_FALSE, &mat_projection[0][0]);
        glBindVertexArray(vao);
        glDisable(GL_DEPTH_TEST);

        for (const auto& point_light : point_lights) {
            glm::mat4 mat_model = glm::translate(glm::mat4(1.0f), point_light.position);
            mat_model = glm::scale(mat_model, glm::vec3(LIGHT_MARKER_SCALE));
            glUniformMatrix4fv(uniform_model, 1, GL_FALSE, &mat_model[0][0]);
            glUniform3fv(uniform_color, 1, &point_light.color[0]);
            glDrawArrays(GL_TRIANGLES, 0, 12);
        }

        glEnable(GL_DEPTH_TEST);
        glBindVertexArray(0);
    }

}
