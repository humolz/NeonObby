#pragma once

#include "ecs/World.h"

enum class SceneAction {
    None,
    Pop,
    Quit
};

class Scene {
public:
    virtual ~Scene() = default;
    virtual void onEnter() = 0;
    virtual void onExit() {}
    virtual void update(float dt) = 0;
    virtual void render(float alpha) = 0;
    virtual void renderUI() {}

    World& world() { return m_world; }

    SceneAction pendingAction() const { return m_pendingAction; }
    void clearAction() { m_pendingAction = SceneAction::None; }

protected:
    World m_world;
    SceneAction m_pendingAction = SceneAction::None;
};
