#pragma once

#include "scene/Scene.h"
#include <memory>
#include <stack>

class SceneManager {
public:
    void push(std::unique_ptr<Scene> scene) {
        scene->onEnter();
        m_scenes.push(std::move(scene));
    }

    void pop() {
        if (!m_scenes.empty()) {
            m_scenes.top()->onExit();
            m_scenes.pop();
        }
    }

    void replace(std::unique_ptr<Scene> scene) {
        pop();
        push(std::move(scene));
    }

    void update(float dt) {
        if (!m_scenes.empty()) m_scenes.top()->update(dt);
    }

    void render(float alpha) {
        if (!m_scenes.empty()) m_scenes.top()->render(alpha);
    }

    void renderUI() {
        if (!m_scenes.empty()) m_scenes.top()->renderUI();
    }

    Scene* current() { return m_scenes.empty() ? nullptr : m_scenes.top().get(); }

    bool empty() const { return m_scenes.empty(); }

private:
    std::stack<std::unique_ptr<Scene>> m_scenes;
};
