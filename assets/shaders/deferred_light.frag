#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D gAlbedo;
uniform sampler2D gNormal;
uniform sampler2D gMaterial;
uniform sampler2D gEmissive;
uniform sampler2D gDepth;
uniform sampler2D uShadowMap;
uniform sampler2D uSsaoMap;
uniform samplerCube uIrradianceMap;
uniform mat4 uInverseProjection;
uniform mat4 uInverseView;
uniform mat4 uLightViewProjection;
uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform float uAmbientStrength;
uniform float uDiffuseStrength;
uniform int uHasIrradianceMap;
uniform int uPointLightCount;
uniform vec3 uPointLightPositions[5];
uniform vec3 uPointLightColors[5];
uniform float uPointLightIntensities[5];
uniform float uPointLightRanges[5];

const float PI = 3.14159265359;

vec3 reconstruct_view_position(vec2 uv, float depth) {
    vec4 clip_pos = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 view_pos = uInverseProjection * clip_pos;
    return view_pos.xyz / view_pos.w;
}

float calculate_directional_shadow(vec3 view_pos, vec3 normal) {
    vec3 world_pos = (uInverseView * vec4(view_pos, 1.0)).xyz;
    vec4 light_clip = uLightViewProjection * vec4(world_pos, 1.0);
    vec3 light_ndc = light_clip.xyz / light_clip.w;
    vec3 shadow_uvz = light_ndc * 0.5 + 0.5;

    if (shadow_uvz.z < 0.0 || shadow_uvz.z > 1.0) {
        return 0.0;
    }
    if (shadow_uvz.x < 0.0 || shadow_uvz.x > 1.0 || shadow_uvz.y < 0.0 || shadow_uvz.y > 1.0) {
        return 0.0;
    }

    vec3 view_light_dir = normalize(-uLightDirection);
    float bias = max(0.0015 * (1.0 - dot(normal, view_light_dir)), 0.0005);
    vec2 texel_size = 1.0 / vec2(textureSize(uShadowMap, 0));
    float shadow = 0.0;

    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            float closest_depth = texture(uShadowMap, shadow_uvz.xy + vec2(x, y) * texel_size).r;
            shadow += shadow_uvz.z - bias > closest_depth ? 1.0 : 0.0;
        }
    }

    return shadow / 9.0;
}

float distribution_ggx(vec3 normal, vec3 half_vector, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float n_dot_h = max(dot(normal, half_vector), 0.0);
    float n_dot_h2 = n_dot_h * n_dot_h;

    float denominator = (n_dot_h2 * (a2 - 1.0) + 1.0);
    denominator = PI * denominator * denominator;
    return a2 / max(denominator, 0.0001);
}

float geometry_schlick_ggx(float n_dot_v, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return n_dot_v / max(n_dot_v * (1.0 - k) + k, 0.0001);
}

float geometry_smith(vec3 normal, vec3 view_dir, vec3 light_dir, float roughness) {
    float n_dot_v = max(dot(normal, view_dir), 0.0);
    float n_dot_l = max(dot(normal, light_dir), 0.0);
    return geometry_schlick_ggx(n_dot_v, roughness) *
        geometry_schlick_ggx(n_dot_l, roughness);
}

vec3 fresnel_schlick(float cos_theta, vec3 f0) {
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

vec3 calculate_pbr_light(
    vec3 albedo,
    vec3 normal,
    vec3 view_dir,
    vec3 light_dir,
    vec3 radiance,
    float metallic,
    float roughness) {
    vec3 half_vector = normalize(view_dir + light_dir);
    float n_dot_l = max(dot(normal, light_dir), 0.0);
    float n_dot_v = max(dot(normal, view_dir), 0.0);
    if (n_dot_l <= 0.0 || n_dot_v <= 0.0) {
        return vec3(0.0);
    }

    vec3 f0 = mix(vec3(0.04), albedo, metallic);
    float ndf = distribution_ggx(normal, half_vector, roughness);
    float geometry = geometry_smith(normal, view_dir, light_dir, roughness);
    vec3 fresnel = fresnel_schlick(max(dot(half_vector, view_dir), 0.0), f0);

    vec3 numerator = ndf * geometry * fresnel;
    float denominator = max(4.0 * n_dot_v * n_dot_l, 0.0001);
    vec3 specular = numerator / denominator;

    vec3 k_s = fresnel;
    vec3 k_d = (vec3(1.0) - k_s) * (1.0 - metallic);
    return (k_d * albedo / PI + specular) * radiance * n_dot_l;
}

void main() {
    vec4 albedo = texture(gAlbedo, TexCoord);
    vec3 normal = normalize(texture(gNormal, TexCoord).rgb);
    vec4 material = texture(gMaterial, TexCoord);
    vec3 emissive = texture(gEmissive, TexCoord).rgb;
    float metallic = clamp(material.r, 0.0, 1.0);
    float roughness = clamp(material.g, 0.04, 1.0);
    float depth = texture(gDepth, TexCoord).r;
    if (depth >= 0.999999) {
        discard;
    }
    float ambient_occlusion = texture(uSsaoMap, TexCoord).r * material.b;

    vec3 view_pos = reconstruct_view_position(TexCoord, depth);
    vec3 view_dir = normalize(-view_pos);
    vec3 light_dir = normalize(-uLightDirection);
    float directional_shadow = calculate_directional_shadow(view_pos, normal);

    vec3 ambient = ambient_occlusion * uAmbientStrength * albedo.rgb;
    if (uHasIrradianceMap != 0) {
        vec3 world_normal = normalize(mat3(uInverseView) * normal);
        vec3 irradiance = texture(uIrradianceMap, world_normal).rgb;
        ambient += ambient_occlusion * irradiance * albedo.rgb * (1.0 - metallic);
    }
    vec3 directional = (1.0 - directional_shadow) * calculate_pbr_light(
        albedo.rgb,
        normal,
        view_dir,
        light_dir,
        uDiffuseStrength * uLightColor,
        metallic,
        roughness);
    vec3 point_light_sum = vec3(0.0);

    for (int i = 0; i < uPointLightCount; ++i) {
        vec3 to_light = uPointLightPositions[i] - view_pos;
        float distance_to_light = length(to_light);
        if (distance_to_light >= uPointLightRanges[i]) {
            continue;
        }

        vec3 point_light_dir = to_light / max(distance_to_light, 0.0001);
        float attenuation = 1.0 - (distance_to_light / uPointLightRanges[i]);
        attenuation *= attenuation;
        vec3 radiance = attenuation * uPointLightIntensities[i] * uPointLightColors[i];
        point_light_sum += calculate_pbr_light(
            albedo.rgb,
            normal,
            view_dir,
            point_light_dir,
            radiance,
            metallic,
            roughness);
    }

    vec3 lit_color = ambient + directional + point_light_sum + emissive;

    FragColor = vec4(lit_color, albedo.a);
}
