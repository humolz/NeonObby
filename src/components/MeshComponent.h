#pragma once

#include <cstdint>

enum class MeshType : uint8_t {
    Cube,
    Sphere,
    Cylinder,
    Capsule,
    Plane,
    Wedge
};

struct MeshComponent {
    MeshType type = MeshType::Cube;
};
