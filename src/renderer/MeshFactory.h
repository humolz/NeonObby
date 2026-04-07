#pragma once

#include "renderer/Mesh.h"

class MeshFactory {
public:
    static Mesh createCube(float size = 1.0f);
    static Mesh createSphere(float radius = 0.5f, int sectors = 24, int stacks = 12);
    static Mesh createCylinder(float radius = 0.5f, float height = 1.0f, int segments = 24);
    static Mesh createCapsule(float radius = 0.3f, float height = 1.0f, int segments = 24, int rings = 8);
    static Mesh createPlane(float width = 10.0f, float depth = 10.0f);
    static Mesh createWedge(float size = 1.0f);
};
