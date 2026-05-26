#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D uTexture;
uniform int uMode;

void main() {
    vec4 sample_value = texture(uTexture, TexCoord);

    if (uMode == 1) {
        FragColor = vec4(sample_value.rgb, 1.0);
        return;
    }

    if (uMode == 2) {
        vec3 normal = normalize(sample_value.rgb);
        FragColor = vec4(normal * 0.5 + 0.5, 1.0);
        return;
    }

    if (uMode == 3) {
        if (uMode == 7) {
        FragColor = vec4(sample_value.rgb, 1.0);
        return;
    }

    float depth = sample_value.r;
        float visual_depth = 1.0 - depth;
        FragColor = vec4(vec3(visual_depth), 1.0);
        return;
    }

    if (uMode == 4) {
        FragColor = vec4(vec3(sample_value.r), 1.0);
        return;
    }

    if (uMode == 5) {
        FragColor = vec4(vec3(sample_value.r), 1.0);
        return;
    }

    if (uMode == 6) {
        FragColor = vec4(vec3(sample_value.g), 1.0);
        return;
    }

    if (uMode == 7) {
        FragColor = vec4(vec3(sample_value.b), 1.0);
        return;
    }

    if (uMode == 8) {
        FragColor = vec4(sample_value.rgb, 1.0);
        return;
    }

    if (uMode == 9) {
        vec3 mapped = sample_value.rgb / (sample_value.rgb + vec3(1.0));
        FragColor = vec4(pow(mapped, vec3(1.0 / 2.2)), 1.0);
        return;
    }

    FragColor = vec4(sample_value.rgb, 1.0);
}

