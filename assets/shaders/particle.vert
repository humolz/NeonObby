#version 410 core

// Per-vertex (unit quad)
layout(location = 0) in vec2 a_quadPos;

// Per-instance
layout(location = 1) in vec3 a_worldPos;
layout(location = 2) in vec4 a_color;
layout(location = 3) in float a_size;

uniform mat4 u_viewProj;
uniform vec3 u_cameraRight;
uniform vec3 u_cameraUp;

out vec4 v_color;
out vec2 v_uv;

void main() {
    vec3 worldPos = a_worldPos
        + u_cameraRight * a_quadPos.x * a_size
        + u_cameraUp * a_quadPos.y * a_size;

    gl_Position = u_viewProj * vec4(worldPos, 1.0);
    v_color = a_color;
    v_uv = a_quadPos + 0.5;
}
