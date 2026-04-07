#pragma once

// -----------------------------------------------------------------------------
// Settings overlay — shared between MenuScene and GameScene's pause menu.
//
// Renders a single ImGui window with sectioned controls (Display / Audio /
// Gameplay / HUD / Controls) that read and write directly to the Settings
// singleton. The display-mode dropdown and vsync toggle defer their effects to
// an "Apply" callback the host scene wires up — that way Settings stays
// independent of the Window class.
//
// Returns true from render() the frame the user clicks Close so the host can
// flush the settings file and pop the overlay.
// -----------------------------------------------------------------------------

#include <imgui.h>
#include <GLFW/glfw3.h>
#include <functional>
#include <string>

#include "core/Settings.h"
#include "core/Input.h"

class SettingsUI {
public:
    // Optional hook the host scene installs to apply display-mode / vsync /
    // resolution changes (which require touching the Window). When unset, those
    // controls just update the Settings::Data and the apply happens at next
    // launch from main.cpp.
    using ApplyDisplayCb = std::function<void()>;
    static inline ApplyDisplayCb onApplyDisplay;

    // Renders the overlay. Returns true the frame the user closes it.
    static bool render() {
        bool closed = false;
        auto& s = Settings::get();

        ImGuiIO& io = ImGui::GetIO();

        // Dark backdrop so the gameplay behind the overlay reads as "paused"
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("##SettingsBackdrop", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(ImVec2(0, 0), io.DisplaySize, IM_COL32(5, 5, 15, 200));
        ImGui::End();

        // Centered settings window
        float winW = 560.0f;
        float winH = 600.0f;
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
                                 0, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(winW, winH));
        ImGui::Begin("##Settings", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        // Title bar
        ImGui::SetWindowFontScale(1.6f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.9f, 1.0f, 1.0f));
        const char* title = "SETTINGS";
        float titleW = ImGui::CalcTextSize(title).x;
        ImGui::SetCursorPosX((winW - titleW) * 0.5f);
        ImGui::Text("%s", title);
        ImGui::PopStyleColor();
        ImGui::SetWindowFontScale(1.0f);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Scrollable body — leave room for the close/reset button row at the bottom
        ImGui::BeginChild("##SettingsBody", ImVec2(0, winH - 130), false);

        // ---------------- Display ----------------
        if (ImGui::CollapsingHeader("Display", ImGuiTreeNodeFlags_DefaultOpen)) {
            const char* modes[] = { "Windowed", "Fullscreen", "Borderless Windowed" };
            int modeIdx = static_cast<int>(s.displayMode);
            if (ImGui::Combo("Window Mode", &modeIdx, modes, IM_ARRAYSIZE(modes))) {
                s.displayMode = static_cast<Settings::DisplayMode>(modeIdx);
                if (onApplyDisplay) onApplyDisplay();
            }
            if (ImGui::Checkbox("V-Sync", &s.vsync)) {
                if (onApplyDisplay) onApplyDisplay();
            }
        }

        // ---------------- Audio ----------------
        if (ImGui::CollapsingHeader("Audio", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat("Master Volume", &s.masterVolume, 0.0f, 1.0f, "%.2f");
        }

        // ---------------- Gameplay ----------------
        if (ImGui::CollapsingHeader("Gameplay", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat("Field of View", &s.fov, 40.0f, 110.0f, "%.0f");
            ImGui::SliderFloat("Mouse Sensitivity", &s.mouseSens, 0.05f, 1.0f, "%.2f");
            ImGui::Checkbox("Invert Y Axis", &s.invertY);
            ImGui::SliderFloat("Bloom Intensity", &s.bloomIntensity, 0.0f, 2.0f, "%.2f");
        }

        // ---------------- HUD ----------------
        if (ImGui::CollapsingHeader("HUD", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Show FPS Counter", &s.showFps);
            ImGui::Checkbox("Show Controls Hint", &s.showControlsHint);
        }

        // ---------------- Controls ----------------
        if (ImGui::CollapsingHeader("Controls (click a binding then press a key)",
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
            // While capturing, the next valid key press goes to *capturePtr
            keybindRow("Move Forward",  s.keys.moveForward);
            keybindRow("Move Back",     s.keys.moveBack);
            keybindRow("Move Left",     s.keys.moveLeft);
            keybindRow("Move Right",    s.keys.moveRight);
            keybindRow("Jump",          s.keys.jump);
            keybindRow("Crouch",        s.keys.crouch);
            keybindRow("Prone",         s.keys.prone);
            keybindRow("Pause",         s.keys.pause);
            keybindRow("Restart Level", s.keys.restart);
            keybindRow("Toggle Debug",  s.keys.debug);
            keybindRow("Free Cursor",   s.keys.freeCursor);
            ImGui::Spacing();
            if (ImGui::Button("Reset Keybinds to Default")) {
                s.keys = Settings::Keybinds{};
            }
        }

        ImGui::EndChild();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Footer button row — Reset All / Close
        float btnW = 240.0f;
        float btnH = 36.0f;
        float gap = 16.0f;
        float totalW = btnW * 2 + gap;
        ImGui::SetCursorPosX((winW - totalW) * 0.5f);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.1f, 0.1f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.15f, 0.15f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("Reset All to Defaults", ImVec2(btnW, btnH))) {
            // Preserve display mode (would yank the window mid-frame) — every
            // other field goes back to its in-source default.
            auto displayMode = s.displayMode;
            auto vsync = s.vsync;
            int ww = s.windowedWidth;
            int wh = s.windowedHeight;
            s = Settings::Data{};
            s.displayMode = displayMode;
            s.vsync = vsync;
            s.windowedWidth = ww;
            s.windowedHeight = wh;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine(0.0f, gap);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.45f, 0.2f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.65f, 0.3f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.8f, 0.4f, 1.0f));
        if (ImGui::Button("Close", ImVec2(btnW, btnH))) {
            closed = true;
            s_capturing = nullptr;
        }
        ImGui::PopStyleColor(3);

        ImGui::End();

        // Process any pending key capture: scan all keys until one is pressed,
        // then store it. This runs after the window closes so the pressed key
        // doesn't also activate buttons in the same frame.
        if (s_capturing) {
            captureKey();
        }

        return closed;
    }

private:
    // Pointer to the integer that the next key press should overwrite.
    static inline int* s_capturing = nullptr;

    static void keybindRow(const char* label, int& key) {
        std::string buttonText;
        if (s_capturing == &key) {
            buttonText = "[ press a key... ]";
        } else {
            buttonText = Settings::keyName(key);
        }

        // Stable ID per-row so two rows with the same current binding don't
        // collide in ImGui's ID stack.
        ImGui::PushID(label);
        ImGui::Text("%s", label);
        ImGui::SameLine(180.0f);
        if (ImGui::Button(buttonText.c_str(), ImVec2(160.0f, 0))) {
            s_capturing = &key;
        }
        ImGui::PopID();
    }

    // Walks every GLFW key code looking for the first that's currently
    // pressed; assigns it to whatever row is in capture mode. Esc cancels.
    static void captureKey() {
        GLFWwindow* w = Input::window();
        if (!w) { s_capturing = nullptr; return; }

        if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            // Cancel — but if the user *wants* to bind Esc itself we honor it
            // when nothing else is held.
            s_capturing = nullptr;
            return;
        }

        // Sweep the printable + special key range. The list mirrors GLFW's
        // documented key constants.
        static const int keysToCheck[] = {
            GLFW_KEY_SPACE, GLFW_KEY_APOSTROPHE, GLFW_KEY_COMMA, GLFW_KEY_MINUS,
            GLFW_KEY_PERIOD, GLFW_KEY_SLASH, GLFW_KEY_0, GLFW_KEY_1, GLFW_KEY_2,
            GLFW_KEY_3, GLFW_KEY_4, GLFW_KEY_5, GLFW_KEY_6, GLFW_KEY_7,
            GLFW_KEY_8, GLFW_KEY_9, GLFW_KEY_SEMICOLON, GLFW_KEY_EQUAL,
            GLFW_KEY_A, GLFW_KEY_B, GLFW_KEY_C, GLFW_KEY_D, GLFW_KEY_E,
            GLFW_KEY_F, GLFW_KEY_G, GLFW_KEY_H, GLFW_KEY_I, GLFW_KEY_J,
            GLFW_KEY_K, GLFW_KEY_L, GLFW_KEY_M, GLFW_KEY_N, GLFW_KEY_O,
            GLFW_KEY_P, GLFW_KEY_Q, GLFW_KEY_R, GLFW_KEY_S, GLFW_KEY_T,
            GLFW_KEY_U, GLFW_KEY_V, GLFW_KEY_W, GLFW_KEY_X, GLFW_KEY_Y,
            GLFW_KEY_Z, GLFW_KEY_LEFT_BRACKET, GLFW_KEY_BACKSLASH,
            GLFW_KEY_RIGHT_BRACKET, GLFW_KEY_GRAVE_ACCENT, GLFW_KEY_ENTER,
            GLFW_KEY_TAB, GLFW_KEY_BACKSPACE, GLFW_KEY_INSERT, GLFW_KEY_DELETE,
            GLFW_KEY_RIGHT, GLFW_KEY_LEFT, GLFW_KEY_DOWN, GLFW_KEY_UP,
            GLFW_KEY_PAGE_UP, GLFW_KEY_PAGE_DOWN, GLFW_KEY_HOME, GLFW_KEY_END,
            GLFW_KEY_F1, GLFW_KEY_F2, GLFW_KEY_F3, GLFW_KEY_F4, GLFW_KEY_F5,
            GLFW_KEY_F6, GLFW_KEY_F7, GLFW_KEY_F8, GLFW_KEY_F9, GLFW_KEY_F10,
            GLFW_KEY_F11, GLFW_KEY_F12,
            GLFW_KEY_LEFT_SHIFT, GLFW_KEY_LEFT_CONTROL, GLFW_KEY_LEFT_ALT,
            GLFW_KEY_RIGHT_SHIFT, GLFW_KEY_RIGHT_CONTROL, GLFW_KEY_RIGHT_ALT,
        };
        for (int k : keysToCheck) {
            if (glfwGetKey(w, k) == GLFW_PRESS) {
                *s_capturing = k;
                s_capturing = nullptr;
                return;
            }
        }
    }
};
