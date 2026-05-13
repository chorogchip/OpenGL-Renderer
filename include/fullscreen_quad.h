#pragma once

#include <cstdint>

namespace chr {

    struct FullscreenQuad {
        uint32_t vao = 0;
        uint32_t vbo = 0;

        void init();
        void clear();
        void draw() const;
    };

}
