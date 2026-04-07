#version 410 core

in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D u_image;
uniform bool u_horizontal;

// 13-tap Gaussian weights
const float weights[7] = float[](
    0.1981, 0.1753, 0.1216, 0.0660, 0.0280, 0.0093, 0.0024
);

void main() {
    vec2 texelSize = 1.0 / textureSize(u_image, 0);
    vec3 result = texture(u_image, vTexCoord).rgb * weights[0];

    if (u_horizontal) {
        for (int i = 1; i < 7; i++) {
            vec2 offset = vec2(texelSize.x * float(i), 0.0);
            result += texture(u_image, vTexCoord + offset).rgb * weights[i];
            result += texture(u_image, vTexCoord - offset).rgb * weights[i];
        }
    } else {
        for (int i = 1; i < 7; i++) {
            vec2 offset = vec2(0.0, texelSize.y * float(i));
            result += texture(u_image, vTexCoord + offset).rgb * weights[i];
            result += texture(u_image, vTexCoord - offset).rgb * weights[i];
        }
    }

    FragColor = vec4(result, 1.0);
}
