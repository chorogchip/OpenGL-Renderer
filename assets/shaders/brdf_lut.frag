#version 330 core

out vec2 FragColor;

in vec2 TexCoord;

const float PI = 3.14159265359;

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
    vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, normal));
    vec3 bitangent = cross(normal, tangent);
    return normalize(tangent * half_vector.x + bitangent * half_vector.y + normal * half_vector.z);
}

float geometry_schlick_ggx(float n_dot_v, float roughness) {
    float a = roughness;
    float k = (a * a) / 2.0;
    return n_dot_v / max(n_dot_v * (1.0 - k) + k, 0.0001);
}

float geometry_smith(float n_dot_v, float n_dot_l, float roughness) {
    return geometry_schlick_ggx(n_dot_v, roughness) *
        geometry_schlick_ggx(n_dot_l, roughness);
}

vec2 integrate_brdf(float n_dot_v, float roughness) {
    vec3 view_dir = vec3(sqrt(max(1.0 - n_dot_v * n_dot_v, 0.0)), 0.0, n_dot_v);
    vec3 normal = vec3(0.0, 0.0, 1.0);
    float a = 0.0;
    float b = 0.0;
    const uint sample_count = 1024u;

    for (uint i = 0u; i < sample_count; ++i) {
        vec2 xi = hammersley(i, sample_count);
        vec3 half_vector = importance_sample_ggx(xi, normal, roughness);
        vec3 light_dir = normalize(2.0 * dot(view_dir, half_vector) * half_vector - view_dir);
        float n_dot_l = max(light_dir.z, 0.0);
        float n_dot_h = max(half_vector.z, 0.0);
        float v_dot_h = max(dot(view_dir, half_vector), 0.0);

        if (n_dot_l > 0.0) {
            float geometry = geometry_smith(n_dot_v, n_dot_l, roughness);
            float geometry_vis = geometry * v_dot_h / max(n_dot_h * n_dot_v, 0.0001);
            float fresnel = pow(1.0 - v_dot_h, 5.0);
            a += (1.0 - fresnel) * geometry_vis;
            b += fresnel * geometry_vis;
        }
    }

    return vec2(a, b) / float(sample_count);
}

void main() {
    FragColor = integrate_brdf(TexCoord.x, TexCoord.y);
}
