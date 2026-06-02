#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D uTexture;
uniform samplerCube uCubemapTexture;
uniform int uMode;

const float PI = 3.14159265359;

vec3 latlong_direction(vec2 uv) {
    float phi = (uv.x - 0.5) * 2.0 * PI;
    float theta = (uv.y - 0.5) * PI;
    float cos_theta = cos(theta);
    return normalize(vec3(
        cos_theta * cos(phi),
        sin(theta),
        cos_theta * sin(phi)));
}

vec3 tonemap_hdr(vec3 color) {
    vec3 mapped = color / (color + vec3(1.0));
    return pow(mapped, vec3(1.0 / 2.2));
}

void main() {
    if (uMode == 10) {
        vec3 environment = texture(uCubemapTexture, latlong_direction(TexCoord)).rgb;
        FragColor = vec4(tonemap_hdr(environment), 1.0);
        return;
    }

    if (uMode == 11) {
        vec3 irradiance = texture(uCubemapTexture, latlong_direction(TexCoord)).rgb;
        FragColor = vec4(tonemap_hdr(irradiance), 1.0);
        return;
    }

    if (uMode >= 12 && uMode <= 16) {
        vec3 prefilter = textureLod(
            uCubemapTexture,
            latlong_direction(TexCoord),
            float(uMode - 12)).rgb;
        FragColor = vec4(tonemap_hdr(prefilter), 1.0);
        return;
    }

    vec4 sample_value = texture(uTexture, TexCoord);

    if (uMode == 1) {
        FragColor = vec4(pow(sample_value.rgb, vec3(1.0 / 2.2)), 1.0);
        return;
    }

    if (uMode == 2) {
        vec3 normal = normalize(sample_value.rgb);
        FragColor = vec4(normal * 0.5 + 0.5, 1.0);
        return;
    }

    if (uMode == 3) {
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
        FragColor = vec4(tonemap_hdr(sample_value.rgb), 1.0);
        return;
    }

    if (uMode == 17) {
        FragColor = vec4(sample_value.r, sample_value.g, 0.0, 1.0);
        return;
    }

    FragColor = vec4(sample_value.rgb, 1.0);
}
