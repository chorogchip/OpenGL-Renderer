#include "deferred_lighting_pass.h"

#include <cstdio>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "graphics_util.h"

namespace {
    constexpr const char* FULLSCREEN_VERTEX_SHADER_PATH = "assets/shaders/fullscreen.vert";
    constexpr const char* LIGHTING_FRAGMENT_SHADER_PATH = "assets/shaders/deferred_light.frag";
    const glm::vec3 LIGHT_DIRECTION = glm::normalize(glm::vec3(-0.6f, -1.0f, -0.35f));
    const glm::vec3 LIGHT_COLOR = glm::vec3(1.0f, 0.98f, 0.92f);
    constexpr float AMBIENT_STRENGTH = 0.20f;
    constexpr float DIFFUSE_STRENGTH = 2.5f;

    static void find_uniforms(chr::DeferredLightingPass* pass) {
        pass->uniform_g_albedo = glGetUniformLocation(pass->shader_program, "gAlbedo");
        pass->uniform_g_normal = glGetUniformLocation(pass->shader_program, "gNormal");
        pass->uniform_g_material = glGetUniformLocation(pass->shader_program, "gMaterial");
        pass->uniform_g_emissive = glGetUniformLocation(pass->shader_program, "gEmissive");
        pass->uniform_irradiance_map = glGetUniformLocation(pass->shader_program, "uIrradianceMap");
        pass->uniform_has_irradiance_map = glGetUniformLocation(pass->shader_program, "uHasIrradianceMap");
        pass->uniform_prefilter_map = glGetUniformLocation(pass->shader_program, "uPrefilterMap");
        pass->uniform_brdf_lut = glGetUniformLocation(pass->shader_program, "uBrdfLut");
        pass->uniform_has_specular_ibl = glGetUniformLocation(pass->shader_program, "uHasSpecularIbl");
        pass->uniform_g_depth = glGetUniformLocation(pass->shader_program, "gDepth");
        pass->uniform_shadow_map = glGetUniformLocation(pass->shader_program, "uShadowMap");
        pass->uniform_ssao_map = glGetUniformLocation(pass->shader_program, "uSsaoMap");
        pass->uniform_inverse_projection = glGetUniformLocation(pass->shader_program, "uInverseProjection");
        pass->uniform_inverse_view = glGetUniformLocation(pass->shader_program, "uInverseView");
        pass->uniform_light_view_projection = glGetUniformLocation(pass->shader_program, "uLightViewProjection");
        pass->uniform_light_direction = glGetUniformLocation(pass->shader_program, "uLightDirection");
        pass->uniform_light_color = glGetUniformLocation(pass->shader_program, "uLightColor");
        pass->uniform_ambient_strength = glGetUniformLocation(pass->shader_program, "uAmbientStrength");
        pass->uniform_diffuse_strength = glGetUniformLocation(pass->shader_program, "uDiffuseStrength");
        pass->uniform_point_light_count = glGetUniformLocation(pass->shader_program, "uPointLightCount");

        for (int i = 0; i < static_cast<int>(pass->uniform_point_light_positions.size()); ++i) {
            char uniform_name[64];
            snprintf(uniform_name, sizeof(uniform_name), "uPointLightPositions[%d]", i);
            pass->uniform_point_light_positions[i] = glGetUniformLocation(pass->shader_program, uniform_name);
            snprintf(uniform_name, sizeof(uniform_name), "uPointLightColors[%d]", i);
            pass->uniform_point_light_colors[i] = glGetUniformLocation(pass->shader_program, uniform_name);
            snprintf(uniform_name, sizeof(uniform_name), "uPointLightIntensities[%d]", i);
            pass->uniform_point_light_intensities[i] = glGetUniformLocation(pass->shader_program, uniform_name);
            snprintf(uniform_name, sizeof(uniform_name), "uPointLightRanges[%d]", i);
            pass->uniform_point_light_ranges[i] = glGetUniformLocation(pass->shader_program, uniform_name);
        }
    }
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

        find_uniforms(this);

        return 0;
    }

    int DeferredLightingPass::reload_shaders() {
        const uint32_t new_shader_program = graphics_util::create_shader_program_from_files(
            FULLSCREEN_VERTEX_SHADER_PATH,
            LIGHTING_FRAGMENT_SHADER_PATH);
        if (new_shader_program == 0) {
            return -1;
        }

        const uint32_t old_shader_program = shader_program;
        shader_program = new_shader_program;
        find_uniforms(this);
        if (old_shader_program != 0) {
            glDeleteProgram(old_shader_program);
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
        uniform_prefilter_map = -1;
        uniform_brdf_lut = -1;
        uniform_has_specular_ibl = -1;
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
        glUniform1i(uniform_prefilter_map, 8);
        glUniform1i(uniform_brdf_lut, 9);
        glUniform1i(uniform_has_irradiance_map, inputs.texture_irradiance != 0 ? 1 : 0);
        glUniform1i(
            uniform_has_specular_ibl,
            inputs.texture_prefiltered_environment != 0 && inputs.texture_brdf_lut != 0 ? 1 : 0);
        glUniformMatrix4fv(uniform_inverse_projection, 1, GL_FALSE, &inverse_projection[0][0]);
        glUniformMatrix4fv(uniform_inverse_view, 1, GL_FALSE, &inverse_view[0][0]);
        glUniformMatrix4fv(uniform_light_view_projection, 1, GL_FALSE, &inputs.mat_light_view_projection[0][0]);
        glUniform3fv(uniform_light_direction, 1, &view_light_direction[0]);
        glUniform3fv(uniform_light_color, 1, &inputs.directional_light_color[0]);
        glUniform1f(uniform_ambient_strength, inputs.ambient_intensity);
        glUniform1f(uniform_diffuse_strength, inputs.directional_light_intensity);
        glUniform1i(uniform_point_light_count, static_cast<int>(inputs.point_lights.size()));

        for (int i = 0; i < static_cast<int>(inputs.point_lights.size()); ++i) {
            const glm::vec3 view_light_position =
                glm::vec3(inputs.mat_view * glm::vec4(inputs.point_lights[i].position, 1.0f));
            glUniform3fv(uniform_point_light_positions[i], 1, &view_light_position[0]);
            glUniform3fv(uniform_point_light_colors[i], 1, &inputs.point_lights[i].color[0]);
            glUniform1f(uniform_point_light_intensities[i], inputs.point_light_intensities[i]);
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
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_CUBE_MAP, inputs.texture_prefiltered_environment);
        glActiveTexture(GL_TEXTURE9);
        glBindTexture(GL_TEXTURE_2D, inputs.texture_brdf_lut);

        glBindVertexArray(inputs.fullscreen_quad_vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

}
