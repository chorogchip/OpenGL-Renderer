#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D uSceneColor;

float luminance(vec3 color) {
    return dot(color, vec3(0.299, 0.587, 0.114));
}

vec3 fxaa(sampler2D tex, vec2 uv) {
    vec2 texel_size = 1.0 / vec2(textureSize(tex, 0));

    vec3 center = texture(tex, uv).rgb;
    float center_lum = luminance(center);

    vec3 up = texture(tex, uv + vec2(0.0, texel_size.y)).rgb;
    vec3 down = texture(tex, uv - vec2(0.0, texel_size.y)).rgb;
    vec3 left = texture(tex, uv - vec2(texel_size.x, 0.0)).rgb;
    vec3 right = texture(tex, uv + vec2(texel_size.x, 0.0)).rgb;

    float up_lum = luminance(up);
    float down_lum = luminance(down);
    float left_lum = luminance(left);
    float right_lum = luminance(right);

    float max_lum = max(center_lum, max(max(up_lum, down_lum), max(left_lum, right_lum)));
    float min_lum = min(center_lum, min(min(up_lum, down_lum), min(left_lum, right_lum)));
    float lum_range = max_lum - min_lum;

    if (lum_range < max(0.0313, max_lum * 0.125)) {
        return center;
    }

    vec2 edge = vec2(abs(up_lum + down_lum - 2.0 * center_lum),
                     abs(left_lum + right_lum - 2.0 * center_lum));

    bool is_horizontal = edge.x > edge.y;
    float pixel_step = is_horizontal ? texel_size.y : texel_size.x;

    float lum_1 = is_horizontal ? up_lum : left_lum;
    float lum_2 = is_horizontal ? down_lum : right_lum;

    bool is_positive = abs(lum_1 - center_lum) > abs(lum_2 - center_lum);
    float gradient = is_positive ? abs(lum_1 - center_lum) : abs(lum_2 - center_lum);

    float step_sign = is_positive ? 1.0 : -1.0;

    vec2 offset = is_horizontal ? vec2(0.0, step_sign * pixel_step) : vec2(step_sign * pixel_step, 0.0);
    vec2 sample_pos = uv + offset * 1.5;

    vec3 sample_color = texture(tex, sample_pos).rgb;
    return mix(center, sample_color, 0.25);
}

void main() {
    vec3 fxaa_result = fxaa(uSceneColor, TexCoord);
    FragColor = vec4(fxaa_result, 1.0);
}
