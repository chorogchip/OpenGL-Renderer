#version 330 core

out vec4 FragColor;

in vec3 LocalPos;

uniform samplerCube uEnvironmentMap;

const float PI = 3.14159265359;

void main() {
    vec3 normal = normalize(LocalPos);
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = cross(up, normal);
    if (length(right) < 0.0001) {
        right = vec3(1.0, 0.0, 0.0);
    }
    right = normalize(right);
    up = normalize(cross(normal, right));

    vec3 irradiance = vec3(0.0);
    float sample_delta = 0.025;
    float sample_count = 0.0;

    for (float phi = 0.0; phi < 2.0 * PI; phi += sample_delta) {
        for (float theta = 0.0; theta < 0.5 * PI; theta += sample_delta) {
            vec3 tangent_sample = vec3(
                sin(theta) * cos(phi),
                sin(theta) * sin(phi),
                cos(theta));
            vec3 sample_vec =
                tangent_sample.x * right +
                tangent_sample.y * up +
                tangent_sample.z * normal;

            irradiance += texture(uEnvironmentMap, sample_vec).rgb * cos(theta) * sin(theta);
            sample_count += 1.0;
        }
    }

    irradiance = PI * irradiance / max(sample_count, 1.0);
    FragColor = vec4(irradiance, 1.0);
}
