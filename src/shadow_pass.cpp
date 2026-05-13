#include "shadow_pass.h"

#include <iostream>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

namespace {
    const glm::vec3 LIGHT_DIRECTION = glm::normalize(glm::vec3(-0.6f, -1.0f, -0.35f));
    constexpr int SHADOW_MAP_SIZE = 2048;
    constexpr float SHADOW_ORTHO_HALF_EXTENT = 18.0f;
    constexpr float SHADOW_NEAR_PLANE = 1.0f;
    constexpr float SHADOW_FAR_PLANE = 60.0f;
    constexpr float SHADOW_LIGHT_DISTANCE = 24.0f;
    const glm::vec3 SHADOW_TARGET = glm::vec3(0.0f, 6.0f, 0.0f);
}

namespace chr {

    int ShadowPass::init() {
        clear();

        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

        glGenTextures(1, &texture_depth);
        glBindTexture(GL_TEXTURE_2D, texture_depth);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
            SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        constexpr float border_color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture_depth, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cout << "Err: Shadow framebuffer is incomplete." << std::endl;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return -1;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return 0;
    }

    void ShadowPass::clear() {
        if (texture_depth != 0) {
            glDeleteTextures(1, &texture_depth);
            texture_depth = 0;
        }
        if (framebuffer != 0) {
            glDeleteFramebuffers(1, &framebuffer);
            framebuffer = 0;
        }
    }

    void ShadowPass::bind() {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
    }

    glm::mat4 ShadowPass::light_view_projection() const {
        const glm::vec3 light_position = SHADOW_TARGET - LIGHT_DIRECTION * SHADOW_LIGHT_DISTANCE;
        const glm::mat4 light_view = glm::lookAt(light_position, SHADOW_TARGET, glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::mat4 light_projection = glm::ortho(
            -SHADOW_ORTHO_HALF_EXTENT, SHADOW_ORTHO_HALF_EXTENT,
            -SHADOW_ORTHO_HALF_EXTENT, SHADOW_ORTHO_HALF_EXTENT,
            SHADOW_NEAR_PLANE, SHADOW_FAR_PLANE);
        return light_projection * light_view;
    }

    uint32_t ShadowPass::depth_texture() const {
        return texture_depth;
    }

}
