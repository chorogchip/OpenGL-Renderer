#include "features/ssao_pass.h"

#include <array>
#include <cstdio>
#include <iostream>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "core/graphics.h"

namespace {
    constexpr const char* FULLSCREEN_VERTEX_SHADER_PATH = "assets/shaders/fullscreen.vert";
    constexpr const char* SSAO_FRAGMENT_SHADER_PATH = "assets/shaders/ssao.frag";
    constexpr const char* SSAO_BLUR_FRAGMENT_SHADER_PATH = "assets/shaders/ssao_blur.frag";
    constexpr int SSAO_KERNEL_SIZE = 16;
    constexpr int SSAO_NOISE_SIZE = 4;

    const std::array<glm::vec3, SSAO_KERNEL_SIZE> SSAO_KERNEL = {{
        glm::vec3(0.078f, 0.068f, 0.085f),
        glm::vec3(-0.113f, 0.041f, 0.126f),
        glm::vec3(0.042f, -0.099f, 0.159f),
        glm::vec3(0.154f, 0.119f, 0.192f),
        glm::vec3(-0.187f, -0.056f, 0.226f),
        glm::vec3(0.238f, -0.144f, 0.261f),
        glm::vec3(-0.052f, 0.265f, 0.308f),
        glm::vec3(0.309f, 0.032f, 0.348f),
        glm::vec3(-0.286f, 0.197f, 0.402f),
        glm::vec3(0.128f, -0.332f, 0.446f),
        glm::vec3(0.373f, 0.231f, 0.503f),
        glm::vec3(-0.401f, -0.154f, 0.558f),
        glm::vec3(0.214f, 0.427f, 0.612f),
        glm::vec3(-0.486f, 0.091f, 0.688f),
        glm::vec3(0.447f, -0.366f, 0.764f),
        glm::vec3(-0.265f, 0.541f, 0.892f)
    }};

    const std::array<glm::vec3, SSAO_NOISE_SIZE * SSAO_NOISE_SIZE> SSAO_NOISE = {{
        glm::vec3( 1.0f,  0.0f, 0.0f), glm::vec3(-1.0f,  0.0f, 0.0f),
        glm::vec3( 0.0f,  1.0f, 0.0f), glm::vec3( 0.0f, -1.0f, 0.0f),
        glm::vec3( 0.7f,  0.7f, 0.0f), glm::vec3(-0.7f,  0.7f, 0.0f),
        glm::vec3( 0.7f, -0.7f, 0.0f), glm::vec3(-0.7f, -0.7f, 0.0f),
        glm::vec3( 0.92f, 0.38f, 0.0f), glm::vec3(-0.92f, 0.38f, 0.0f),
        glm::vec3( 0.92f,-0.38f, 0.0f), glm::vec3(-0.92f,-0.38f, 0.0f),
        glm::vec3( 0.38f, 0.92f, 0.0f), glm::vec3(-0.38f, 0.92f, 0.0f),
        glm::vec3( 0.38f,-0.92f, 0.0f), glm::vec3(-0.38f,-0.92f, 0.0f)
    }};

    static int create_attachments(chr::SSAOPass* pass, int width, int height) {
        glGenFramebuffers(1, &pass->framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, pass->framebuffer);

        glGenTextures(1, &pass->texture_ssao);
        glBindTexture(GL_TEXTURE_2D, pass->texture_ssao);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pass->texture_ssao, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cout << "Err: SSAO framebuffer is incomplete." << std::endl;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return -1;
        }

        glGenFramebuffers(1, &pass->blur_framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, pass->blur_framebuffer);

        glGenTextures(1, &pass->texture_ssao_blur);
        glBindTexture(GL_TEXTURE_2D, pass->texture_ssao_blur);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pass->texture_ssao_blur, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cout << "Err: SSAO blur framebuffer is incomplete." << std::endl;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return -1;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        pass->width = width;
        pass->height = height;
        return 0;
    }

    static void create_noise_texture(chr::SSAOPass* pass) {
        glGenTextures(1, &pass->texture_noise);
        glBindTexture(GL_TEXTURE_2D, pass->texture_noise);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGB16F,
            SSAO_NOISE_SIZE, SSAO_NOISE_SIZE, 0, GL_RGB, GL_FLOAT, SSAO_NOISE.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    static void find_ssao_uniforms(chr::SSAOPass* pass) {
        pass->uniform_g_normal = glGetUniformLocation(pass->shader_program, "gNormal");
        pass->uniform_g_depth = glGetUniformLocation(pass->shader_program, "gDepth");
        pass->uniform_noise_texture = glGetUniformLocation(pass->shader_program, "uNoiseTexture");
        pass->uniform_projection = glGetUniformLocation(pass->shader_program, "uProjection");
        pass->uniform_inverse_projection = glGetUniformLocation(pass->shader_program, "uInverseProjection");
        pass->uniform_noise_scale = glGetUniformLocation(pass->shader_program, "uNoiseScale");
        for (int i = 0; i < SSAO_KERNEL_SIZE; ++i) {
            char uniform_name[32];
            snprintf(uniform_name, sizeof(uniform_name), "uSamples[%d]", i);
            pass->uniform_samples[i] = glGetUniformLocation(pass->shader_program, uniform_name);
        }
    }

    static void find_blur_uniforms(chr::SSAOPass* pass) {
        pass->uniform_blur_texture = glGetUniformLocation(pass->blur_shader_program, "uTexture");
    }
}

namespace chr {

    int SSAOPass::init(int width, int height) {
        clear();

        if (create_attachments(this, width, height) != 0) {
            clear();
            return -1;
        }
        create_noise_texture(this);

        shader_program = graphics::create_shader_program_from_files(
            FULLSCREEN_VERTEX_SHADER_PATH,
            SSAO_FRAGMENT_SHADER_PATH);
        if (shader_program == 0) {
            clear();
            return -1;
        }

        find_ssao_uniforms(this);

        blur_shader_program = graphics::create_shader_program_from_files(
            FULLSCREEN_VERTEX_SHADER_PATH,
            SSAO_BLUR_FRAGMENT_SHADER_PATH);
        if (blur_shader_program == 0) {
            clear();
            return -1;
        }

        find_blur_uniforms(this);
        return 0;
    }

    int SSAOPass::reload_shaders() {
        const uint32_t new_shader_program = graphics::create_shader_program_from_files(
            FULLSCREEN_VERTEX_SHADER_PATH,
            SSAO_FRAGMENT_SHADER_PATH);
        if (new_shader_program == 0) {
            return -1;
        }

        const uint32_t new_blur_shader_program = graphics::create_shader_program_from_files(
            FULLSCREEN_VERTEX_SHADER_PATH,
            SSAO_BLUR_FRAGMENT_SHADER_PATH);
        if (new_blur_shader_program == 0) {
            glDeleteProgram(new_shader_program);
            return -1;
        }

        const uint32_t old_shader_program = shader_program;
        const uint32_t old_blur_shader_program = blur_shader_program;
        shader_program = new_shader_program;
        blur_shader_program = new_blur_shader_program;
        find_ssao_uniforms(this);
        find_blur_uniforms(this);
        if (old_blur_shader_program != 0) {
            glDeleteProgram(old_blur_shader_program);
        }
        if (old_shader_program != 0) {
            glDeleteProgram(old_shader_program);
        }
        return 0;
    }

    int SSAOPass::resize(int width, int height) {
        if (width <= 0 || height <= 0) {
            return -1;
        }
        if (this->width == width && this->height == height) {
            return 0;
        }

        if (texture_noise != 0) {
            glDeleteTextures(1, &texture_noise);
            texture_noise = 0;
        }
        if (texture_ssao_blur != 0) {
            glDeleteTextures(1, &texture_ssao_blur);
            texture_ssao_blur = 0;
        }
        if (texture_ssao != 0) {
            glDeleteTextures(1, &texture_ssao);
            texture_ssao = 0;
        }
        if (blur_framebuffer != 0) {
            glDeleteFramebuffers(1, &blur_framebuffer);
            blur_framebuffer = 0;
        }
        if (framebuffer != 0) {
            glDeleteFramebuffers(1, &framebuffer);
            framebuffer = 0;
        }

        if (create_attachments(this, width, height) != 0) {
            clear();
            return -1;
        }
        create_noise_texture(this);
        return 0;
    }

    void SSAOPass::clear() {
        if (blur_shader_program != 0) {
            glDeleteProgram(blur_shader_program);
            blur_shader_program = 0;
        }
        if (shader_program != 0) {
            glDeleteProgram(shader_program);
            shader_program = 0;
        }
        if (texture_noise != 0) {
            glDeleteTextures(1, &texture_noise);
            texture_noise = 0;
        }
        if (texture_ssao_blur != 0) {
            glDeleteTextures(1, &texture_ssao_blur);
            texture_ssao_blur = 0;
        }
        if (texture_ssao != 0) {
            glDeleteTextures(1, &texture_ssao);
            texture_ssao = 0;
        }
        if (blur_framebuffer != 0) {
            glDeleteFramebuffers(1, &blur_framebuffer);
            blur_framebuffer = 0;
        }
        if (framebuffer != 0) {
            glDeleteFramebuffers(1, &framebuffer);
            framebuffer = 0;
        }

        width = 0;
        height = 0;
        uniform_g_normal = -1;
        uniform_g_depth = -1;
        uniform_noise_texture = -1;
        uniform_projection = -1;
        uniform_inverse_projection = -1;
        uniform_noise_scale = -1;
        uniform_samples.fill(-1);
        uniform_blur_texture = -1;
    }

    void SSAOPass::render(
        uint32_t texture_normal,
        uint32_t texture_depth,
        uint32_t fullscreen_quad_vao,
        const glm::mat4& mat_projection) {
        const glm::vec2 noise_scale = glm::vec2(
            static_cast<float>(width) / SSAO_NOISE_SIZE,
            static_cast<float>(height) / SSAO_NOISE_SIZE);

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shader_program);
        glUniform1i(uniform_g_normal, 0);
        glUniform1i(uniform_g_depth, 1);
        glUniform1i(uniform_noise_texture, 2);
        glUniformMatrix4fv(uniform_projection, 1, GL_FALSE, &mat_projection[0][0]);
        const glm::mat4 inverse_projection = glm::inverse(mat_projection);
        glUniformMatrix4fv(uniform_inverse_projection, 1, GL_FALSE, &inverse_projection[0][0]);
        glUniform2fv(uniform_noise_scale, 1, &noise_scale[0]);
        for (int i = 0; i < SSAO_KERNEL_SIZE; ++i) {
            glUniform3fv(uniform_samples[i], 1, &SSAO_KERNEL[i][0]);
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_normal);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture_depth);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, texture_noise);
        glBindVertexArray(fullscreen_quad_vao);
        glDisable(GL_DEPTH_TEST);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindFramebuffer(GL_FRAMEBUFFER, blur_framebuffer);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(blur_shader_program);
        glUniform1i(uniform_blur_texture, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_ssao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    uint32_t SSAOPass::blurred_texture() const {
        return texture_ssao_blur;
    }

}
