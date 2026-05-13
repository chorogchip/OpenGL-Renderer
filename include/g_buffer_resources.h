#pragma once

#include <array>
#include <cstdint>

#include <glm/glm.hpp>

#include "ssao_pass.h"
#include "tone_mapping_pass.h"
#include "debug_preview_pass.h"
#include "deferred_lighting_pass.h"
#include "light_marker_pass.h"
#include "shadow_pass.h"

namespace chr {

    struct GBufferResources {
        uint32_t framebuffer = 0;
        uint32_t texture_albedo = 0;
        uint32_t texture_normal = 0;
        uint32_t texture_material = 0;
        uint32_t texture_depth = 0;
        ShadowPass shadow_pass;
        uint32_t hdr_framebuffer = 0;
        uint32_t texture_scene_color = 0;
        SSAOPass ssao_pass;
        uint32_t quad_vao = 0;
        uint32_t quad_vbo = 0;
        DeferredLightingPass deferred_lighting_pass;
        ToneMappingPass tone_mapping_pass;
        DebugPreviewPass debug_preview_pass;
        LightMarkerPass light_marker_pass;
        int width = 0;
        int height = 0;
        int init(int width, int height);
        int resize(int width, int height);
        void clear();
        glm::mat4 get_directional_light_view_projection() const;
        void bind_for_shadow_pass();
        void bind_for_geometry_pass();
        void draw_lighting_pass(const glm::mat4& mat_projection, const glm::mat4& mat_view);
        void draw_light_markers(const glm::mat4& mat_projection, const glm::mat4& mat_view);
        void draw_debug_views();
        static void bind_default_framebuffer(int width, int height);
    };

}
