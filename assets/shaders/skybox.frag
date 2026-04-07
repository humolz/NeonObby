#version 410 core

in vec2 vTexCoord;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

void main() {
    // Dark gradient — deep purple at bottom, near-black at top
    vec3 bottomColor = vec3(0.03, 0.01, 0.08);
    vec3 topColor = vec3(0.005, 0.005, 0.015);
    vec3 color = mix(bottomColor, topColor, vTexCoord.y);

    FragColor = vec4(color, 1.0);
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0); // skybox never blooms
}
