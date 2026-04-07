#pragma once

#include "ecs/System.h"
#include <vector>
#include <memory>

class SystemScheduler {
public:
    void addSystem(std::unique_ptr<System> system) {
        m_systems.push_back(std::move(system));
    }

    void updateAll(World& world, float dt) {
        for (auto& sys : m_systems) {
            sys->update(world, dt);
        }
    }

private:
    std::vector<std::unique_ptr<System>> m_systems;
};
