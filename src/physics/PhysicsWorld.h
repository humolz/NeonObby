#pragma once

#include "ecs/World.h"
#include "physics/CollisionTests.h"
#include "components/TransformComponent.h"
#include "components/RigidBodyComponent.h"
#include "components/ColliderComponent.h"
#include <glm/glm.hpp>

class PhysicsWorld {
public:
    glm::vec3 gravity{0.0f, -25.0f, 0.0f};

    void step(World& world, float dt) {
        // 1. Integrate velocity and position for dynamic bodies
        world.each<TransformComponent, RigidBodyComponent>(
            [&](Entity, TransformComponent& tc, RigidBodyComponent& rb) {
                if (rb.isKinematic) return;

                // Apply gravity
                rb.velocity += (gravity * rb.gravityScale + rb.acceleration) * dt;
                rb.acceleration = glm::vec3(0.0f);

                // Integrate position
                tc.position += rb.velocity * dt;

                // Ground friction (horizontal only, when grounded)
                if (rb.isGrounded) {
                    rb.velocity.x *= rb.friction;
                    rb.velocity.z *= rb.friction;
                }
            }
        );

        // 2. Collision detection and resolution
        struct ColInfo {
            Entity entity;
            TransformComponent* tc;
            RigidBodyComponent* rb;
            ColliderComponent* col;
        };
        std::vector<ColInfo> dynamics;
        std::vector<ColInfo> statics;

        world.each<TransformComponent, ColliderComponent>(
            [&](Entity e, TransformComponent& tc, ColliderComponent& col) {
                RigidBodyComponent* rb = world.has<RigidBodyComponent>(e) ? &world.get<RigidBodyComponent>(e) : nullptr;
                if (rb && !rb->isKinematic) {
                    dynamics.push_back({e, &tc, rb, &col});
                } else {
                    statics.push_back({e, &tc, nullptr, &col});
                }
            }
        );

        // Save previous grounded state (for ground snap pass below)
        std::vector<bool> wasGroundedAtStart(dynamics.size(), false);
        for (size_t i = 0; i < dynamics.size(); i++) {
            wasGroundedAtStart[i] = dynamics[i].rb->isGrounded;
        }

        // Reset grounded state
        for (auto& d : dynamics) {
            d.rb->isGrounded = false;
            d.rb->groundEntity = Entity::null();
        }

        // Dynamic vs Static collision
        for (auto& dyn : dynamics) {
            for (auto& stat : statics) {
                Contact contact;

                // Use rotation-aware AABB for rotated static objects
                bool hasRotation = (stat.tc->rotation.x != 0.0f ||
                                    stat.tc->rotation.y != 0.0f ||
                                    stat.tc->rotation.z != 0.0f);

                if (dyn.col->type == ColliderType::CapsuleCollider) {
                    AABB box = hasRotation
                        ? stat.col->getWorldAABB(stat.tc->position, stat.tc->scale, stat.tc->rotation)
                        : stat.col->getAABB(stat.tc->position, stat.tc->scale);
                    contact = CollisionTests::capsuleVsAabb(
                        dyn.col->capsule, dyn.tc->position, box);
                } else {
                    AABB a = dyn.col->getAABB(dyn.tc->position, dyn.tc->scale);
                    AABB b = hasRotation
                        ? stat.col->getWorldAABB(stat.tc->position, stat.tc->scale, stat.tc->rotation)
                        : stat.col->getAABB(stat.tc->position, stat.tc->scale);
                    contact = CollisionTests::aabbVsAabb(a, b);
                }

                if (contact.hit) {
                    resolveContact(*dyn.tc, *dyn.rb, contact, stat.entity);
                }
            }
        }

        // Ground snap pass: keep capsules stuck to ground when hovering just above.
        // Without this, a moving platform causes the player to bounce every frame
        // (capsule barely loses contact, gravity pulls down, collision pushes up,
        // repeat — retriggering the land SFX and producing a visible jitter).
        // Only snaps capsules that were grounded last frame and aren't jumping.
        const float SNAP_DIST = 0.15f;
        for (size_t i = 0; i < dynamics.size(); i++) {
            if (!wasGroundedAtStart[i]) continue;
            auto& dyn = dynamics[i];
            if (dyn.rb->isGrounded) continue; // already resolved by main pass
            if (dyn.col->type != ColliderType::CapsuleCollider) continue;
            if (dyn.rb->velocity.y > 1.0f) continue; // genuine jump, don't snap

            glm::vec3 testCenter = dyn.tc->position - glm::vec3(0, SNAP_DIST, 0);

            Contact bestContact;
            Entity bestEnt = Entity::null();

            for (auto& stat : statics) {
                bool hasRotation = (stat.tc->rotation.x != 0.0f ||
                                    stat.tc->rotation.y != 0.0f ||
                                    stat.tc->rotation.z != 0.0f);
                AABB box = hasRotation
                    ? stat.col->getWorldAABB(stat.tc->position, stat.tc->scale, stat.tc->rotation)
                    : stat.col->getAABB(stat.tc->position, stat.tc->scale);

                Contact c = CollisionTests::capsuleVsAabb(dyn.col->capsule, testCenter, box);
                if (c.hit && c.normal.y > 0.7f) {
                    if (!bestEnt.valid() || c.penetration > bestContact.penetration) {
                        bestContact = c;
                        bestEnt = stat.entity;
                    }
                }
            }

            if (bestEnt.valid()) {
                dyn.tc->position = testCenter + bestContact.normal * bestContact.penetration;
                if (dyn.rb->velocity.y < 0.0f) dyn.rb->velocity.y = 0.0f;
                dyn.rb->isGrounded = true;
                dyn.rb->groundEntity = bestEnt;
            }
        }
    }

    // Raycast against all static colliders
    RayHit raycast(World& world, const Ray& ray, float maxDist, Entity ignore = Entity::null()) {
        RayHit bestHit;
        bestHit.distance = maxDist;

        world.each<TransformComponent, ColliderComponent>(
            [&](Entity e, TransformComponent& tc, ColliderComponent& col) {
                if (e == ignore) return;
                AABB box = col.getAABB(tc.position, tc.scale);
                RayHit hit = CollisionTests::rayVsAabb(ray, box);
                if (hit.hit && hit.distance < bestHit.distance && hit.distance >= 0.0f) {
                    bestHit = hit;
                }
            }
        );

        return bestHit;
    }

private:
    void resolveContact(TransformComponent& tc, RigidBodyComponent& rb,
                        const Contact& contact, Entity groundEnt) {
        // Push out of collision
        tc.position += contact.normal * contact.penetration;

        // Cancel velocity into the collision normal
        float velAlongNormal = glm::dot(rb.velocity, contact.normal);
        if (velAlongNormal < 0.0f) {
            rb.velocity -= contact.normal * velAlongNormal;
        }

        // Ground detection: if the contact normal points mostly upward
        if (contact.normal.y > 0.7f) {
            rb.isGrounded = true;
            rb.groundEntity = groundEnt;
        }
    }
};
