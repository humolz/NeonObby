#include "core/Window.h"
#include "core/Timer.h"
#include "core/Input.h"
#include "core/Paths.h"
#include "renderer/Shader.h"
#include "renderer/MeshCache.h"
#include "renderer/PostProcess.h"
#include "renderer/Model.h"
#include "renderer/ModelLoader.h"
#include "scene/SceneManager.h"
#include "scene/GameScene.h"
#include "scene/MenuScene.h"
#include "ui/UIManager.h"
#include "save/SaveManager.h"
#include "audio/AudioEngine.h"
#include "net/Updater.h"
#include "ui/UpdateNotification.h"

#include <string>
#include <vector>

// Deferred transition to avoid destroying scenes mid-frame
enum class PendingTransition { None, GoToMenu, LaunchLevel };

int main() {
    Window window(1280, 720, "NeonObby - Phase 7");
    Input::init(window.handle());

    std::string assetsDir = Paths::getAssetPath("");
    Shader neonShader(assetsDir + "/shaders/neon.vert", assetsDir + "/shaders/neon.frag");
    Shader skyboxShader(assetsDir + "/shaders/skybox.vert", assetsDir + "/shaders/skybox.frag");
    Shader particleShader(assetsDir + "/shaders/particle.vert", assetsDir + "/shaders/particle.frag");
    Shader debugShader(assetsDir + "/shaders/debug.vert", assetsDir + "/shaders/debug.frag");
    Shader skinnedShader(assetsDir + "/shaders/skinned.vert", assetsDir + "/shaders/neon.frag");
    MeshCache meshCache;
    meshCache.init();

    // Load player character model
    auto playerModel = ModelLoader::load(assetsDir + "/models/pixeldude.glb");

    PostProcess postProcess;
    postProcess.init(window.width(), window.height(), assetsDir);

    UIManager ui;
    ui.init(window.handle());

    // Audio
    AudioEngine audio;
    audio.init();

    // Level paths
    std::vector<std::string> levelPaths = {
        assetsDir + "/levels/level_01.json",
        assetsDir + "/levels/level_02.json",
        assetsDir + "/levels/level_03.json",
    };
    int totalLevels = static_cast<int>(levelPaths.size());

    // Save system
    SaveManager saveManager;
    saveManager.init(totalLevels);

    SceneManager scenes;
    GameScene* activeGameScene = nullptr;
    bool cursorCaptured = false;

    // Deferred transition state
    PendingTransition pendingTransition = PendingTransition::None;
    int pendingLevelIndex = 0;

    auto setCursorCaptured = [&](bool captured) {
        cursorCaptured = captured;
        glfwSetInputMode(window.handle(), GLFW_CURSOR,
            captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    };

    // These now just set the pending transition — actual work happens at frame start
    auto requestMenu = [&]() {
        pendingTransition = PendingTransition::GoToMenu;
    };

    auto requestLevel = [&](int idx) {
        pendingLevelIndex = idx;
        pendingTransition = PendingTransition::LaunchLevel;
    };

    // Actual transition functions (called at safe point in main loop)
    auto doGoToMenu = [&]() {
        activeGameScene = nullptr;
        setCursorCaptured(false);
        while (!scenes.empty()) scenes.pop();
        auto menu = std::make_unique<MenuScene>(skyboxShader, saveManager, [&](int idx) {
            requestLevel(idx);
        });
        scenes.push(std::move(menu));
    };

    auto doLaunchLevel = [&](int levelIndex) {
        if (levelIndex < 0 || levelIndex >= totalLevels) return;
        if (!saveManager.data().isUnlocked(levelIndex)) return;

        while (!scenes.empty()) scenes.pop();

        auto game = std::make_unique<GameScene>(neonShader, skyboxShader, particleShader,
                                                 debugShader, skinnedShader, meshCache,
                                                 audio, playerModel.get());
        game->setLevelPath(levelPaths[levelIndex]);
        game->setLevelIndex(levelIndex);
        game->setTotalLevels(totalLevels);
        game->setAspect(window.aspectRatio());
        game->setOnMainMenu(requestMenu);
        game->setOnNextLevel(requestLevel);
        game->setOnComplete([&](int idx, float time, int deaths) {
            saveManager.onLevelComplete(idx, time, deaths);
        });

        activeGameScene = game.get();
        scenes.push(std::move(game));
        setCursorCaptured(true);
    };

    window.setResizeCallback([&](int w, int h) {
        if (h > 0) {
            if (activeGameScene) {
                activeGameScene->setAspect(static_cast<float>(w) / static_cast<float>(h));
            }
            postProcess.resize(w, h);
        }
    });

    // Start at main menu
    doGoToMenu();

    // Kick off non-blocking version check against the GitHub Releases API.
    // The update banner only appears if a newer version is published — until
    // then this is silent. The worker thread runs to completion on its own.
    Updater::checkForUpdate();

    Timer timer;
    bool tabWasDown = false;

    while (!window.shouldClose()) {
        // Process deferred transitions at the start of frame (safe point)
        if (pendingTransition != PendingTransition::None) {
            PendingTransition action = pendingTransition;
            int lvl = pendingLevelIndex;
            pendingTransition = PendingTransition::None;

            if (action == PendingTransition::GoToMenu) {
                doGoToMenu();
            } else if (action == PendingTransition::LaunchLevel) {
                doLaunchLevel(lvl);
            }
        }

        Input::update();
        timer.tick();
        float dt = timer.deltaTime();

        // Check if current scene wants to quit
        Scene* current = scenes.current();
        if (current && current->pendingAction() == SceneAction::Quit) {
            break;
        }

        // Auto-updater requested a clean shutdown so the installer can replace
        // files. The installer was already launched silently — just exit.
        if (UpdateUI::g_quitRequested) {
            break;
        }


        // Cursor management for game scenes
        if (activeGameScene) {
            bool uiWantsCursor = activeGameScene->wantsCursor();
            if (uiWantsCursor && cursorCaptured) {
                setCursorCaptured(false);
            } else if (!uiWantsCursor && !cursorCaptured) {
                setCursorCaptured(true);
            }

            if (Input::keyDown(GLFW_KEY_TAB)) {
                if (!tabWasDown) {
                    setCursorCaptured(!cursorCaptured);
                }
                tabWasDown = true;
            } else {
                tabWasDown = false;
            }

            if (cursorCaptured && !activeGameScene->isPaused() && !activeGameScene->isLevelComplete()) {
                glm::vec2 md = Input::mouseDelta();
                activeGameScene->camera().processInput(md.x, md.y, Input::scrollDelta());
            }
        }

        scenes.update(dt);

        postProcess.beginScene();
        scenes.render(0.0f);
        postProcess.endScene(window.width(), window.height());

        // Debug renderer (after post-process, directly to screen)
        if (activeGameScene && activeGameScene->debugEnabled()) {
            activeGameScene->renderDebug(window.aspectRatio());
        }

        ui.beginFrame();
        scenes.renderUI();
        ui.endFrame();

        window.swapBuffers();

        int fps = timer.fps();
        if (fps > 0) {
            std::string title = "NeonObby - Phase 7 | FPS: " + std::to_string(fps);
            glfwSetWindowTitle(window.handle(), title.c_str());
        }
    }

    Updater::shutdown();
    audio.shutdown();
    ui.shutdown();
    return 0;
}
