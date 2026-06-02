#version 330 core

layout (location = 0) out vec4 GAlbedo;
layout (location = 1) out vec3 GNormal;
layout (location = 2) out vec4 GMaterial;
layout (location = 3) out vec3 GEmissive;

in vec2 TexCoord;
in vec3 Normal;
in vec3 Tangent;
in vec3 Bitangent;
in vec3 ViewPos;

uniform sampler2D uTexture;
uniform sampler2D uNormalTexture;
uniform sampler2D uAlphaMaskTexture;
uniform sampler2D uMetallicRoughnessTexture;
uniform sampler2D uOcclusionTexture;
uniform sampler2D uEmissiveTexture;
uniform vec4 uBaseColorFactor;
uniform float uMetallicFactor;
uniform float uRoughnessFactor;
uniform vec3 uEmissiveFactor;
uniform float uAlphaCutoff;
uniform float uNormalScale;
uniform float uOcclusionStrength;
uniform float uHeightScale;

const float METALLIC_THRESHOLD = 0.5;
const float POM_LAYERS = 32.0;

vec2 parallax_occlusion_mapping(vec2 tex_coord, vec3 view_dir, vec3 tangent, vec3 bitangent, vec3 normal) {
    vec3 view_dir_ts = normalize(vec3(
        dot(view_dir, tangent),
        dot(view_dir, bitangent),
        dot(view_dir, normal)
    ));

    float layer_depth = 1.0 / POM_LAYERS;
    float current_layer_depth = 0.0;
    vec2 p = view_dir_ts.xy / -view_dir_ts.z * uHeightScale;
    vec2 delta_tc = p / POM_LAYERS;

    vec2 current_tc = tex_coord;
    float surface_height = texture(uNormalTexture, current_tc).r;

    while (current_layer_depth < surface_height) {
        current_tc -= delta_tc;
        surface_height = texture(uNormalTexture, current_tc).r;
        current_layer_depth += layer_depth;
    }

    vec2 prev_tc = current_tc + delta_tc;
    float next_depth = surface_height - current_layer_depth;
    float prev_depth = texture(uNormalTexture, prev_tc).r - (current_layer_depth - layer_depth);

    float weight = next_depth / (next_depth - prev_depth);
    return mix(current_tc, prev_tc, weight);
}

void main() {
    vec3 base_normal = normalize(Normal);
    vec3 tangent = Tangent;
    tangent = tangent - dot(tangent, base_normal) * base_normal;
    float tangent_length = length(tangent);

    vec2 final_tex_coord = TexCoord;
    if (tangent_length > 0.0001 && uHeightScale > 0.001) {
        tangent /= tangent_length;
        vec3 bitangent = Bitangent;
        bitangent = bitangent - dot(bitangent, base_normal) * base_normal;
        bitangent = bitangent - dot(bitangent, tangent) * tangent;
        float bitangent_length = length(bitangent);
        if (bitangent_length > 0.0001) {
            bitangent /= bitangent_length;
        } else {
            bitangent = normalize(cross(base_normal, tangent));
        }
        vec3 view_dir = normalize(-ViewPos);
        final_tex_coord = parallax_occlusion_mapping(TexCoord, view_dir, tangent, bitangent, base_normal);
    }

    float alpha_mask = texture(uAlphaMaskTexture, final_tex_coord).r;
    if (alpha_mask < uAlphaCutoff) {
        discard;
    }

    vec4 albedo = texture(uTexture, final_tex_coord) * uBaseColorFactor;
    vec3 final_normal = base_normal;
    if (tangent_length > 0.0001) {
        tangent /= tangent_length;
        vec3 bitangent = Bitangent;
        bitangent = bitangent - dot(bitangent, base_normal) * base_normal;
        bitangent = bitangent - dot(bitangent, tangent) * tangent;
        float bitangent_length = length(bitangent);
        if (bitangent_length > 0.0001) {
            bitangent /= bitangent_length;
        }
        else {
            bitangent = normalize(cross(base_normal, tangent));
        }
        vec3 sampled_normal = texture(uNormalTexture, final_tex_coord).rgb * 2.0 - 1.0;
        sampled_normal.xy *= uNormalScale;
        mat3 tbn = mat3(tangent, bitangent, base_normal);
        final_normal = normalize(tbn * sampled_normal);
    }

    vec4 metallic_roughness = texture(uMetallicRoughnessTexture, final_tex_coord);
    float roughness = clamp(metallic_roughness.g * uRoughnessFactor, 0.04, 1.0);
    float metallic = clamp(metallic_roughness.b * uMetallicFactor, 0.0, 1.0);
    metallic = metallic < METALLIC_THRESHOLD ? 0.0 : metallic;
    float occlusion = mix(1.0, texture(uOcclusionTexture, final_tex_coord).r, uOcclusionStrength);
    vec3 emissive = texture(uEmissiveTexture, final_tex_coord).rgb * uEmissiveFactor;

    GAlbedo = albedo;
    GNormal = final_normal;
    GMaterial = vec4(metallic, roughness, occlusion, albedo.a);
    GEmissive = emissive;
}
