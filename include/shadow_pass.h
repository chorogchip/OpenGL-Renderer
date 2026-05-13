#pragma once

#include <cstdint>

#include <glm/glm.hpp>

namespace chr {

    struct ShadowPass {
        uint32_t framebuffer = 0;
        uint32_t texture_depth = 0;

        int init();
        void clear();
        void bind();
        glm::mat4 light_view_projection() const;
        uint32_t depth_texture() const;
    };

}
