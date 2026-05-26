#version 330 core

out vec4 FragColor;

in vec3 LocalPos;

uniform samplerCube uEnvironmentMap;

void main() {
    vec3 environment_color = texture(uEnvironmentMap, normalize(LocalPos)).rgb;
    FragColor = vec4(environment_color, 1.0);
}
