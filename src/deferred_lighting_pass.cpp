#include "deferred_lighting_pass.h"

#include <cstdio>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "graphics_util.h"

namespace {
    constexpr const char* FULLSCREEN_VERTEX_SHADER_PATH = "assets/shaders/deferred_light.vert";
    constexpr const char* LIGHTING_FRAGMENT_SHADER_PATH = "assets/shaders/deferred_light.frag";
    const glm::vec3 LIGHT_DIRECTION = glm::normalize(glm::vec3(-0.6f, -1.0f, -0.35f));
    const glm::vec3 LIGHT_COLOR = glm::vec3(1.0f, 0.98f, 0.92f);
    constexpr float AMBIENT_STRENGTH = 0.32f;
    constexpr float DIFFUSE_STRENGTH = 0.85f;
}

namespace chr {

    int DeferredLightingPass::init() {
        clear();

        shader_program = graphics_util::create_shader_program_from_files(
            FULLSCREEN_VERTEX_SHADER_PATH,
            LIGHTING_FRAGMENT_SHADER_PATH);
        if (shader_program == 0) {
            return -1;
        }

        uniform_g_albedo = glGetUniformLocation(shader_program, "gAlbedo");
        uniform_g_normal = glGetUniformLocation(shader_program, "gNormal");
        uniform_g_material = glGetUniformLocation(shader_program, "gMaterial");
        uniform_g_emissive = glGetUniformLocation(shader_program, "gEmissive");
        uniform_irradiance_map = glGetUniformLocation(shader_program, "uIrradianceMap");
        uniform_has_irradiance_map = glGetUniformLocation(shader_program, "uHasIrradianceMap");
        uniform_g_depth = glGetUniformLocation(shader_program, "gDepth");
        uniform_shadow_map = glGetUniformLocation(shader_program, "uShadowMap");
        uniform_ssao_map = glGetUniformLocation(shader_program, "uSsaoMap");
        uniform_inverse_projection = glGetUniformLocation(shader_program, "uInverseProjection");
        uniform_inverse_view = glGetUniformLocation(shader_program, "uInverseView");
        uniform_light_view_projection = glGetUniformLocation(shader_program, "uLightViewProjection");
        uniform_light_direction = glGetUniformLocation(shader_program, "uLightDirection");
        uniform_light_color = glGetUniformLocation(shader_program, "uLightColor");
        uniform_ambient_strength = glGetUniformLocation(shader_program, "uAmbientStrength");
        uniform_diffuse_strength = glGetUniformLocation(shader_program, "uDiffuseStrength");
        uniform_point_light_count = glGetUniformLocation(shader_program, "uPointLightCount");

        for (int i = 0; i < static_cast<int>(uniform_point_light_positions.size()); ++i) {
            char uniform_name[64];
            snprintf(uniform_name, sizeof(uniform_name), "uPointLightPositions[%d]", i);
            uniform_point_light_positions[i] = glGetUniformLocation(shader_program, uniform_name);
            snprintf(uniform_name, sizeof(uniform_name), "uPointLightColors[%d]", i);
            uniform_point_light_colors[i] = glGetUniformLocation(shader_program, uniform_name);
            snprintf(uniform_name, sizeof(uniform_name), "uPointLightIntensities[%d]", i);
            uniform_point_light_intensities[i] = glGetUniformLocation(shader_program, uniform_name);
            snprintf(uniform_name, sizeof(uniform_name), "uPointLightRanges[%d]", i);
            uniform_point_light_ranges[i] = glGetUniformLocation(shader_program, uniform_name);
        }

        return 0;
    }

    void DeferredLightingPass::clear() {
        if (shader_program != 0) {
            glDeleteProgram(shader_program);
            shader_program = 0;
        }

        uniform_g_albedo = -1;
        uniform_g_normal = -1;
        uniform_g_material = -1;
        uniform_g_emissive = -1;
        uniform_irradiance_map = -1;
        uniform_has_irradiance_map = -1;
        uniform_g_depth = -1;
        uniform_shadow_map = -1;
        uniform_ssao_map = -1;
        uniform_inverse_projection = -1;
        uniform_inverse_view = -1;
        uniform_light_view_projection = -1;
        uniform_light_direction = -1;
        uniform_light_color = -1;
        uniform_ambient_strength = -1;
        uniform_diffuse_strength = -1;
        uniform_point_light_count = -1;
        uniform_point_light_positions.fill(-1);
        uniform_point_light_colors.fill(-1);
        uniform_point_light_intensities.fill(-1);
        uniform_point_light_ranges.fill(-1);
    }

    void DeferredLightingPass::render(const DeferredLightingInputs& inputs) {
        glBindFramebuffer(GL_FRAMEBUFFER, inputs.framebuffer);
        glViewport(0, 0, inputs.width, inputs.height);
        if (inputs.clear_color) {
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        }

        const glm::mat4 inverse_projection = glm::inverse(inputs.mat_projection);
        const glm::mat4 inverse_view = glm::inverse(inputs.mat_view);
        const glm::vec3 view_light_direction =
            glm::normalize(glm::mat3(inputs.mat_view) * LIGHT_DIRECTION);

        glUseProgram(shader_program);
        glUniform1i(uniform_g_albedo, 0);
        glUniform1i(uniform_g_normal, 1);
        glUniform1i(uniform_g_material, 2);
        glUniform1i(uniform_g_depth, 3);
        glUniform1i(uniform_shadow_map, 4);
        glUniform1i(uniform_ssao_map, 5);
        glUniform1i(uniform_g_emissive, 6);
        glUniform1i(uniform_irradiance_map, 7);
        glUniform1i(uniform_has_irradiance_map, inputs.texture_irradiance != 0 ? 1 : 0);
        glUniformMatrix4fv(uniform_inverse_projection, 1, GL_FALSE, &inverse_projection[0][0]);
        glUniformMatrix4fv(uniform_inverse_view, 1, GL_FALSE, &inverse_view[0][0]);
        glUniformMatrix4fv(uniform_light_view_projection, 1, GL_FALSE, &inputs.mat_light_view_projection[0][0]);
        glUniform3fv(uniform_light_direction, 1, &view_light_direction[0]);
        glUniform3fv(uniform_light_color, 1, &LIGHT_COLOR[0]);
        glUniform1f(uniform_ambient_strength, AMBIENT_STRENGTH);
        glUniform1f(uniform_diffuse_strength, DIFFUSE_STRENGTH);
        glUniform1i(uniform_point_light_count, static_cast<int>(inputs.point_lights.size()));

        for (int i = 0; i < static_cast<int>(inputs.point_lights.size()); ++i) {
            const glm::vec3 view_light_position =
                glm::vec3(inputs.mat_view * glm::vec4(inputs.point_lights[i].position, 1.0f));
            glUniform3fv(uniform_point_light_positions[i], 1, &view_light_position[0]);
            glUniform3fv(uniform_point_light_colors[i], 1, &inputs.point_lights[i].color[0]);
            glUniform1f(uniform_point_light_intensities[i], inputs.point_lights[i].intensity);
            glUniform1f(uniform_point_light_ranges[i], inputs.point_lights[i].range);
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inputs.texture_albedo);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, inputs.texture_normal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, inputs.texture_material);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, inputs.texture_depth);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, inputs.texture_shadow_depth);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, inputs.texture_ssao);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, inputs.texture_emissive);
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_CUBE_MAP, inputs.texture_irradiance);

        glBindVertexArray(inputs.fullscreen_quad_vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

}
