#pragma once

#include <array>
#include <cstdint>

#include <glm/glm.hpp>

#include "ssao_pass.h"
#include "tone_mapping_pass.h"
#include "debug_preview_pass.h"
#include "deferred_lighting_pass.h"
#include "fullscreen_quad.h"
#include "g_buffer_pass.h"
#include "hdr_scene_target.h"
#include "light_marker_pass.h"
#include "shadow_pass.h"

namespace chr {

    struct Renderer {
        GBufferPass g_buffer_pass;
        ShadowPass shadow_pass;
        HdrSceneTarget hdr_scene_target;
        SSAOPass ssao_pass;
        FullscreenQuad fullscreen_quad;
        DeferredLightingPass deferred_lighting_pass;
        ToneMappingPass tone_mapping_pass;
        uint32_t debug_environment_texture = 0;
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
        void draw_lit_frame(const glm::mat4& mat_projection, const glm::mat4& mat_view);
        void draw_light_markers(const glm::mat4& mat_projection, const glm::mat4& mat_view);
        void draw_debug_view(DebugViewMode mode);
        static void bind_default_framebuffer(int width, int height);
    };

}
