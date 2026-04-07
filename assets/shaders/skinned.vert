#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in ivec4 aJoints;
layout (location = 3) in vec4 aWeights;

uniform mat4 u_model;
uniform mat4 u_viewProj;
uniform mat4 u_bones[64];

out vec3 vNormal;
out vec3 vFragPos;

void main() {
    // Skinning: weighted blend of bone transforms
    mat4 boneMat =
        u_bones[aJoints.x] * aWeights.x +
        u_bones[aJoints.y] * aWeights.y +
        u_bones[aJoints.z] * aWeights.z +
        u_bones[aJoints.w] * aWeights.w;

    vec4 skinnedPos = boneMat * vec4(aPos, 1.0);
    vec4 worldPos = u_model * skinnedPos;
    vFragPos = worldPos.xyz;

    mat3 normalMat = mat3(transpose(inverse(u_model * boneMat)));
    vNormal = normalMat * aNormal;

    gl_Position = u_viewProj * worldPos;
}
