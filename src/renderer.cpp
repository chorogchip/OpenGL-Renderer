#include "renderer.h"

#include <array>

#include <glad/glad.h>

namespace {
    constexpr const char* LIGHTING_VERTEX_SHADER_PATH = "assets/shaders/deferred_light.vert";
    constexpr int POINT_LIGHT_COUNT = 5;
    const std::array<chr::PointLightDesc, POINT_LIGHT_COUNT> POINT_LIGHTS = {{
        {
            glm::vec3(0.0f, 5.5f, 0.0f),
            glm::vec3(1.0f, 0.82f, 0.55f),
            12.0f,
            3.0f
        },
        {
            glm::vec3(-5.5f, 3.0f, 4.0f),
            glm::vec3(1.0f, 0.55f, 0.35f),
            8.0f,
            2.5f
        },
        {
            glm::vec3(5.5f, 3.0f, 4.0f),
            glm::vec3(0.45f, 0.65f, 1.0f),
            8.0f,
            2.5f
        },
        {
            glm::vec3(-5.5f, 3.0f, -4.0f),
            glm::vec3(0.55f, 1.0f, 0.65f),
            8.0f,
            2.5f
        },
        {
            glm::vec3(5.5f, 3.0f, -4.0f),
            glm::vec3(1.0f, 0.35f, 0.45f),
            8.0f,
            2.5f
        }
    }};
}

namespace chr {

    int Renderer::init(int width, int height) {
        clear();

        if (this->g_buffer_pass.init(width, height) != 0) {
            clear();
            return -1;
        }
        this->width = width;
        this->height = height;
        if (this->shadow_pass.init() != 0) {
            clear();
            return -1;
        }
        if (this->hdr_scene_target.init(width, height) != 0) {
            clear();
            return -1;
        }
        if (this->ssao_pass.init(width, height) != 0) {
            clear();
            return -1;
        }

        this->fullscreen_quad.init();

        if (this->deferred_lighting_pass.init() != 0) {
            clear();
            return -1;
        }

        if (this->tone_mapping_pass.init() != 0) {
            clear();
            return -1;
        }

        if (this->debug_preview_pass.init() != 0) {
            clear();
            return -1;
        }

        if (this->light_marker_pass.init() != 0) {
            clear();
            return -1;
        }
        return 0;
    }

    int Renderer::resize(int width, int height) {
        if (width <= 0 || height <= 0) {
            return -1;
        }

        if (this->width == width && this->height == height) {
            return 0;
        }

        if (this->g_buffer_pass.resize(width, height) != 0) {
            return -1;
        }
        if (this->hdr_scene_target.resize(width, height) != 0) {
            return -1;
        }
        if (this->ssao_pass.resize(width, height) != 0) {
            return -1;
        }

        this->width = width;
        this->height = height;
        return 0;
    }

    void Renderer::clear() {
        this->ssao_pass.clear();
        this->deferred_lighting_pass.clear();
        this->tone_mapping_pass.clear();
        this->debug_preview_pass.clear();
        this->light_marker_pass.clear();
        this->shadow_pass.clear();
        this->fullscreen_quad.clear();
        this->g_buffer_pass.clear();
        this->hdr_scene_target.clear();
        this->debug_environment_texture = 0;
        this->width = 0;
        this->height = 0;
    }

    void Renderer::bind_for_geometry_pass() {
        this->g_buffer_pass.bind_for_geometry_pass();
    }

    glm::mat4 Renderer::get_directional_light_view_projection() const {
        return this->shadow_pass.light_view_projection();
    }

    void Renderer::bind_for_shadow_pass() {
        this->shadow_pass.bind();
    }

    void Renderer::draw_lit_frame(const glm::mat4& mat_projection, const glm::mat4& mat_view) {
        this->ssao_pass.render(
            this->g_buffer_pass.texture_normal,
            this->g_buffer_pass.texture_depth,
            this->fullscreen_quad.vao,
            mat_projection);

        DeferredLightingInputs lighting_inputs{};
        lighting_inputs.framebuffer = this->hdr_scene_target.framebuffer;
        lighting_inputs.texture_albedo = this->g_buffer_pass.texture_albedo;
        lighting_inputs.texture_normal = this->g_buffer_pass.texture_normal;
        lighting_inputs.texture_material = this->g_buffer_pass.texture_material;
        lighting_inputs.texture_emissive = this->g_buffer_pass.texture_emissive;
        lighting_inputs.texture_depth = this->g_buffer_pass.texture_depth;
        lighting_inputs.texture_shadow_depth = this->shadow_pass.depth_texture();
        lighting_inputs.texture_ssao = this->ssao_pass.blurred_texture();
        lighting_inputs.fullscreen_quad_vao = this->fullscreen_quad.vao;
        lighting_inputs.width = this->width;
        lighting_inputs.height = this->height;
        lighting_inputs.mat_projection = mat_projection;
        lighting_inputs.mat_view = mat_view;
        lighting_inputs.mat_light_view_projection = this->shadow_pass.light_view_projection();
        lighting_inputs.point_lights = POINT_LIGHTS;
        this->deferred_lighting_pass.render(lighting_inputs);

        this->tone_mapping_pass.render(
            this->hdr_scene_target.texture_scene_color,
            this->fullscreen_quad.vao,
            this->width,
            this->height);
        glEnable(GL_DEPTH_TEST);
        glBindVertexArray(0);
    }

    void Renderer::draw_light_markers(const glm::mat4& mat_projection, const glm::mat4& mat_view) {
        this->light_marker_pass.render(
            POINT_LIGHTS,
            mat_projection,
            mat_view,
            this->width,
            this->height);
    }

    void Renderer::draw_debug_view(DebugViewMode mode) {
        DebugPreviewTextures preview_textures{};
        preview_textures.albedo = this->g_buffer_pass.texture_albedo;
        preview_textures.normal = this->g_buffer_pass.texture_normal;
        preview_textures.depth = this->g_buffer_pass.texture_depth;
        preview_textures.ssao = this->ssao_pass.blurred_texture();
        preview_textures.material = this->g_buffer_pass.texture_material;
        preview_textures.emissive = this->g_buffer_pass.texture_emissive;
        preview_textures.environment = this->debug_environment_texture;
        this->debug_preview_pass.render(
            preview_textures,
            mode,
            this->fullscreen_quad.vao,
            this->width,
            this->height);
    }

    void Renderer::bind_default_framebuffer(int width, int height) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, width, height);
    }

}

