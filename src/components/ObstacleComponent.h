#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

enum class ObstacleType : uint8_t {
    StaticPlatform,
    MovingPlatform,
    RotatingPlatform,
    Spinner,
    TimedObstacle,
    LowWall,
    CrawlTunnel
};

struct ObstacleComponent {
    ObstacleType type = ObstacleType::StaticPlatform;

    // Moving platform
    std::vector<glm::vec3> waypoints;
    float speed = 3.0f;
    float pauseTime = 0.5f;
    int currentWaypoint = 0;
    bool forward = true;
    float pauseTimer = 0.0f;
    glm::vec3 previousPosition{0.0f}; // for delta tracking

    // Rotating
    glm::vec3 rotationAxis{0, 1, 0};
    float rotationSpeed = 45.0f; // degrees per second

    // Timed
    float onDuration = 2.0f;
    float offDuration = 1.5f;
    float phase = 0.0f;
    float timer = 0.0f;
    bool active = true;
    glm::vec3 cachedScale{1.0f}; // stored when timed obstacle hides
};
