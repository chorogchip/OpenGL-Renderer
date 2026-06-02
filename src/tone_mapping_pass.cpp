#include "tone_mapping_pass.h"

#include <glad/glad.h>

#include "graphics_util.h"

namespace {
    constexpr const char* FULLSCREEN_VERTEX_SHADER_PATH = "assets/shaders/fullscreen.vert";
    constexpr const char* TONE_MAP_FRAGMENT_SHADER_PATH = "assets/shaders/post_process.frag";

    static void find_uniforms(chr::ToneMappingPass* pass) {
        pass->uniform_scene_color = graphics_util::get_uniform_location(pass->shader_program, "uSceneColor");
        pass->uniform_exposure = graphics_util::get_uniform_location(pass->shader_program, "uExposure");
        pass->uniform_enable_fxaa = graphics_util::get_uniform_location(pass->shader_program, "uEnableFxaa");
    }
}

namespace chr {

    int ToneMappingPass::init() {
        clear();

        shader_program = graphics_util::create_shader_program_from_files(
            FULLSCREEN_VERTEX_SHADER_PATH,
            TONE_MAP_FRAGMENT_SHADER_PATH);
        if (shader_program == 0) {
            return -1;
        }

        find_uniforms(this);
        return 0;
    }

    int ToneMappingPass::reload_shaders() {
        const uint32_t new_shader_program = graphics_util::create_shader_program_from_files(
            FULLSCREEN_VERTEX_SHADER_PATH,
            TONE_MAP_FRAGMENT_SHADER_PATH);
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

    void ToneMappingPass::clear() {
        if (shader_program != 0) {
            glDeleteProgram(shader_program);
            shader_program = 0;
        }
        uniform_scene_color = -1;
        uniform_exposure = -1;
        uniform_enable_fxaa = -1;
    }

    void ToneMappingPass::render(
        uint32_t texture_scene_color,
        uint32_t fullscreen_quad_vao,
        int width,
        int height) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shader_program);
        graphics_util::set_uniform_int(uniform_scene_color, 0);
        graphics_util::set_uniform_float(uniform_exposure, exposure);
        graphics_util::set_uniform_int(uniform_enable_fxaa, enable_fxaa ? 1 : 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_scene_color);
        glBindVertexArray(fullscreen_quad_vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

}
