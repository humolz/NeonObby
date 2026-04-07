#version 410 core

in vec3 vNormal;
in vec3 vFragPos;

uniform vec3 u_baseColor;
uniform vec3 u_emissionColor;
uniform float u_emissionStrength;
uniform vec3 u_lightDir;
uniform vec3 u_viewPos;
uniform float u_time;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

void main() {
    vec3 norm = normalize(vNormal);
    vec3 lightDir = normalize(u_lightDir);

    // Ambient
    float ambient = 0.08;

    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);

    // Specular (Blinn-Phong)
    vec3 viewDir = normalize(u_viewPos - vFragPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfDir), 0.0), 64.0);

    // Base lighting
    vec3 lit = u_baseColor * (ambient + diff * 0.6 + spec * 0.3);

    // Emission with subtle pulse
    float pulse = 1.0 + 0.15 * sin(u_time * 2.0);
    vec3 emission = u_emissionColor * u_emissionStrength * pulse;

    // Fresnel rim glow — edges glow stronger
    float fresnel = 1.0 - max(dot(norm, viewDir), 0.0);
    fresnel = pow(fresnel, 3.0);
    vec3 rim = u_emissionColor * fresnel * u_emissionStrength * 0.5;

    vec3 finalColor = lit + emission + rim;
    FragColor = vec4(finalColor, 1.0);

    // Bright pass: extract pixels above brightness threshold for bloom
    float brightness = dot(finalColor, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 0.8) {
        BrightColor = vec4(finalColor, 1.0);
    } else {
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
