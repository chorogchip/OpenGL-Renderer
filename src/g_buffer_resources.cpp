#include "g_buffer_resources.h"

#include <array>
#include <cstdio>
#include <iostream>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "graphics_util.h"

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

    static int create_g_buffer_attachments(chr::GBufferResources* resources, int width, int height) {
        glGenFramebuffers(1, &resources->framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, resources->framebuffer);

        glGenTextures(1, &resources->texture_albedo);
        glBindTexture(GL_TEXTURE_2D, resources->texture_albedo);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA8,
            width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resources->texture_albedo, 0);

        glGenTextures(1, &resources->texture_normal);
        glBindTexture(GL_TEXTURE_2D, resources->texture_normal);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGB16F,
            width, height, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, resources->texture_normal, 0);

        glGenTextures(1, &resources->texture_material);
        glBindTexture(GL_TEXTURE_2D, resources->texture_material);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA8,
            width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, resources->texture_material, 0);

        glGenTextures(1, &resources->texture_depth);
        glBindTexture(GL_TEXTURE_2D, resources->texture_depth);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
            width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, resources->texture_depth, 0);

        constexpr GLenum draw_buffers[] = {
            GL_COLOR_ATTACHMENT0,
            GL_COLOR_ATTACHMENT1,
            GL_COLOR_ATTACHMENT2
        };
        glDrawBuffers(3, draw_buffers);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cout << "Err: G-buffer framebuffer is incomplete." << std::endl;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return -1;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        resources->width = width;
        resources->height = height;
        return 0;
    }

    static int create_hdr_attachments(chr::GBufferResources* resources, int width, int height) {
        glGenFramebuffers(1, &resources->hdr_framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, resources->hdr_framebuffer);

        glGenTextures(1, &resources->texture_scene_color);
        glBindTexture(GL_TEXTURE_2D, resources->texture_scene_color);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resources->texture_scene_color, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cout << "Err: HDR framebuffer is incomplete." << std::endl;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return -1;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return 0;
    }

    static void clear_g_buffer_attachments(chr::GBufferResources* resources) {
        if (resources->texture_depth != 0) {
            glDeleteTextures(1, &resources->texture_depth);
            resources->texture_depth = 0;
        }

        if (resources->texture_normal != 0) {
            glDeleteTextures(1, &resources->texture_normal);
            resources->texture_normal = 0;
        }

        if (resources->texture_material != 0) {
            glDeleteTextures(1, &resources->texture_material);
            resources->texture_material = 0;
        }

        if (resources->texture_albedo != 0) {
            glDeleteTextures(1, &resources->texture_albedo);
            resources->texture_albedo = 0;
        }

        if (resources->framebuffer != 0) {
            glDeleteFramebuffers(1, &resources->framebuffer);
            resources->framebuffer = 0;
        }

        resources->width = 0;
        resources->height = 0;
    }

    static void clear_hdr_attachments(chr::GBufferResources* resources) {
        if (resources->texture_scene_color != 0) {
            glDeleteTextures(1, &resources->texture_scene_color);
            resources->texture_scene_color = 0;
        }
        if (resources->hdr_framebuffer != 0) {
            glDeleteFramebuffers(1, &resources->hdr_framebuffer);
            resources->hdr_framebuffer = 0;
        }
    }

}

namespace chr {

    int GBufferResources::init(int width, int height) {
        clear();

        if (width <= 0 || height <= 0) {
            std::cout << "Err: Invalid G-buffer size." << std::endl;
            return -1;
        }

        if (create_g_buffer_attachments(this, width, height) != 0) {
            clear();
            return -1;
        }
        if (this->shadow_pass.init() != 0) {
            clear();
            return -1;
        }
        if (create_hdr_attachments(this, width, height) != 0) {
            clear();
            return -1;
        }
        if (this->ssao_pass.init(width, height) != 0) {
            clear();
            return -1;
        }

        constexpr float quad_vertices[] = {
            -1.0f, -1.0f, 0.0f, 0.0f,
             1.0f, -1.0f, 1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f, 1.0f,
            -1.0f,  1.0f, 0.0f, 1.0f,
             1.0f, -1.0f, 1.0f, 0.0f,
             1.0f,  1.0f, 1.0f, 1.0f
        };

        glGenVertexArrays(1, &this->quad_vao);
        glGenBuffers(1, &this->quad_vbo);
        glBindVertexArray(this->quad_vao);
        glBindBuffer(GL_ARRAY_BUFFER, this->quad_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glBindVertexArray(0);

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

    int GBufferResources::resize(int width, int height) {
        if (width <= 0 || height <= 0) {
            return -1;
        }

        if (this->width == width && this->height == height) {
            return 0;
        }

        clear_g_buffer_attachments(this);
        if (create_g_buffer_attachments(this, width, height) != 0) {
            clear_g_buffer_attachments(this);
            return -1;
        }
        clear_hdr_attachments(this);
        if (create_hdr_attachments(this, width, height) != 0) {
            clear_hdr_attachments(this);
            return -1;
        }
        if (this->ssao_pass.resize(width, height) != 0) {
            return -1;
        }

        return 0;
    }

    void GBufferResources::clear() {
        this->ssao_pass.clear();
        this->deferred_lighting_pass.clear();
        this->tone_mapping_pass.clear();
        this->debug_preview_pass.clear();
        this->light_marker_pass.clear();
        this->shadow_pass.clear();

        if (this->quad_vbo != 0) {
            glDeleteBuffers(1, &this->quad_vbo);
            this->quad_vbo = 0;
        }

        if (this->quad_vao != 0) {
            glDeleteVertexArrays(1, &this->quad_vao);
            this->quad_vao = 0;
        }

        clear_g_buffer_attachments(this);
        clear_hdr_attachments(this);
    }

    void GBufferResources::bind_for_geometry_pass() {
        glBindFramebuffer(GL_FRAMEBUFFER, this->framebuffer);
        glViewport(0, 0, this->width, this->height);
    }

    glm::mat4 GBufferResources::get_directional_light_view_projection() const {
        return this->shadow_pass.light_view_projection();
    }

    void GBufferResources::bind_for_shadow_pass() {
        this->shadow_pass.bind();
    }

    void GBufferResources::draw_lighting_pass(const glm::mat4& mat_projection, const glm::mat4& mat_view) {
        this->ssao_pass.render(this->texture_normal, this->texture_depth, this->quad_vao, mat_projection);

        DeferredLightingInputs lighting_inputs{};
        lighting_inputs.framebuffer = this->hdr_framebuffer;
        lighting_inputs.texture_albedo = this->texture_albedo;
        lighting_inputs.texture_normal = this->texture_normal;
        lighting_inputs.texture_material = this->texture_material;
        lighting_inputs.texture_depth = this->texture_depth;
        lighting_inputs.texture_shadow_depth = this->shadow_pass.depth_texture();
        lighting_inputs.texture_ssao = this->ssao_pass.blurred_texture();
        lighting_inputs.fullscreen_quad_vao = this->quad_vao;
        lighting_inputs.width = this->width;
        lighting_inputs.height = this->height;
        lighting_inputs.mat_projection = mat_projection;
        lighting_inputs.mat_view = mat_view;
        lighting_inputs.mat_light_view_projection = this->shadow_pass.light_view_projection();
        lighting_inputs.point_lights = POINT_LIGHTS;
        this->deferred_lighting_pass.render(lighting_inputs);

        this->tone_mapping_pass.render(
            this->texture_scene_color,
            this->quad_vao,
            this->width,
            this->height);
        glEnable(GL_DEPTH_TEST);
        glBindVertexArray(0);
    }

    void GBufferResources::draw_light_markers(const glm::mat4& mat_projection, const glm::mat4& mat_view) {
        this->light_marker_pass.render(
            POINT_LIGHTS,
            mat_projection,
            mat_view,
            this->width,
            this->height);
    }

    void GBufferResources::draw_debug_views() {
        DebugPreviewTextures preview_textures{};
        preview_textures.albedo = this->texture_albedo;
        preview_textures.normal = this->texture_normal;
        preview_textures.depth = this->texture_depth;
        preview_textures.ssao = this->ssao_pass.blurred_texture();
        preview_textures.material = this->texture_material;
        this->debug_preview_pass.render(
            preview_textures,
            this->quad_vao,
            this->width,
            this->height);
    }

    void GBufferResources::bind_default_framebuffer(int width, int height) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, width, height);
    }

}
