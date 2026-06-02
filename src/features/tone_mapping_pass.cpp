#include "features/tone_mapping_pass.h"

#include <glad/glad.h>

#include "core/graphics.h"

namespace {
    constexpr const char* FULLSCREEN_VERTEX_SHADER_PATH = "assets/shaders/fullscreen.vert";
    constexpr const char* TONE_MAP_FRAGMENT_SHADER_PATH = "assets/shaders/tone_map.frag";
    constexpr const char* FXAA_FRAGMENT_SHADER_PATH = "assets/shaders/fxaa.frag";

    static void find_uniforms(chr::ToneMappingPass* pass) {
        pass->uniform_scene_color_tone_map = graphics::get_uniform_location(pass->tone_map_shader, "uSceneColor");
        pass->uniform_exposure_tone_map = graphics::get_uniform_location(pass->tone_map_shader, "uExposure");
        pass->uniform_scene_color_fxaa = graphics::get_uniform_location(pass->fxaa_shader, "uSceneColor");
    }
}

namespace chr {

    int ToneMappingPass::init() {
        clear();

        tone_map_shader = graphics::create_shader_program_from_files(
            FULLSCREEN_VERTEX_SHADER_PATH,
            TONE_MAP_FRAGMENT_SHADER_PATH);
        if (tone_map_shader == 0) {
            return -1;
        }

        fxaa_shader = graphics::create_shader_program_from_files(
            FULLSCREEN_VERTEX_SHADER_PATH,
            FXAA_FRAGMENT_SHADER_PATH);
        if (fxaa_shader == 0) {
            return -1;
        }

        find_uniforms(this);
        return 0;
    }

    int ToneMappingPass::reload_shaders() {
        const uint32_t new_tone_map_shader = graphics::create_shader_program_from_files(
            FULLSCREEN_VERTEX_SHADER_PATH,
            TONE_MAP_FRAGMENT_SHADER_PATH);
        if (new_tone_map_shader == 0) {
            return -1;
        }

        const uint32_t new_fxaa_shader = graphics::create_shader_program_from_files(
            FULLSCREEN_VERTEX_SHADER_PATH,
            FXAA_FRAGMENT_SHADER_PATH);
        if (new_fxaa_shader == 0) {
            glDeleteProgram(new_tone_map_shader);
            return -1;
        }

        const uint32_t old_tone_map_shader = tone_map_shader;
        const uint32_t old_fxaa_shader = fxaa_shader;
        tone_map_shader = new_tone_map_shader;
        fxaa_shader = new_fxaa_shader;
        find_uniforms(this);
        if (old_tone_map_shader != 0) {
            glDeleteProgram(old_tone_map_shader);
        }
        if (old_fxaa_shader != 0) {
            glDeleteProgram(old_fxaa_shader);
        }
        return 0;
    }

    int ToneMappingPass::init_tone_map_resources(int width, int height) {
        clear_tone_map_resources();

        glGenFramebuffers(1, &framebuffer_tone_mapped);
        glGenTextures(1, &texture_tone_mapped);

        glBindTexture(GL_TEXTURE_2D, texture_tone_mapped);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_tone_mapped);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_tone_mapped, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            clear_tone_map_resources();
            return -1;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        this->width = width;
        this->height = height;
        return 0;
    }

    void ToneMappingPass::clear_tone_map_resources() {
        if (texture_tone_mapped != 0) {
            glDeleteTextures(1, &texture_tone_mapped);
            texture_tone_mapped = 0;
        }
        if (framebuffer_tone_mapped != 0) {
            glDeleteFramebuffers(1, &framebuffer_tone_mapped);
            framebuffer_tone_mapped = 0;
        }
    }

    void ToneMappingPass::clear() {
        if (tone_map_shader != 0) {
            glDeleteProgram(tone_map_shader);
            tone_map_shader = 0;
        }
        if (fxaa_shader != 0) {
            glDeleteProgram(fxaa_shader);
            fxaa_shader = 0;
        }
        clear_tone_map_resources();
        uniform_scene_color_tone_map = -1;
        uniform_exposure_tone_map = -1;
        uniform_scene_color_fxaa = -1;
    }

    void ToneMappingPass::render(
        uint32_t texture_scene_color,
        uint32_t fullscreen_quad_vao,
        int width,
        int height) {
        if (width != this->width || height != this->height) {
            init_tone_map_resources(width, height);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_tone_mapped);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(tone_map_shader);
        graphics::set_uniform_int(uniform_scene_color_tone_map, 0);
        graphics::set_uniform_float(uniform_exposure_tone_map, exposure);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_scene_color);
        glBindVertexArray(fullscreen_quad_vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        if (enable_fxaa) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, width, height);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glUseProgram(fxaa_shader);
            graphics::set_uniform_int(uniform_scene_color_fxaa, 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture_tone_mapped);
            glBindVertexArray(fullscreen_quad_vao);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        } else {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, width, height);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glUseProgram(tone_map_shader);
            graphics::set_uniform_int(uniform_scene_color_tone_map, 0);
            graphics::set_uniform_float(uniform_exposure_tone_map, exposure);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture_tone_mapped);
            glBindVertexArray(fullscreen_quad_vao);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }

}
