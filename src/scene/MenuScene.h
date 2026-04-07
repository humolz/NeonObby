#pragma once

#include "scene/Scene.h"
#include "ui/MainMenuUI.h"
#include "ui/UpdateNotification.h"
#include "save/SaveManager.h"
#include "renderer/Shader.h"
#include "renderer/ScreenQuad.h"
#include "core/Paths.h"
#include <glad/glad.h>
#include <string>
#include <vector>
#include <functional>

class MenuScene : public Scene {
public:
    using LaunchCallback = std::function<void(int levelIndex)>;

    MenuScene(Shader& skyboxShader, SaveManager& saveManager, LaunchCallback onLaunch)
        : m_skyboxShader(skyboxShader), m_saveManager(saveManager),
          m_onLaunch(std::move(onLaunch)) {}

    void onEnter() override {
        rebuildLevelList();
    }

    void update(float) override {}

    void render(float) override {
        m_skyboxShader.bind();
        glDepthMask(GL_FALSE);
        m_skyQuad.draw();
        glDepthMask(GL_TRUE);
    }

    void renderUI() override {
        MenuAction action = m_menuUI.render();

        // Auto-update banner — silent unless a newer version was published.
        UpdateUI::render();

        switch (action) {
        case MenuAction::Play:
        case MenuAction::SelectLevel: {
            int idx = m_menuUI.selectedLevel();
            if (m_onLaunch) m_onLaunch(idx);
            break;
        }
        case MenuAction::Quit:
            m_pendingAction = SceneAction::Quit;
            break;
        default:
            break;
        }
    }

private:
    Shader& m_skyboxShader;
    SaveManager& m_saveManager;
    ScreenQuad m_skyQuad;
    MainMenuUI m_menuUI;
    LaunchCallback m_onLaunch;

    void rebuildLevelList() {
        std::string assetsDir = Paths::getAssetPath("");
        const auto& save = m_saveManager.data();

        std::vector<LevelEntry> levels = {
            {"First Steps",    assetsDir + "/levels/level_01.json"},
            {"Neon Gauntlet",  assetsDir + "/levels/level_02.json"},
            {"Neon Nightmare", assetsDir + "/levels/level_03.json"},
        };

        for (size_t i = 0; i < levels.size(); i++) {
            levels[i].unlocked = save.isUnlocked(static_cast<int>(i));
            if (i < save.levelsCompleted.size()) {
                levels[i].completed = save.levelsCompleted[i];
                levels[i].bestTime = save.bestTimes[i];
                levels[i].bestDeaths = save.bestDeaths[i];
            }
        }

        m_menuUI.setLevels(levels);
    }
};
