#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>

#include "core/Settings.h"

class ThirdPersonCamera {
public:
    float distance = 8.0f;
    float heightOffset = 2.0f;
    float yaw = 0.0f;
    float pitch = 20.0f;
    float smoothSpeed = 10.0f;
    float minPitch = -30.0f;
    float maxPitch = 60.0f;
    float minDist = 2.0f;
    float maxDist = 20.0f;
    float fov = 60.0f;
    float baseFov = 60.0f;
    float targetFov = 60.0f;
    float fovSpeed = 8.0f;  // lerp speed

    void processInput(float dx, float dy, float /*scroll*/) {
        const auto& s = Settings::get();
        float sens = s.mouseSens;
        float yawDelta   = dx * sens;
        float pitchDelta = dy * sens * (s.invertY ? -1.0f : 1.0f);

        yaw += yawDelta;
        pitch -= pitchDelta;
        pitch = std::clamp(pitch, minPitch, maxPitch);
        // Scroll-wheel distance zoom was removed in v1.0.3 — users now control
        // perceived zoom with the FOV slider in Settings, and scrolling no
        // longer fights it.
    }

    void update(const glm::vec3& targetPos, float dt) {
        m_target = targetPos + glm::vec3(0, heightOffset, 0);

        glm::vec3 desired = calcDesiredPosition();

        // Smooth follow
        float t = 1.0f - std::exp(-smoothSpeed * dt);
        m_currentPos = glm::mix(m_currentPos, desired, t);

        // Keep baseFov synced with the user's FOV slider so edits in the
        // pause menu apply instantly. GameScene resets targetFov to baseFov
        // when no speed-boost effect is active.
        baseFov = Settings::get().fov;

        // Smooth FOV transition (for speed boost effect)
        float fovT = 1.0f - std::exp(-fovSpeed * dt);
        fov = glm::mix(fov, targetFov, fovT);
    }

    glm::mat4 viewMatrix() const {
        return glm::lookAt(m_currentPos, m_target, glm::vec3(0, 1, 0));
    }

    glm::mat4 projectionMatrix(float aspect) const {
        return glm::perspective(glm::radians(fov), aspect, 0.1f, 500.0f);
    }

    glm::vec3 position() const { return m_currentPos; }

    // Forward direction on the XZ plane (for player movement relative to camera)
    glm::vec3 forward() const {
        float yawRad = glm::radians(yaw);
        return glm::normalize(glm::vec3(-std::sin(yawRad), 0, -std::cos(yawRad)));
    }

    glm::vec3 right() const {
        float yawRad = glm::radians(yaw);
        return glm::normalize(glm::vec3(std::cos(yawRad), 0, -std::sin(yawRad)));
    }

private:
    glm::vec3 m_currentPos{0, 5, 10};
    glm::vec3 m_target{0, 0, 0};

    glm::vec3 calcDesiredPosition() const {
        float yawRad = glm::radians(yaw);
        float pitchRad = glm::radians(pitch);
        float cosP = std::cos(pitchRad);
        return m_target + glm::vec3(
            distance * cosP * std::sin(yawRad),
            distance * std::sin(pitchRad),
            distance * cosP * std::cos(yawRad)
        );
    }
};
