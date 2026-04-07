#pragma once

#include <glm/glm.hpp>

class Camera {
public:
    Camera(float fovDeg = 45.0f, float aspect = 16.0f / 9.0f, float nearPlane = 0.1f, float farPlane = 500.0f);

    void setAspect(float aspect);

    void processMouseMovement(float dx, float dy);
    void processScroll(float offset);

    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix() const;
    glm::vec3 position() const;

private:
    float m_fov;
    float m_aspect;
    float m_near;
    float m_far;

    float m_distance = 5.0f;
    float m_yaw = -45.0f;
    float m_pitch = 30.0f;
    glm::vec3 m_target = {0.0f, 0.0f, 0.0f};

    float m_sensitivity = 0.2f;
    float m_minDist = 1.5f;
    float m_maxDist = 50.0f;
    float m_minPitch = -89.0f;
    float m_maxPitch = 89.0f;
};
