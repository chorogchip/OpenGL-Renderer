#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D gAlbedo;
uniform sampler2D gNormal;
uniform sampler2D gDepth;
uniform mat4 uInverseProjection;
uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform float uAmbientStrength;
uniform float uDiffuseStrength;
uniform int uPointLightCount;
uniform vec3 uPointLightPositions[5];
uniform vec3 uPointLightColors[5];
uniform float uPointLightIntensities[5];
uniform float uPointLightRanges[5];

vec3 reconstruct_view_position(vec2 uv, float depth) {
    vec4 clip_pos = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 view_pos = uInverseProjection * clip_pos;
    return view_pos.xyz / view_pos.w;
}

void main() {
    vec4 albedo = texture(gAlbedo, TexCoord);
    vec3 normal = normalize(texture(gNormal, TexCoord).rgb);
    float depth = texture(gDepth, TexCoord).r;

    vec3 view_pos = reconstruct_view_position(TexCoord, depth);
    vec3 light_dir = normalize(-uLightDirection);
    float lambert = max(dot(normal, light_dir), 0.0);

    vec3 ambient = uAmbientStrength * uLightColor;
    vec3 diffuse = lambert * uDiffuseStrength * uLightColor;
    vec3 point_light_sum = vec3(0.0);

    for (int i = 0; i < uPointLightCount; ++i) {
        vec3 to_light = uPointLightPositions[i] - view_pos;
        float distance_to_light = length(to_light);
        if (distance_to_light >= uPointLightRanges[i]) {
            continue;
        }

        vec3 point_light_dir = to_light / max(distance_to_light, 0.0001);
        float point_lambert = max(dot(normal, point_light_dir), 0.0);
        float attenuation = 1.0 - (distance_to_light / uPointLightRanges[i]);
        attenuation *= attenuation;
        point_light_sum += point_lambert * attenuation * uPointLightIntensities[i] * uPointLightColors[i];
    }

    vec3 lit_color = (ambient + diffuse + point_light_sum) * albedo.rgb;

    FragColor = vec4(lit_color, albedo.a);
}
