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
    vec3 lit_color = (ambient + diffuse) * albedo.rgb;

    FragColor = vec4(lit_color, albedo.a);
}
