#version 410 core

in vec4 v_color;
in vec2 v_uv;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

void main() {
    // Soft glowing circle
    float dist = length(v_uv - 0.5);
    float alpha = 1.0 - smoothstep(0.2, 0.5, dist);

    vec4 color = v_color;
    color.a *= alpha;

    if (color.a < 0.01) discard;

    FragColor = color;

    // Bloom: particles always glow
    float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    BrightColor = (brightness > 0.2) ? vec4(color.rgb * color.a, 1.0) : vec4(0.0);
}
