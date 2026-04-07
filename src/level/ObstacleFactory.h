#pragma once

#include "ecs/World.h"
#include "components/TransformComponent.h"
#include "components/MeshComponent.h"
#include "components/MaterialComponent.h"
#include "components/ColliderComponent.h"
#include "components/ObstacleComponent.h"
#include "components/TriggerComponent.h"
#include "components/CheckpointComponent.h"
#include "components/RigidBodyComponent.h"
#include <glm/glm.hpp>
#include <vector>

namespace ObstacleFactory {

inline Entity createPlatform(World& world, const glm::vec3& pos, const glm::vec3& scale,
                              const glm::vec3& baseColor, const glm::vec3& emissionColor,
                              float emissionStrength = 0.5f) {
    Entity e = world.create();
    auto& tc = world.add<TransformComponent>(e);
    tc.position = pos;
    tc.scale = scale;
    world.add<MeshComponent>(e, {MeshType::Cube});
    world.add<MaterialComponent>(e, {baseColor, emissionColor, emissionStrength});
    auto& col = world.add<ColliderComponent>(e);
    col.type = ColliderType::AABBCollider;
    col.halfExtents = glm::vec3(0.5f);
    return e;
}

inline Entity createMovingPlatform(World& world, const glm::vec3& pos, const glm::vec3& scale,
                                    const glm::vec3& baseColor, const glm::vec3& emissionColor,
                                    float emissionStrength,
                                    const std::vector<glm::vec3>& waypoints,
                                    float speed = 3.0f, float pauseTime = 0.5f) {
    Entity e = createPlatform(world, pos, scale, baseColor, emissionColor, emissionStrength);
    auto& obs = world.add<ObstacleComponent>(e);
    obs.type = ObstacleType::MovingPlatform;
    obs.waypoints = waypoints;
    obs.speed = speed;
    obs.pauseTime = pauseTime;
    obs.previousPosition = pos;
    return e;
}

inline Entity createRotatingPlatform(World& world, const glm::vec3& pos, const glm::vec3& scale,
                                      const glm::vec3& baseColor, const glm::vec3& emissionColor,
                                      float emissionStrength,
                                      const glm::vec3& rotAxis = {0, 1, 0},
                                      float rotSpeed = 45.0f) {
    Entity e = createPlatform(world, pos, scale, baseColor, emissionColor, emissionStrength);
    auto& obs = world.add<ObstacleComponent>(e);
    obs.type = ObstacleType::RotatingPlatform;
    obs.rotationAxis = rotAxis;
    obs.rotationSpeed = rotSpeed;
    return e;
}

inline Entity createSpinner(World& world, const glm::vec3& pos, const glm::vec3& scale,
                             const glm::vec3& baseColor, const glm::vec3& emissionColor,
                             float emissionStrength, float rotSpeed = 90.0f) {
    Entity e = createPlatform(world, pos, scale, baseColor, emissionColor, emissionStrength);
    auto& obs = world.add<ObstacleComponent>(e);
    obs.type = ObstacleType::Spinner;
    obs.rotationAxis = {0, 1, 0};
    obs.rotationSpeed = rotSpeed;
    return e;
}

inline Entity createTimedPlatform(World& world, const glm::vec3& pos, const glm::vec3& scale,
                                   const glm::vec3& baseColor, const glm::vec3& emissionColor,
                                   float emissionStrength,
                                   float onDuration = 2.0f, float offDuration = 1.5f,
                                   float phase = 0.0f) {
    Entity e = createPlatform(world, pos, scale, baseColor, emissionColor, emissionStrength);
    auto& obs = world.add<ObstacleComponent>(e);
    obs.type = ObstacleType::TimedObstacle;
    obs.onDuration = onDuration;
    obs.offDuration = offDuration;
    obs.phase = phase;
    obs.cachedScale = scale;
    return e;
}

inline Entity createLowWall(World& world, const glm::vec3& pos, const glm::vec3& scale,
                             const glm::vec3& baseColor, const glm::vec3& emissionColor,
                             float emissionStrength = 0.5f) {
    Entity e = createPlatform(world, pos, scale, baseColor, emissionColor, emissionStrength);
    auto& obs = world.add<ObstacleComponent>(e);
    obs.type = ObstacleType::LowWall;
    return e;
}

inline Entity createCrawlTunnel(World& world, const glm::vec3& pos, const glm::vec3& scale,
                                 const glm::vec3& baseColor, const glm::vec3& emissionColor,
                                 float emissionStrength = 0.5f) {
    Entity e = createPlatform(world, pos, scale, baseColor, emissionColor, emissionStrength);
    auto& obs = world.add<ObstacleComponent>(e);
    obs.type = ObstacleType::CrawlTunnel;
    return e;
}

// Triggers
inline Entity createKillZone(World& world, const glm::vec3& pos, const glm::vec3& scale) {
    Entity e = world.create();
    auto& tc = world.add<TransformComponent>(e);
    tc.position = pos;
    tc.scale = scale;
    // Kill zones are invisible — no mesh/material
    auto& col = world.add<ColliderComponent>(e);
    col.type = ColliderType::AABBCollider;
    col.halfExtents = glm::vec3(0.5f);
    auto& trigger = world.add<TriggerComponent>(e);
    trigger.type = TriggerType::KillZone;
    return e;
}

inline Entity createJumpPad(World& world, const glm::vec3& pos, const glm::vec3& scale,
                             float strength = 15.0f) {
    Entity e = createPlatform(world, pos, scale,
                               {0.08f, 0.06f, 0.02f}, {1.0f, 0.8f, 0.0f}, 1.0f);
    auto& trigger = world.add<TriggerComponent>(e);
    trigger.type = TriggerType::JumpPad;
    trigger.strength = strength;
    return e;
}

inline Entity createSpeedBoost(World& world, const glm::vec3& pos, const glm::vec3& scale,
                                float strength = 20.0f) {
    Entity e = createPlatform(world, pos, scale,
                               {0.02f, 0.08f, 0.06f}, {0.0f, 1.0f, 0.5f}, 1.0f);
    auto& trigger = world.add<TriggerComponent>(e);
    trigger.type = TriggerType::SpeedBoost;
    trigger.strength = strength;
    return e;
}

inline Entity createCheckpoint(World& world, const glm::vec3& pos, int index,
                                const glm::vec3& respawnPos) {
    Entity e = world.create();
    auto& tc = world.add<TransformComponent>(e);
    tc.position = pos;
    tc.scale = {2.5f, 0.2f, 2.5f};
    world.add<MeshComponent>(e, {MeshType::Cube});
    world.add<MaterialComponent>(e, {{0.02f, 0.06f, 0.08f}, {0.0f, 0.5f, 1.0f}, 1.2f});
    auto& col = world.add<ColliderComponent>(e);
    col.type = ColliderType::AABBCollider;
    col.halfExtents = glm::vec3(0.5f);
    auto& trigger = world.add<TriggerComponent>(e);
    trigger.type = TriggerType::Checkpoint;
    trigger.checkpointIndex = index;
    auto& cp = world.add<CheckpointComponent>(e);
    cp.index = index;
    cp.respawnPosition = respawnPos;
    return e;
}

inline Entity createFinishLine(World& world, const glm::vec3& pos, const glm::vec3& scale) {
    Entity e = createPlatform(world, pos, scale,
                               {0.08f, 0.08f, 0.02f}, {1.0f, 1.0f, 0.0f}, 1.5f);
    auto& trigger = world.add<TriggerComponent>(e);
    trigger.type = TriggerType::LevelFinish;
    return e;
}

} // namespace ObstacleFactory
