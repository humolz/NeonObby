#pragma once

#include "renderer/Mesh.h"
#include "renderer/MeshFactory.h"
#include "components/MeshComponent.h"
#include <unordered_map>
#include <memory>

class MeshCache {
public:
    void init() {
        m_meshes[MeshType::Cube]     = std::make_unique<Mesh>(MeshFactory::createCube());
        m_meshes[MeshType::Sphere]   = std::make_unique<Mesh>(MeshFactory::createSphere());
        m_meshes[MeshType::Cylinder] = std::make_unique<Mesh>(MeshFactory::createCylinder());
        m_meshes[MeshType::Capsule]  = std::make_unique<Mesh>(MeshFactory::createCapsule());
        m_meshes[MeshType::Plane]    = std::make_unique<Mesh>(MeshFactory::createPlane());
        m_meshes[MeshType::Wedge]    = std::make_unique<Mesh>(MeshFactory::createWedge());
    }

    const Mesh& get(MeshType type) const {
        return *m_meshes.at(type);
    }

private:
    std::unordered_map<MeshType, std::unique_ptr<Mesh>> m_meshes;
};
