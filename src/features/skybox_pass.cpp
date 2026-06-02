#include "features/skybox_pass.h"

#include <iostream>

#include <glad/glad.h>

namespace {
    constexpr const char* SKYBOX_VERTEX_SHADER_PATH = "assets/shaders/skybox.vert";
    constexpr const char* SKYBOX_FRAGMENT_SHADER_PATH = "assets/shaders/skybox.frag";
    constexpr const char* EQUIRECTANGULAR_FRAGMENT_SHADER_PATH = "assets/shaders/equirectangular_to_cubemap.frag";
    constexpr const char* IRRADIANCE_FRAGMENT_SHADER_PATH = "assets/shaders/irradiance_convolution.frag";
    constexpr const char* PREFILTER_FRAGMENT_SHADER_PATH = "assets/shaders/prefilter_environment.frag";
    constexpr const char* BRDF_LUT_VERTEX_SHADER_PATH = "assets/shaders/brdf_lut.vert";
    constexpr const char* BRDF_LUT_FRAGMENT_SHADER_PATH = "assets/shaders/brdf_lut.frag";

    constexpr float CUBE_VERTICES[] = {
        -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f
    };

    constexpr float BRDF_QUAD_VERTICES[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f
    };

    static void find_uniforms(chr::SkyboxPass* pass) {
        pass->uniform_equirectangular_map =
            glGetUniformLocation(pass->equirectangular_shader_program, "uEquirectangularMap");
        pass->uniform_capture_projection =
            glGetUniformLocation(pass->equirectangular_shader_program, "projection");
        pass->uniform_capture_view =
            glGetUniformLocation(pass->equirectangular_shader_program, "view");
        pass->uniform_irradiance_environment_map =
            glGetUniformLocation(pass->irradiance_shader_program, "uEnvironmentMap");
        pass->uniform_irradiance_projection =
            glGetUniformLocation(pass->irradiance_shader_program, "projection");
        pass->uniform_irradiance_view =
            glGetUniformLocation(pass->irradiance_shader_program, "view");
        pass->uniform_prefilter_environment_map =
            glGetUniformLocation(pass->prefilter_shader_program, "uEnvironmentMap");
        pass->uniform_prefilter_projection =
            glGetUniformLocation(pass->prefilter_shader_program, "projection");
        pass->uniform_prefilter_view =
            glGetUniformLocation(pass->prefilter_shader_program, "view");
        pass->uniform_prefilter_roughness =
            glGetUniformLocation(pass->prefilter_shader_program, "uRoughness");
        pass->uniform_skybox_map = glGetUniformLocation(pass->skybox_shader_program, "uEnvironmentMap");
        pass->uniform_projection = glGetUniformLocation(pass->skybox_shader_program, "projection");
        pass->uniform_view = glGetUniformLocation(pass->skybox_shader_program, "view");
    }

}

namespace chr {

    int SkyboxPass::init(const std::string& hdr_environment_path) {
        clear();
        this->hdr_environment_path = hdr_environment_path;

        if (hdr_environment_path.empty()) {
            std::cout << "No HDR environment path configured." << std::endl;
            return 0;
        }

        equirectangular_texture = graphics::load_hdr_texture_2d(hdr_environment_path);
        if (equirectangular_texture == 0) {
            return -1;
        }

        equirectangular_shader_program = graphics::create_shader_program_from_files(
            SKYBOX_VERTEX_SHADER_PATH,
            EQUIRECTANGULAR_FRAGMENT_SHADER_PATH);
        skybox_shader_program = graphics::create_shader_program_from_files(
            SKYBOX_VERTEX_SHADER_PATH,
            SKYBOX_FRAGMENT_SHADER_PATH);
        irradiance_shader_program = graphics::create_shader_program_from_files(
            SKYBOX_VERTEX_SHADER_PATH,
            IRRADIANCE_FRAGMENT_SHADER_PATH);
        prefilter_shader_program = graphics::create_shader_program_from_files(
            SKYBOX_VERTEX_SHADER_PATH,
            PREFILTER_FRAGMENT_SHADER_PATH);
        brdf_lut_shader_program = graphics::create_shader_program_from_files(
            BRDF_LUT_VERTEX_SHADER_PATH,
            BRDF_LUT_FRAGMENT_SHADER_PATH);
        if (equirectangular_shader_program == 0 ||
            skybox_shader_program == 0 ||
            irradiance_shader_program == 0 ||
            prefilter_shader_program == 0 ||
            brdf_lut_shader_program == 0) {
            return -1;
        }

        find_uniforms(this);

        glGenVertexArrays(1, &cube_vao);
        glGenBuffers(1, &cube_vbo);
        glBindVertexArray(cube_vao);
        glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_VERTICES), CUBE_VERTICES, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glBindVertexArray(0);

        glGenVertexArrays(1, &brdf_quad_vao);
        glGenBuffers(1, &brdf_quad_vbo);
        glBindVertexArray(brdf_quad_vao);
        glBindBuffer(GL_ARRAY_BUFFER, brdf_quad_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(BRDF_QUAD_VERTICES), BRDF_QUAD_VERTICES, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glBindVertexArray(0);

        graphics::CubemapTextureDesc cubemap_desc{};
        cubemap_desc.size = cubemap_size;
        cubemap_desc.internal_format = GL_RGB16F;
        cubemap_desc.format = GL_RGB;
        cubemap_desc.type = GL_FLOAT;
        cubemap_desc.generate_mipmaps = false;
        environment_cubemap = graphics::create_cubemap_texture(cubemap_desc);
        if (environment_cubemap == 0) {
            return -1;
        }

        cubemap_desc.size = irradiance_size;
        irradiance_cubemap = graphics::create_cubemap_texture(cubemap_desc);
        if (irradiance_cubemap == 0) {
            return -1;
        }

        cubemap_desc.size = prefilter_size;
        cubemap_desc.generate_mipmaps = true;
        prefiltered_environment_cubemap = graphics::create_cubemap_texture(cubemap_desc);
        if (prefiltered_environment_cubemap == 0) {
            return -1;
        }

        glGenTextures(1, &brdf_lut_texture);
        glBindTexture(GL_TEXTURE_2D, brdf_lut_texture);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RG16F,
            brdf_lut_size, brdf_lut_size, 0, GL_RG, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        graphics::CubemapCaptureTarget capture_target{};
        if (graphics::create_cubemap_capture_target(cubemap_size, &capture_target) != 0) {
            return -1;
        }

        glUseProgram(equirectangular_shader_program);
        glUniform1i(uniform_equirectangular_map, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, equirectangular_texture);
        glBindVertexArray(cube_vao);

        const GLboolean depth_was_enabled = glIsEnabled(GL_DEPTH_TEST);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        graphics::render_cubemap_faces(
            capture_target,
            environment_cubemap,
            [&](const graphics::CubemapFaceRenderInfo& face_info) {
            glUniformMatrix4fv(uniform_capture_projection, 1, GL_FALSE, &face_info.projection[0][0]);
            glUniformMatrix4fv(uniform_capture_view, 1, GL_FALSE, &face_info.view[0][0]);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        });

        graphics::destroy_cubemap_capture_target(&capture_target);
        if (graphics::create_cubemap_capture_target(irradiance_size, &capture_target) != 0) {
            return -1;
        }

        glUseProgram(irradiance_shader_program);
        glUniform1i(uniform_irradiance_environment_map, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, environment_cubemap);
        graphics::render_cubemap_faces(
            capture_target,
            irradiance_cubemap,
            [&](const graphics::CubemapFaceRenderInfo& face_info) {
            glUniformMatrix4fv(uniform_irradiance_projection, 1, GL_FALSE, &face_info.projection[0][0]);
            glUniformMatrix4fv(uniform_irradiance_view, 1, GL_FALSE, &face_info.view[0][0]);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        });

        graphics::destroy_cubemap_capture_target(&capture_target);
        if (graphics::create_cubemap_capture_target(prefilter_size, &capture_target) != 0) {
            return -1;
        }

        glUseProgram(prefilter_shader_program);
        glUniform1i(uniform_prefilter_environment_map, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, environment_cubemap);
        for (int mip = 0; mip < prefilter_mip_levels; ++mip) {
            const float roughness = static_cast<float>(mip) /
                static_cast<float>(prefilter_mip_levels - 1);
            glUniform1f(uniform_prefilter_roughness, roughness);
            graphics::render_cubemap_faces(
                capture_target,
                prefiltered_environment_cubemap,
                [&](const graphics::CubemapFaceRenderInfo& face_info) {
                    glUniformMatrix4fv(uniform_prefilter_projection, 1, GL_FALSE, &face_info.projection[0][0]);
                    glUniformMatrix4fv(uniform_prefilter_view, 1, GL_FALSE, &face_info.view[0][0]);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                },
                mip);
        }

        uint32_t brdf_framebuffer = 0;
        uint32_t brdf_renderbuffer = 0;
        glGenFramebuffers(1, &brdf_framebuffer);
        glGenRenderbuffers(1, &brdf_renderbuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, brdf_framebuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, brdf_renderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, brdf_lut_size, brdf_lut_size);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, brdf_renderbuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdf_lut_texture, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cout << "Err: BRDF LUT framebuffer is incomplete." << std::endl;
            glDeleteRenderbuffers(1, &brdf_renderbuffer);
            glDeleteFramebuffers(1, &brdf_framebuffer);
            return -1;
        }
        glViewport(0, 0, brdf_lut_size, brdf_lut_size);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(brdf_lut_shader_program);
        glBindVertexArray(brdf_quad_vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDeleteRenderbuffers(1, &brdf_renderbuffer);
        glDeleteFramebuffers(1, &brdf_framebuffer);

        glDepthFunc(GL_LESS);
        if (!depth_was_enabled) {
            glDisable(GL_DEPTH_TEST);
        }
        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        graphics::destroy_cubemap_capture_target(&capture_target);

        return 0;
    }

    int SkyboxPass::reload_shaders() {
        if (hdr_environment_path.empty()) {
            return 0;
        }

        SkyboxPass rebuilt;
        rebuilt.cubemap_size = cubemap_size;
        rebuilt.irradiance_size = irradiance_size;
        rebuilt.prefilter_size = prefilter_size;
        rebuilt.prefilter_mip_levels = prefilter_mip_levels;
        rebuilt.brdf_lut_size = brdf_lut_size;

        std::cout << "Rebuilding skybox environment maps..." << std::endl;
        if (rebuilt.init(hdr_environment_path) != 0) {
            rebuilt.clear();
            return -1;
        }

        clear();
        *this = rebuilt;
        std::cout << "Skybox environment maps rebuilt." << std::endl;
        return 0;
    }

    void SkyboxPass::clear() {
        if (brdf_lut_texture != 0) {
            glDeleteTextures(1, &brdf_lut_texture);
            brdf_lut_texture = 0;
        }
        if (prefiltered_environment_cubemap != 0) {
            glDeleteTextures(1, &prefiltered_environment_cubemap);
            prefiltered_environment_cubemap = 0;
        }
        if (irradiance_cubemap != 0) {
            glDeleteTextures(1, &irradiance_cubemap);
            irradiance_cubemap = 0;
        }
        if (environment_cubemap != 0) {
            glDeleteTextures(1, &environment_cubemap);
            environment_cubemap = 0;
        }
        if (equirectangular_texture != 0) {
            glDeleteTextures(1, &equirectangular_texture);
            equirectangular_texture = 0;
        }
        if (cube_vbo != 0) {
            glDeleteBuffers(1, &cube_vbo);
            cube_vbo = 0;
        }
        if (cube_vao != 0) {
            glDeleteVertexArrays(1, &cube_vao);
            cube_vao = 0;
        }
        if (brdf_quad_vbo != 0) {
            glDeleteBuffers(1, &brdf_quad_vbo);
            brdf_quad_vbo = 0;
        }
        if (brdf_quad_vao != 0) {
            glDeleteVertexArrays(1, &brdf_quad_vao);
            brdf_quad_vao = 0;
        }
        if (equirectangular_shader_program != 0) {
            glDeleteProgram(equirectangular_shader_program);
            equirectangular_shader_program = 0;
        }
        if (skybox_shader_program != 0) {
            glDeleteProgram(skybox_shader_program);
            skybox_shader_program = 0;
        }
        if (irradiance_shader_program != 0) {
            glDeleteProgram(irradiance_shader_program);
            irradiance_shader_program = 0;
        }
        if (prefilter_shader_program != 0) {
            glDeleteProgram(prefilter_shader_program);
            prefilter_shader_program = 0;
        }
        if (brdf_lut_shader_program != 0) {
            glDeleteProgram(brdf_lut_shader_program);
            brdf_lut_shader_program = 0;
        }

        uniform_equirectangular_map = -1;
        uniform_capture_projection = -1;
        uniform_capture_view = -1;
        uniform_irradiance_environment_map = -1;
        uniform_irradiance_projection = -1;
        uniform_irradiance_view = -1;
        uniform_prefilter_environment_map = -1;
        uniform_prefilter_projection = -1;
        uniform_prefilter_view = -1;
        uniform_prefilter_roughness = -1;
        uniform_skybox_map = -1;
        uniform_projection = -1;
        uniform_view = -1;
    }

    bool SkyboxPass::is_ready() const {
        return environment_cubemap != 0 && skybox_shader_program != 0 && cube_vao != 0;
    }

    void SkyboxPass::render(
        uint32_t framebuffer,
        int width,
        int height,
        const glm::mat4& projection,
        const glm::mat4& view) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glViewport(0, 0, width, height);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (!is_ready()) {
            return;
        }

        const GLboolean depth_was_enabled = glIsEnabled(GL_DEPTH_TEST);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);

        const glm::mat4 rotation_view = glm::mat4(glm::mat3(view));
        glUseProgram(skybox_shader_program);
        glUniform1i(uniform_skybox_map, 0);
        glUniformMatrix4fv(uniform_projection, 1, GL_FALSE, &projection[0][0]);
        glUniformMatrix4fv(uniform_view, 1, GL_FALSE, &rotation_view[0][0]);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, environment_cubemap);
        glBindVertexArray(cube_vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
        if (!depth_was_enabled) {
            glDisable(GL_DEPTH_TEST);
        }
        glBindVertexArray(0);
    }

}
