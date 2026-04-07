#pragma once

#include <imgui.h>
#include <string>
#include <vector>
#include <cstdio>

enum class MenuAction {
    None,
    Play,
    SelectLevel,
    Settings,
    Quit
};

struct LevelEntry {
    std::string name;
    std::string path;
    bool unlocked = true;
    bool completed = false;
    float bestTime = 0.0f;
    int bestDeaths = -1;
};

class MainMenuUI {
public:
    void setLevels(const std::vector<LevelEntry>& levels) { m_levels = levels; }

    MenuAction render() {
        MenuAction action = MenuAction::None;
        ImGuiIO& io = ImGui::GetIO();

        if (m_showLevelSelect) {
            action = renderLevelSelect();
            return action;
        }

        // Main menu window
        float winW = 400.0f, winH = 440.0f;
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
                                 0, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(winW, winH));
        ImGui::Begin("##MainMenu", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBackground);

        ImGui::Spacing();

        // Title
        ImGui::SetWindowFontScale(2.2f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.9f, 1.0f, 1.0f));
        const char* title = "NEON OBBY";
        float titleW = ImGui::CalcTextSize(title).x;
        ImGui::SetCursorPosX((winW - titleW) * 0.5f);
        ImGui::Text("%s", title);
        ImGui::PopStyleColor();
        ImGui::SetWindowFontScale(1.0f);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.55f, 0.7f, 0.8f));
        const char* sub = "A Neon Obstacle Course";
        float subW = ImGui::CalcTextSize(sub).x;
        ImGui::SetCursorPosX((winW - subW) * 0.5f);
        ImGui::Text("%s", sub);
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();

        float btnW = 280.0f;
        float btnH = 45.0f;
        float btnX = (winW - btnW) * 0.5f;

        // Find first unlocked, uncompleted level for "Play"
        ImGui::SetCursorPosX(btnX);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.45f, 0.2f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.65f, 0.3f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.8f, 0.4f, 1.0f));
        if (ImGui::Button("Play", ImVec2(btnW, btnH))) {
            // Find the first uncompleted level, or the last level
            m_selectedLevel = 0;
            for (size_t i = 0; i < m_levels.size(); i++) {
                if (m_levels[i].unlocked && !m_levels[i].completed) {
                    m_selectedLevel = static_cast<int>(i);
                    break;
                }
                if (m_levels[i].unlocked) {
                    m_selectedLevel = static_cast<int>(i);
                }
            }
            action = MenuAction::SelectLevel;
        }
        ImGui::PopStyleColor(3);

        ImGui::Spacing();
        ImGui::SetCursorPosX(btnX);
        if (ImGui::Button("Level Select", ImVec2(btnW, btnH))) {
            m_showLevelSelect = true;
        }

        ImGui::Spacing();
        ImGui::SetCursorPosX(btnX);
        if (ImGui::Button("Settings", ImVec2(btnW, btnH))) {
            action = MenuAction::Settings;
        }

        ImGui::Spacing();
        ImGui::SetCursorPosX(btnX);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.45f, 0.1f, 0.1f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.65f, 0.15f, 0.15f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("Quit", ImVec2(btnW, btnH))) {
            action = MenuAction::Quit;
        }
        ImGui::PopStyleColor(3);

        ImGui::End();

        return action;
    }

    int selectedLevel() const { return m_selectedLevel; }

private:
    std::vector<LevelEntry> m_levels;
    bool m_showLevelSelect = false;
    int m_selectedLevel = 0;

    MenuAction renderLevelSelect() {
        MenuAction action = MenuAction::None;
        ImGuiIO& io = ImGui::GetIO();

        float winW = 420.0f, winH = 450.0f;
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
                                 0, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(winW, winH));
        ImGui::Begin("##LevelSelect", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        // Title
        ImGui::SetWindowFontScale(1.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.9f, 1.0f, 1.0f));
        const char* title = "SELECT LEVEL";
        float titleW = ImGui::CalcTextSize(title).x;
        ImGui::SetCursorPosX((winW - titleW) * 0.5f);
        ImGui::Text("%s", title);
        ImGui::PopStyleColor();
        ImGui::SetWindowFontScale(1.0f);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        float btnW = winW - 32.0f;
        float btnH = 55.0f;

        for (size_t i = 0; i < m_levels.size(); i++) {
            ImGui::SetCursorPosX(16.0f);

            if (!m_levels[i].unlocked) {
                // Locked level — grayed out
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.12f, 0.15f, 0.6f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.12f, 0.12f, 0.15f, 0.6f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.12f, 0.12f, 0.15f, 0.6f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.35f, 0.4f, 0.7f));

                std::string label = std::to_string(i + 1) + ". " + m_levels[i].name + "  [LOCKED]";
                ImGui::Button(label.c_str(), ImVec2(btnW, btnH));
                ImGui::PopStyleColor(4);
            } else {
                // Unlocked level
                float hue = static_cast<float>(i) * 0.25f;
                ImGui::PushStyleColor(ImGuiCol_Button,
                    ImVec4(0.1f + hue * 0.2f, 0.2f * (1.0f - hue), 0.35f + hue * 0.1f, 0.7f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                    ImVec4(0.15f + hue * 0.25f, 0.3f * (1.0f - hue), 0.5f + hue * 0.15f, 0.85f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                    ImVec4(0.2f + hue * 0.3f, 0.4f * (1.0f - hue), 0.65f + hue * 0.2f, 1.0f));

                std::string label = std::to_string(i + 1) + ". " + m_levels[i].name;
                if (m_levels[i].completed) {
                    int mins = static_cast<int>(m_levels[i].bestTime) / 60;
                    int secs = static_cast<int>(m_levels[i].bestTime) % 60;
                    int ms = static_cast<int>((m_levels[i].bestTime - static_cast<int>(m_levels[i].bestTime)) * 100);
                    char buf[64];
                    std::snprintf(buf, sizeof(buf), "  [%02d:%02d.%02d | %d deaths]",
                                  mins, secs, ms, m_levels[i].bestDeaths);
                    label += buf;
                }

                if (ImGui::Button(label.c_str(), ImVec2(btnW, btnH))) {
                    m_selectedLevel = static_cast<int>(i);
                    action = MenuAction::SelectLevel;
                    m_showLevelSelect = false;
                }
                ImGui::PopStyleColor(3);
            }
            ImGui::Spacing();
        }

        ImGui::Spacing();
        ImGui::SetCursorPosX(16.0f);
        if (ImGui::Button("Back", ImVec2(btnW, 35.0f))) {
            m_showLevelSelect = false;
        }

        ImGui::End();

        return action;
    }
};
