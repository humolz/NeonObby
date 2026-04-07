#include "renderer/Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>

Camera::Camera(float fovDeg, float aspect, float nearPlane, float farPlane)
    : m_fov(fovDeg), m_aspect(aspect), m_near(nearPlane), m_far(farPlane)
{
}

void Camera::setAspect(float aspect) {
    m_aspect = aspect;
}

void Camera::processMouseMovement(float dx, float dy) {
    m_yaw += dx * m_sensitivity;
    m_pitch -= dy * m_sensitivity;
    m_pitch = std::clamp(m_pitch, m_minPitch, m_maxPitch);
}

void Camera::processScroll(float offset) {
    m_distance -= offset * 0.5f;
    m_distance = std::clamp(m_distance, m_minDist, m_maxDist);
}

glm::vec3 Camera::position() const {
    float yawRad = glm::radians(m_yaw);
    float pitchRad = glm::radians(m_pitch);
    float cosP = std::cos(pitchRad);
    return m_target + glm::vec3(
        m_distance * cosP * std::cos(yawRad),
        m_distance * std::sin(pitchRad),
        m_distance * cosP * std::sin(yawRad)
    );
}

glm::mat4 Camera::viewMatrix() const {
    return glm::lookAt(position(), m_target, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::projectionMatrix() const {
    return glm::perspective(glm::radians(m_fov), m_aspect, m_near, m_far);
}
