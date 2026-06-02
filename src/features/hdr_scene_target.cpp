#include "features/hdr_scene_target.h"

#include <iostream>

#include <glad/glad.h>

namespace chr {

    int HdrSceneTarget::init(int width, int height) {
        clear();

        if (width <= 0 || height <= 0) {
            std::cout << "Err: Invalid HDR target size." << std::endl;
            return -1;
        }

        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

        glGenTextures(1, &texture_scene_color);
        glBindTexture(GL_TEXTURE_2D, texture_scene_color);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_scene_color, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cout << "Err: HDR framebuffer is incomplete." << std::endl;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return -1;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        this->width = width;
        this->height = height;
        return 0;
    }

    int HdrSceneTarget::resize(int width, int height) {
        if (this->width == width && this->height == height) {
            return 0;
        }
        return init(width, height);
    }

    void HdrSceneTarget::clear() {
        if (texture_scene_color != 0) {
            glDeleteTextures(1, &texture_scene_color);
            texture_scene_color = 0;
        }
        if (framebuffer != 0) {
            glDeleteFramebuffers(1, &framebuffer);
            framebuffer = 0;
        }
        width = 0;
        height = 0;
    }

}
