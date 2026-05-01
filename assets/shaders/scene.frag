#version 330 core

layout (location = 0) out vec4 GAlbedo;
layout (location = 1) out vec3 GNormal;

in vec2 TexCoord;
in vec3 Normal;

uniform sampler2D uTexture;

void main() {
    vec4 albedo = texture(uTexture, TexCoord);
    GAlbedo = albedo;
    GNormal = normalize(Normal);
}
