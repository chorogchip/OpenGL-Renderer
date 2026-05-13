#pragma once

#include <cstdint>

namespace chr {

    struct HdrSceneTarget {
        uint32_t framebuffer = 0;
        uint32_t texture_scene_color = 0;
        int width = 0;
        int height = 0;

        int init(int width, int height);
        int resize(int width, int height);
        void clear();
    };

}
