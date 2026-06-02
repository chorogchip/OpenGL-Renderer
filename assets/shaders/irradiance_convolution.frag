#version 330 core

out vec4 FragColor;

in vec3 LocalPos;

uniform samplerCube uEnvironmentMap;

const float PI = 3.14159265359;
const float GOLDEN_ANGLE = 2.39996322973;
const int SAMPLE_COUNT = 2048;
const float MAX_SAMPLE_LUMINANCE = 20.0;

void build_tangent_frame(vec3 normal, out vec3 right, out vec3 up) {
    vec3 helper = abs(normal.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    right = normalize(cross(helper, normal));
    up = normalize(cross(normal, right));
}

vec3 clamp_sample_luminance(vec3 color) {
    color = max(color, vec3(0.0));
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if (luminance > MAX_SAMPLE_LUMINANCE) {
        color *= MAX_SAMPLE_LUMINANCE / luminance;
    }
    return color;
}

void main() {
    vec3 normal = normalize(LocalPos);
    vec3 right;
    vec3 up;
    build_tangent_frame(normal, right, up);

    vec3 irradiance = vec3(0.0);

    for (int i = 0; i < SAMPLE_COUNT; ++i) {
        float xi = (float(i) + 0.5) / float(SAMPLE_COUNT);
        float phi = float(i) * GOLDEN_ANGLE;
        float radius = sqrt(xi);
        vec3 tangent_sample = vec3(
            radius * cos(phi),
            radius * sin(phi),
            sqrt(max(1.0 - xi, 0.0)));
        vec3 sample_vec = normalize(
            tangent_sample.x * right +
            tangent_sample.y * up +
            tangent_sample.z * normal);

        irradiance += clamp_sample_luminance(texture(uEnvironmentMap, sample_vec).rgb);
    }

    irradiance = irradiance / float(SAMPLE_COUNT);
    FragColor = vec4(irradiance, 1.0);
}
