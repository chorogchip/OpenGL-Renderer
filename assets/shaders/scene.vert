#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vec4 world_pos = model * vec4(aPos, 1.0);
    gl_Position = projection * view * world_pos;
    TexCoord = aTexCoord;
    FragPos = world_pos.xyz;
    Normal = mat3(transpose(inverse(model))) * aNormal;
}
