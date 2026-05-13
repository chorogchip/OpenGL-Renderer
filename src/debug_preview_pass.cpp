#include "debug_preview_pass.h"

#include <glad/glad.h>

#include "graphics_util.h"

namespace {
    constexpr const char* FULLSCREEN_VERTEX_SHADER_PATH = "assets/shaders/deferred_light.vert";
    constexpr const char* DEBUG_FRAGMENT_SHADER_PATH = "assets/shaders/debug_buffer.frag";
}

namespace chr {

    int DebugPreviewPass::init() {
        clear();

        shader_program = graphics_util::create_shader_program_from_files(
            FULLSCREEN_VERTEX_SHADER_PATH,
            DEBUG_FRAGMENT_SHADER_PATH);
        if (shader_program == 0) {
            return -1;
        }

        uniform_texture = glGetUniformLocation(shader_program, "uTexture");
        uniform_mode = glGetUniformLocation(shader_program, "uMode");
        return 0;
    }

    void DebugPreviewPass::clear() {
        if (shader_program != 0) {
            glDeleteProgram(shader_program);
            shader_program = 0;
        }
        uniform_texture = -1;
        uniform_mode = -1;
    }

    void DebugPreviewPass::render(
        const DebugPreviewTextures& textures,
        uint32_t fullscreen_quad_vao,
        int width,
        int height) {
        constexpr int preview_count = 7;
        const int padding = 16;
        const int preview_width = width / 6;
        const int preview_height = height / 6;
        constexpr int previews_per_column = 4;

        const uint32_t preview_textures[preview_count] = {
            textures.albedo,
            textures.normal,
            textures.depth,
            textures.ssao,
            textures.material,
            textures.material,
            textures.material
        };
        const int preview_modes[preview_count] = { 0, 1, 2, 3, 4, 5, 6 };

        glUseProgram(shader_program);
        glUniform1i(uniform_texture, 0);
        glBindVertexArray(fullscreen_quad_vao);
        glDisable(GL_DEPTH_TEST);

        for (int i = 0; i < preview_count; ++i) {
            const int column = i / previews_per_column;
            const int row = i % previews_per_column;
            const int x = width - padding -
                ((2 - column) * preview_width) -
                ((1 - column) * padding);
            const int y = height - padding - ((row + 1) * preview_height) - (row * padding);
            glViewport(x, y, preview_width, preview_height);
            glUniform1i(uniform_mode, preview_modes[i]);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, preview_textures[i]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        glEnable(GL_DEPTH_TEST);
        glBindVertexArray(0);
        glViewport(0, 0, width, height);
    }

}
