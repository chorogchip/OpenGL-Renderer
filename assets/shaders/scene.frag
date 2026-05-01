#version 330 core

out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D uTexture;
uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform float uAmbientStrength;
uniform float uDiffuseStrength;

void main() {
    vec4 albedo = texture(uTexture, TexCoord);
    vec3 normal = normalize(Normal);
    vec3 light_dir = normalize(-uLightDirection);

    float lambert = max(dot(normal, light_dir), 0.0);
    vec3 ambient = uAmbientStrength * uLightColor;
    vec3 diffuse = lambert * uDiffuseStrength * uLightColor;
    vec3 lit_color = (ambient + diffuse) * albedo.rgb;

    FragColor = vec4(lit_color, albedo.a);
}
