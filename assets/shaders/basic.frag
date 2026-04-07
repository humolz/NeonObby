#version 410 core

in vec3 vNormal;
in vec3 vFragPos;

uniform vec3 u_color;
uniform vec3 u_lightDir;
uniform vec3 u_viewPos;

out vec4 FragColor;

void main() {
    vec3 norm = normalize(vNormal);
    vec3 lightDir = normalize(u_lightDir);

    // Ambient
    float ambient = 0.15;

    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);

    // Specular (Blinn-Phong)
    vec3 viewDir = normalize(u_viewPos - vFragPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfDir), 0.0), 64.0);

    vec3 result = u_color * (ambient + diff * 0.7 + spec * 0.4);
    FragColor = vec4(result, 1.0);
}
