#include "renderer/MeshFactory.h"
#include <glm/glm.hpp>
#include <cmath>

static constexpr float PI = 3.14159265358979323846f;

Mesh MeshFactory::createCube(float size) {
    float h = size * 0.5f;

    std::vector<Vertex> vertices = {
        // Front (+Z)
        {{-h,-h, h},{0,0,1}}, {{ h,-h, h},{0,0,1}}, {{ h, h, h},{0,0,1}}, {{-h, h, h},{0,0,1}},
        // Back (-Z)
        {{ h,-h,-h},{0,0,-1}}, {{-h,-h,-h},{0,0,-1}}, {{-h, h,-h},{0,0,-1}}, {{ h, h,-h},{0,0,-1}},
        // Right (+X)
        {{ h,-h, h},{1,0,0}}, {{ h,-h,-h},{1,0,0}}, {{ h, h,-h},{1,0,0}}, {{ h, h, h},{1,0,0}},
        // Left (-X)
        {{-h,-h,-h},{-1,0,0}}, {{-h,-h, h},{-1,0,0}}, {{-h, h, h},{-1,0,0}}, {{-h, h,-h},{-1,0,0}},
        // Top (+Y)
        {{-h, h, h},{0,1,0}}, {{ h, h, h},{0,1,0}}, {{ h, h,-h},{0,1,0}}, {{-h, h,-h},{0,1,0}},
        // Bottom (-Y)
        {{-h,-h,-h},{0,-1,0}}, {{ h,-h,-h},{0,-1,0}}, {{ h,-h, h},{0,-1,0}}, {{-h,-h, h},{0,-1,0}},
    };

    std::vector<uint32_t> indices = {
         0, 1, 2, 2, 3, 0,   4, 5, 6, 6, 7, 4,
         8, 9,10,10,11, 8,  12,13,14,14,15,12,
        16,17,18,18,19,16,  20,21,22,22,23,20,
    };

    return Mesh(vertices, indices);
}

Mesh MeshFactory::createSphere(float radius, int sectors, int stacks) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    for (int i = 0; i <= stacks; i++) {
        float phi = PI * static_cast<float>(i) / static_cast<float>(stacks);
        float sinP = std::sin(phi), cosP = std::cos(phi);

        for (int j = 0; j <= sectors; j++) {
            float theta = 2.0f * PI * static_cast<float>(j) / static_cast<float>(sectors);
            float sinT = std::sin(theta), cosT = std::cos(theta);

            glm::vec3 n(cosT * sinP, cosP, sinT * sinP);
            vertices.push_back({n * radius, n});
        }
    }

    for (int i = 0; i < stacks; i++) {
        for (int j = 0; j < sectors; j++) {
            uint32_t a = static_cast<uint32_t>(i * (sectors + 1) + j);
            uint32_t b = a + static_cast<uint32_t>(sectors + 1);
            indices.insert(indices.end(), {a, b, a+1, a+1, b, b+1});
        }
    }

    return Mesh(vertices, indices);
}

Mesh MeshFactory::createCylinder(float radius, float height, int segments) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    float halfH = height * 0.5f;

    // Side vertices
    for (int i = 0; i <= segments; i++) {
        float theta = 2.0f * PI * static_cast<float>(i) / static_cast<float>(segments);
        float c = std::cos(theta), s = std::sin(theta);
        glm::vec3 n(c, 0.0f, s);
        vertices.push_back({{c * radius, -halfH, s * radius}, n}); // bottom ring
        vertices.push_back({{c * radius,  halfH, s * radius}, n}); // top ring
    }

    for (int i = 0; i < segments; i++) {
        uint32_t a = static_cast<uint32_t>(i * 2);
        indices.insert(indices.end(), {a, a+2, a+1, a+1, a+2, a+3});
    }

    // Top cap
    uint32_t topCenter = static_cast<uint32_t>(vertices.size());
    vertices.push_back({{0, halfH, 0}, {0, 1, 0}});
    for (int i = 0; i <= segments; i++) {
        float theta = 2.0f * PI * static_cast<float>(i) / static_cast<float>(segments);
        vertices.push_back({{std::cos(theta)*radius, halfH, std::sin(theta)*radius}, {0,1,0}});
    }
    for (int i = 0; i < segments; i++) {
        indices.insert(indices.end(), {topCenter, topCenter+1+static_cast<uint32_t>(i+1), topCenter+1+static_cast<uint32_t>(i)});
    }

    // Bottom cap
    uint32_t botCenter = static_cast<uint32_t>(vertices.size());
    vertices.push_back({{0, -halfH, 0}, {0, -1, 0}});
    for (int i = 0; i <= segments; i++) {
        float theta = 2.0f * PI * static_cast<float>(i) / static_cast<float>(segments);
        vertices.push_back({{std::cos(theta)*radius, -halfH, std::sin(theta)*radius}, {0,-1,0}});
    }
    for (int i = 0; i < segments; i++) {
        indices.insert(indices.end(), {botCenter, botCenter+1+static_cast<uint32_t>(i), botCenter+1+static_cast<uint32_t>(i+1)});
    }

    return Mesh(vertices, indices);
}

Mesh MeshFactory::createCapsule(float radius, float height, int segments, int rings) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    float halfCyl = height * 0.5f;

    // Top hemisphere
    for (int i = 0; i <= rings; i++) {
        float phi = (PI * 0.5f) * static_cast<float>(i) / static_cast<float>(rings);
        float sinP = std::sin(phi), cosP = std::cos(phi);
        for (int j = 0; j <= segments; j++) {
            float theta = 2.0f * PI * static_cast<float>(j) / static_cast<float>(segments);
            glm::vec3 n(std::cos(theta)*sinP, cosP, std::sin(theta)*sinP);
            vertices.push_back({n * radius + glm::vec3(0, halfCyl, 0), n});
        }
    }

    // Cylinder body
    for (int i = 0; i <= 1; i++) {
        float y = halfCyl - static_cast<float>(i) * height;
        for (int j = 0; j <= segments; j++) {
            float theta = 2.0f * PI * static_cast<float>(j) / static_cast<float>(segments);
            float c = std::cos(theta), s = std::sin(theta);
            glm::vec3 n(c, 0, s);
            vertices.push_back({{c * radius, y, s * radius}, n});
        }
    }

    // Bottom hemisphere
    for (int i = 0; i <= rings; i++) {
        float phi = (PI * 0.5f) + (PI * 0.5f) * static_cast<float>(i) / static_cast<float>(rings);
        float sinP = std::sin(phi), cosP = std::cos(phi);
        for (int j = 0; j <= segments; j++) {
            float theta = 2.0f * PI * static_cast<float>(j) / static_cast<float>(segments);
            glm::vec3 n(std::cos(theta)*sinP, cosP, std::sin(theta)*sinP);
            vertices.push_back({n * radius + glm::vec3(0, -halfCyl, 0), n});
        }
    }

    int totalRows = (rings + 1) + 2 + (rings + 1) - 1;
    for (int i = 0; i < totalRows; i++) {
        for (int j = 0; j < segments; j++) {
            uint32_t a = static_cast<uint32_t>(i * (segments + 1) + j);
            uint32_t b = a + static_cast<uint32_t>(segments + 1);
            indices.insert(indices.end(), {a, b, a+1, a+1, b, b+1});
        }
    }

    return Mesh(vertices, indices);
}

Mesh MeshFactory::createPlane(float width, float depth) {
    float hw = width * 0.5f, hd = depth * 0.5f;
    std::vector<Vertex> vertices = {
        {{-hw, 0, -hd}, {0, 1, 0}},
        {{ hw, 0, -hd}, {0, 1, 0}},
        {{ hw, 0,  hd}, {0, 1, 0}},
        {{-hw, 0,  hd}, {0, 1, 0}},
    };
    std::vector<uint32_t> indices = {0, 2, 1, 0, 3, 2};
    return Mesh(vertices, indices);
}

Mesh MeshFactory::createWedge(float size) {
    float h = size * 0.5f;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // Slope normal
    glm::vec3 slopeN = glm::normalize(glm::vec3(0.0f, 1.0f, 1.0f));

    // Bottom face
    uint32_t b = 0;
    vertices.push_back({{-h, -h, -h}, {0,-1,0}});
    vertices.push_back({{ h, -h, -h}, {0,-1,0}});
    vertices.push_back({{ h, -h,  h}, {0,-1,0}});
    vertices.push_back({{-h, -h,  h}, {0,-1,0}});
    indices.insert(indices.end(), {b, b+1, b+2, b, b+2, b+3});

    // Back face (-Z, the tall vertical face)
    b = static_cast<uint32_t>(vertices.size());
    vertices.push_back({{ h, -h, -h}, {0,0,-1}});
    vertices.push_back({{-h, -h, -h}, {0,0,-1}});
    vertices.push_back({{-h,  h, -h}, {0,0,-1}});
    vertices.push_back({{ h,  h, -h}, {0,0,-1}});
    indices.insert(indices.end(), {b, b+1, b+2, b, b+2, b+3});

    // Slope face
    b = static_cast<uint32_t>(vertices.size());
    vertices.push_back({{-h, -h,  h}, slopeN});
    vertices.push_back({{ h, -h,  h}, slopeN});
    vertices.push_back({{ h,  h, -h}, slopeN});
    vertices.push_back({{-h,  h, -h}, slopeN});
    indices.insert(indices.end(), {b, b+1, b+2, b, b+2, b+3});

    // Left face (-X, triangle)
    b = static_cast<uint32_t>(vertices.size());
    vertices.push_back({{-h, -h, -h}, {-1,0,0}});
    vertices.push_back({{-h, -h,  h}, {-1,0,0}});
    vertices.push_back({{-h,  h, -h}, {-1,0,0}});
    indices.insert(indices.end(), {b, b+1, b+2});

    // Right face (+X, triangle)
    b = static_cast<uint32_t>(vertices.size());
    vertices.push_back({{ h, -h,  h}, {1,0,0}});
    vertices.push_back({{ h, -h, -h}, {1,0,0}});
    vertices.push_back({{ h,  h, -h}, {1,0,0}});
    indices.insert(indices.end(), {b, b+1, b+2});

    return Mesh(vertices, indices);
}
