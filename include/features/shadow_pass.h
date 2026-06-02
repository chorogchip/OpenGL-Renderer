#pragma once

#include <cstdint>

#include <glm/glm.hpp>

// Shadow mapping for directional light
// Renders scene depth from light's perspective into shadow map texture
// Used in deferred lighting for shadow calculation
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
