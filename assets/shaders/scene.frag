#version 330 core

layout (location = 0) out vec4 GAlbedo;
layout (location = 1) out vec3 GNormal;
layout (location = 2) out vec4 GMaterial;
layout (location = 3) out vec3 GEmissive;

in vec2 TexCoord;
in vec3 Normal;
in vec3 Tangent;

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

void main() {
    float alpha_mask = texture(uAlphaMaskTexture, TexCoord).r;
    if (alpha_mask < uAlphaCutoff) {
        discard;
    }

    vec4 albedo = texture(uTexture, TexCoord) * uBaseColorFactor;
    vec3 base_normal = normalize(Normal);
    vec3 tangent = Tangent;
    tangent = tangent - dot(tangent, base_normal) * base_normal;
    float tangent_length = length(tangent);

    vec3 final_normal = base_normal;
    if (tangent_length > 0.0001) {
        tangent /= tangent_length;
        vec3 bitangent = normalize(cross(base_normal, tangent));
        vec3 sampled_normal = texture(uNormalTexture, TexCoord).rgb * 2.0 - 1.0;
        sampled_normal.xy *= uNormalScale;
        mat3 tbn = mat3(tangent, bitangent, base_normal);
        final_normal = normalize(tbn * sampled_normal);
    }

    vec4 metallic_roughness = texture(uMetallicRoughnessTexture, TexCoord);
    float roughness = clamp(metallic_roughness.g * uRoughnessFactor, 0.04, 1.0);
    float metallic = clamp(metallic_roughness.b * uMetallicFactor, 0.0, 1.0);
    float occlusion = mix(1.0, texture(uOcclusionTexture, TexCoord).r, uOcclusionStrength);
    vec3 emissive = texture(uEmissiveTexture, TexCoord).rgb * uEmissiveFactor;

    GAlbedo = albedo;
    GNormal = final_normal;
    GMaterial = vec4(metallic, roughness, occlusion, albedo.a);
    GEmissive = emissive;
}
