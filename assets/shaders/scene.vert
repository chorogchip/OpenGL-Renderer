#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out vec2 TexCoord;
out vec3 Normal;
out vec3 Tangent;
out vec3 Bitangent;
out vec3 ViewPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    mat4 model_view = view * model;
    mat3 normal_matrix = mat3(transpose(inverse(model_view)));
    vec4 view_pos = model_view * vec4(aPos, 1.0);
    gl_Position = projection * view_pos;
    TexCoord = aTexCoord;
    ViewPos = view_pos.xyz;
    Normal = normal_matrix * aNormal;
    Tangent = normal_matrix * aTangent;
    Bitangent = normal_matrix * aBitangent;
}
