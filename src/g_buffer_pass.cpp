#include "g_buffer_pass.h"

#include <iostream>

#include <glad/glad.h>

namespace chr {

    int GBufferPass::init(int width, int height) {
        clear();

        if (width <= 0 || height <= 0) {
            std::cout << "Err: Invalid G-buffer size." << std::endl;
            return -1;
        }

        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

        glGenTextures(1, &texture_albedo);
        glBindTexture(GL_TEXTURE_2D, texture_albedo);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_albedo, 0);

        glGenTextures(1, &texture_normal);
        glBindTexture(GL_TEXTURE_2D, texture_normal);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, texture_normal, 0);

        glGenTextures(1, &texture_material);
        glBindTexture(GL_TEXTURE_2D, texture_material);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, texture_material, 0);

        glGenTextures(1, &texture_emissive);
        glBindTexture(GL_TEXTURE_2D, texture_emissive);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, texture_emissive, 0);

        glGenTextures(1, &texture_depth);
        glBindTexture(GL_TEXTURE_2D, texture_depth);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
            width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture_depth, 0);

        constexpr GLenum draw_buffers[] = {
            GL_COLOR_ATTACHMENT0,
            GL_COLOR_ATTACHMENT1,
            GL_COLOR_ATTACHMENT2,
            GL_COLOR_ATTACHMENT3
        };
        glDrawBuffers(4, draw_buffers);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cout << "Err: G-buffer framebuffer is incomplete." << std::endl;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return -1;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        this->width = width;
        this->height = height;
        return 0;
    }

    int GBufferPass::resize(int width, int height) {
        if (this->width == width && this->height == height) {
            return 0;
        }
        return init(width, height);
    }

    void GBufferPass::clear() {
        if (texture_depth != 0) {
            glDeleteTextures(1, &texture_depth);
            texture_depth = 0;
        }
        if (texture_normal != 0) {
            glDeleteTextures(1, &texture_normal);
            texture_normal = 0;
        }
        if (texture_material != 0) {
            glDeleteTextures(1, &texture_material);
            texture_material = 0;
        }
        if (texture_emissive != 0) {
            glDeleteTextures(1, &texture_emissive);
            texture_emissive = 0;
        }
        if (texture_albedo != 0) {
            glDeleteTextures(1, &texture_albedo);
            texture_albedo = 0;
        }
        if (framebuffer != 0) {
            glDeleteFramebuffers(1, &framebuffer);
            framebuffer = 0;
        }

        width = 0;
        height = 0;
    }

    void GBufferPass::bind_for_geometry_pass() {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glViewport(0, 0, width, height);
    }

}
