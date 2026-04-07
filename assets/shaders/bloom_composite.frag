#version 410 core

in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D u_scene;
uniform sampler2D u_bloom;
uniform float u_bloomIntensity;
uniform float u_exposure;

void main() {
    vec3 sceneColor = texture(u_scene, vTexCoord).rgb;
    vec3 bloomColor = texture(u_bloom, vTexCoord).rgb;

    // Additive blend
    vec3 combined = sceneColor + bloomColor * u_bloomIntensity;

    // Tone mapping (ACES approximation)
    combined = combined * u_exposure;
    vec3 mapped = (combined * (2.51 * combined + 0.03)) / (combined * (2.43 * combined + 0.59) + 0.14);

    // Gamma correction
    mapped = pow(mapped, vec3(1.0 / 2.2));

    FragColor = vec4(mapped, 1.0);
}
