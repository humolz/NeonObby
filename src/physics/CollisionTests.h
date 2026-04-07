#pragma once

#include "physics/AABB.h"
#include "physics/Capsule.h"
#include "physics/Ray.h"
#include <glm/glm.hpp>
#include <cmath>
#include <algorithm>

struct Contact {
    bool hit = false;
    glm::vec3 normal{0.0f};   // push direction for object A (out of B)
    float penetration = 0.0f;
    glm::vec3 point{0.0f};
};

namespace CollisionTests {

// Closest point on line segment AB to point P
inline glm::vec3 closestPointOnSegment(const glm::vec3& a, const glm::vec3& b, const glm::vec3& p) {
    glm::vec3 ab = b - a;
    float t = glm::dot(p - a, ab) / glm::dot(ab, ab);
    t = std::clamp(t, 0.0f, 1.0f);
    return a + ab * t;
}

// AABB vs AABB
inline Contact aabbVsAabb(const AABB& a, const AABB& b) {
    Contact c;
    glm::vec3 ac = a.center(), bc = b.center();
    glm::vec3 ah = a.halfExtents(), bh = b.halfExtents();
    glm::vec3 delta = bc - ac;
    glm::vec3 overlap = ah + bh - glm::abs(delta);

    if (overlap.x <= 0 || overlap.y <= 0 || overlap.z <= 0) return c;

    c.hit = true;
    // Push along axis of minimum overlap
    if (overlap.x <= overlap.y && overlap.x <= overlap.z) {
        c.normal = glm::vec3(delta.x < 0 ? 1.0f : -1.0f, 0, 0);
        c.penetration = overlap.x;
    } else if (overlap.y <= overlap.x && overlap.y <= overlap.z) {
        c.normal = glm::vec3(0, delta.y < 0 ? 1.0f : -1.0f, 0);
        c.penetration = overlap.y;
    } else {
        c.normal = glm::vec3(0, 0, delta.z < 0 ? 1.0f : -1.0f);
        c.penetration = overlap.z;
    }
    c.point = ac + delta * 0.5f;
    return c;
}

// Capsule vs AABB — the critical player collision test
inline Contact capsuleVsAabb(const Capsule& cap, const glm::vec3& capCenter, const AABB& box) {
    Contact c;

    // Find closest point on capsule segment to AABB
    glm::vec3 segTop = cap.top(capCenter);
    glm::vec3 segBot = cap.bottom(capCenter);

    // Find closest point on the AABB to the capsule segment
    // We test a few sample points on the segment and pick the one closest to the AABB
    glm::vec3 bestSegPt = capCenter;
    float bestDist = 1e30f;

    // Check segment endpoints and center against AABB
    glm::vec3 testPoints[] = {segBot, capCenter, segTop};
    for (auto& pt : testPoints) {
        glm::vec3 closest = box.closestPoint(pt);
        float d = glm::length(closest - pt);
        if (d < bestDist) {
            bestDist = d;
            bestSegPt = pt;
        }
    }

    // Now find the closest point on the segment to the AABB closest point
    glm::vec3 aabbClosest = box.closestPoint(bestSegPt);
    glm::vec3 segClosest = closestPointOnSegment(segBot, segTop, aabbClosest);

    // Refine: get the actual closest point on AABB to this segment point
    aabbClosest = box.closestPoint(segClosest);

    glm::vec3 delta = segClosest - aabbClosest;
    float dist = glm::length(delta);

    if (dist >= cap.radius || dist < 1e-6f) {
        // Check if segment point is INSIDE the AABB
        if (segClosest.x >= box.min.x && segClosest.x <= box.max.x &&
            segClosest.y >= box.min.y && segClosest.y <= box.max.y &&
            segClosest.z >= box.min.z && segClosest.z <= box.max.z) {
            // Point is inside — use AABB overlap approach
            glm::vec3 center = box.center();
            glm::vec3 half = box.halfExtents();
            glm::vec3 d = segClosest - center;

            // Find the axis of minimum penetration
            float minPen = 1e30f;
            glm::vec3 normal{0, 1, 0};

            for (int axis = 0; axis < 3; axis++) {
                float pen = half[axis] - std::abs(d[axis]);
                if (pen < minPen) {
                    minPen = pen;
                    normal = glm::vec3(0.0f);
                    normal[axis] = d[axis] >= 0 ? 1.0f : -1.0f;
                }
            }

            c.hit = true;
            c.normal = normal;
            c.penetration = minPen + cap.radius;
            c.point = aabbClosest;
            return c;
        }
        return c;
    }

    c.hit = true;
    c.normal = delta / dist;
    c.penetration = cap.radius - dist;
    c.point = aabbClosest;
    return c;
}

// Ray vs AABB (slab method)
inline RayHit rayVsAabb(const Ray& ray, const AABB& box) {
    RayHit result;
    glm::vec3 invDir = 1.0f / ray.direction;

    glm::vec3 t1 = (box.min - ray.origin) * invDir;
    glm::vec3 t2 = (box.max - ray.origin) * invDir;

    glm::vec3 tmin = glm::min(t1, t2);
    glm::vec3 tmax = glm::max(t1, t2);

    float tNear = std::max({tmin.x, tmin.y, tmin.z});
    float tFar = std::min({tmax.x, tmax.y, tmax.z});

    if (tNear > tFar || tFar < 0.0f) return result;

    float t = tNear >= 0 ? tNear : tFar;
    result.hit = true;
    result.distance = t;
    result.point = ray.pointAt(t);

    // Determine hit normal
    if (tmin.x > tmin.y && tmin.x > tmin.z) {
        result.normal = glm::vec3(ray.direction.x > 0 ? -1.0f : 1.0f, 0, 0);
    } else if (tmin.y > tmin.x && tmin.y > tmin.z) {
        result.normal = glm::vec3(0, ray.direction.y > 0 ? -1.0f : 1.0f, 0);
    } else {
        result.normal = glm::vec3(0, 0, ray.direction.z > 0 ? -1.0f : 1.0f);
    }

    return result;
}

} // namespace CollisionTests
