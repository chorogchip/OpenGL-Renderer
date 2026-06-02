#pragma once

#include <cstdint>

// Deferred rendering G-buffer pass
// Renders scene geometry into multiple textures:
// - Albedo (diffuse color)
// - Normal (world-space normal)
// - Material (metallic, roughness, AO)
// - Emissive (self-illumination)
// - Depth (for reconstruction)
namespace chr {

    struct GBufferPass {
        uint32_t framebuffer = 0;
        uint32_t texture_albedo = 0;
        uint32_t texture_normal = 0;
        uint32_t texture_material = 0;
        uint32_t texture_emissive = 0;
        uint32_t texture_depth = 0;
        int width = 0;
        int height = 0;

        int init(int width, int height);
        int resize(int width, int height);
        void clear();
        void bind_for_geometry_pass();
    };

}
