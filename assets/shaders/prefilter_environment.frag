#version 330 core

out vec4 FragColor;

in vec3 LocalPos;

uniform samplerCube uEnvironmentMap;
uniform float uRoughness;

const float PI = 3.14159265359;
const float MAX_SAMPLE_LUMINANCE = 20.0;
const uint SAMPLE_COUNT = 2048u;

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

float radical_inverse_vdc(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 hammersley(uint i, uint sample_count) {
    return vec2(float(i) / float(sample_count), radical_inverse_vdc(i));
}

vec3 importance_sample_ggx(vec2 xi, vec3 normal, float roughness) {
    float a = roughness * roughness;
    float phi = 2.0 * PI * xi.x;
    float cos_theta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    float sin_theta = sqrt(max(1.0 - cos_theta * cos_theta, 0.0));

    vec3 half_vector = vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
    vec3 right;
    vec3 up;
    build_tangent_frame(normal, right, up);
    return normalize(right * half_vector.x + up * half_vector.y + normal * half_vector.z);
}

void main() {
    vec3 normal = normalize(LocalPos);
    vec3 view_dir = normal;
    vec3 prefiltered_color = vec3(0.0);
    float total_weight = 0.0;

    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        vec2 xi = hammersley(i, SAMPLE_COUNT);
        vec3 half_vector = importance_sample_ggx(xi, normal, uRoughness);
        vec3 light_dir = normalize(2.0 * dot(view_dir, half_vector) * half_vector - view_dir);
        float n_dot_l = max(dot(normal, light_dir), 0.0);
        if (n_dot_l > 0.0) {
            prefiltered_color += clamp_sample_luminance(texture(uEnvironmentMap, light_dir).rgb) * n_dot_l;
            total_weight += n_dot_l;
        }
    }

    prefiltered_color /= max(total_weight, 0.0001);
    FragColor = vec4(prefiltered_color, 1.0);
}
