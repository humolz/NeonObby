#pragma once

#include "ecs/World.h"
#include "components/TransformComponent.h"
#include "components/ObstacleComponent.h"
#include "components/RigidBodyComponent.h"
#include "components/ColliderComponent.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>

class ObstacleSystem {
public:
    void update(World& world, float dt) {
        world.each<TransformComponent, ObstacleComponent>(
            [&](Entity e, TransformComponent& tc, ObstacleComponent& obs) {
                switch (obs.type) {
                case ObstacleType::MovingPlatform:
                    updateMovingPlatform(tc, obs, dt);
                    break;
                case ObstacleType::RotatingPlatform:
                case ObstacleType::Spinner:
                    updateRotating(tc, obs, dt);
                    break;
                case ObstacleType::TimedObstacle:
                    updateTimed(world, e, tc, obs, dt);
                    break;
                default:
                    break;
                }
            }
        );
    }

private:
    void updateMovingPlatform(TransformComponent& tc, ObstacleComponent& obs, float dt) {
        if (obs.waypoints.size() < 2) return;

        obs.previousPosition = tc.position;

        // Pausing at waypoint
        if (obs.pauseTimer > 0.0f) {
            obs.pauseTimer -= dt;
            return;
        }

        int targetIdx = obs.forward
            ? obs.currentWaypoint + 1
            : obs.currentWaypoint - 1;

        glm::vec3 target = obs.waypoints[targetIdx];
        glm::vec3 dir = target - tc.position;
        float dist = glm::length(dir);

        if (dist < 0.05f) {
            tc.position = target;
            obs.currentWaypoint = targetIdx;
            obs.pauseTimer = obs.pauseTime;

            // Reverse at ends
            if (obs.currentWaypoint >= static_cast<int>(obs.waypoints.size()) - 1) {
                obs.forward = false;
            } else if (obs.currentWaypoint <= 0) {
                obs.forward = true;
            }
        } else {
            tc.position += glm::normalize(dir) * obs.speed * dt;
        }
    }

    void updateRotating(TransformComponent& tc, ObstacleComponent& obs, float dt) {
        tc.rotation += obs.rotationAxis * obs.rotationSpeed * dt;
        // Keep angles in 0-360 range
        for (int i = 0; i < 3; i++) {
            if (tc.rotation[i] > 360.0f) tc.rotation[i] -= 360.0f;
            if (tc.rotation[i] < 0.0f) tc.rotation[i] += 360.0f;
        }
    }

    void updateTimed(World& world, Entity e, TransformComponent& tc,
                     ObstacleComponent& obs, float dt) {
        obs.timer += dt;
        float cycleDuration = obs.onDuration + obs.offDuration;
        float phase = std::fmod(obs.timer + obs.phase, cycleDuration);
        bool shouldBeActive = phase < obs.onDuration;

        if (shouldBeActive != obs.active) {
            obs.active = shouldBeActive;
            if (shouldBeActive) {
                // Make visible and collidable — restore scale
                tc.scale = obs.cachedScale;
            } else {
                // Hide and disable collision — scale to zero
                obs.cachedScale = tc.scale;
                tc.scale = glm::vec3(0.0f);
            }
        }
    }
};
