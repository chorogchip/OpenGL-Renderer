#include "debug_preview_pass.h"

#include <glad/glad.h>

#include "graphics_util.h"

namespace {
    constexpr const char* FULLSCREEN_VERTEX_SHADER_PATH = "assets/shaders/deferred_light.vert";
    constexpr const char* DEBUG_FRAGMENT_SHADER_PATH = "assets/shaders/debug_buffer.frag";

    static void find_uniforms(chr::DebugPreviewPass* pass) {
        pass->uniform_texture = glGetUniformLocation(pass->shader_program, "uTexture");
        pass->uniform_cubemap_texture = glGetUniformLocation(pass->shader_program, "uCubemapTexture");
        pass->uniform_mode = glGetUniformLocation(pass->shader_program, "uMode");
    }
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

        find_uniforms(this);
        return 0;
    }

    int DebugPreviewPass::reload_shaders() {
        const uint32_t new_shader_program = graphics_util::create_shader_program_from_files(
            FULLSCREEN_VERTEX_SHADER_PATH,
            DEBUG_FRAGMENT_SHADER_PATH);
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

    void DebugPreviewPass::clear() {
        if (shader_program != 0) {
            glDeleteProgram(shader_program);
            shader_program = 0;
        }
        uniform_texture = -1;
        uniform_cubemap_texture = -1;
        uniform_mode = -1;
    }

    void DebugPreviewPass::render(
        const DebugPreviewTextures& textures,
        DebugViewMode mode,
        uint32_t fullscreen_quad_vao,
        int width,
        int height) {
        if (mode == DebugViewMode::Final) {
            return;
        }

        uint32_t preview_texture = 0;
        GLenum texture_target = GL_TEXTURE_2D;
        switch (mode) {
        case DebugViewMode::Albedo:
            preview_texture = textures.albedo;
            break;
        case DebugViewMode::Normal:
            preview_texture = textures.normal;
            break;
        case DebugViewMode::Depth:
            preview_texture = textures.depth;
            break;
        case DebugViewMode::Ssao:
            preview_texture = textures.ssao;
            break;
        case DebugViewMode::Metallic:
        case DebugViewMode::Roughness:
        case DebugViewMode::Ao:
            preview_texture = textures.material;
            break;
        case DebugViewMode::Emissive:
            preview_texture = textures.emissive;
            break;
        case DebugViewMode::Environment:
            preview_texture = textures.environment;
            break;
        case DebugViewMode::EnvironmentCubemap:
            preview_texture = textures.environment_cubemap;
            texture_target = GL_TEXTURE_CUBE_MAP;
            break;
        case DebugViewMode::Irradiance:
            preview_texture = textures.irradiance;
            texture_target = GL_TEXTURE_CUBE_MAP;
            break;
        case DebugViewMode::Prefilter:
        case DebugViewMode::PrefilterMip1:
        case DebugViewMode::PrefilterMip2:
        case DebugViewMode::PrefilterMip3:
        case DebugViewMode::PrefilterMip4:
            preview_texture = textures.prefilter;
            texture_target = GL_TEXTURE_CUBE_MAP;
            break;
        case DebugViewMode::BrdfLut:
            preview_texture = textures.brdf_lut;
            break;
        case DebugViewMode::Final:
        case DebugViewMode::Count:
            return;
        }

        glUseProgram(shader_program);
        glUniform1i(uniform_texture, 0);
        glUniform1i(uniform_cubemap_texture, 0);
        glBindVertexArray(fullscreen_quad_vao);
        glDisable(GL_DEPTH_TEST);
        glViewport(0, 0, width, height);
        glUniform1i(uniform_mode, static_cast<int>(mode));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(texture_target, preview_texture);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glEnable(GL_DEPTH_TEST);
        glBindVertexArray(0);
    }

}
