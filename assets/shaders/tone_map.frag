#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D uSceneColor;
uniform float uExposure;

vec3 aces_filmic(vec3 color) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

void main() {
    vec3 hdr_color = texture(uSceneColor, TexCoord).rgb;
    vec3 exposed = hdr_color * uExposure;
    vec3 mapped = aces_filmic(exposed);
    FragColor = vec4(mapped, 1.0);
}
