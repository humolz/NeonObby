#pragma once

#include <imgui.h>
#include <cstdio>

enum class LevelCompleteAction {
    None,
    NextLevel,
    Restart,
    MainMenu
};

class LevelCompleteUI {
public:
    LevelCompleteAction render(float elapsed, int deaths, bool hasNextLevel) {
        LevelCompleteAction action = LevelCompleteAction::None;

        ImGuiIO& io = ImGui::GetIO();

        // Semi-transparent overlay
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("##CompleteOverlay", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(ImVec2(0, 0), io.DisplaySize, IM_COL32(5, 5, 15, 160));
        ImGui::End();

        // Complete window
        float winW = 350.0f, winH = 350.0f;
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
                                 0, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(winW, winH));
        ImGui::Begin("##Complete", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        // Title
        ImGui::SetWindowFontScale(1.6f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
        const char* title = "LEVEL COMPLETE!";
        float titleW = ImGui::CalcTextSize(title).x;
        ImGui::SetCursorPosX((winW - titleW) * 0.5f);
        ImGui::Text("%s", title);
        ImGui::PopStyleColor();
        ImGui::SetWindowFontScale(1.0f);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Stats
        int mins = static_cast<int>(elapsed) / 60;
        int secs = static_cast<int>(elapsed) % 60;
        int ms = static_cast<int>((elapsed - static_cast<int>(elapsed)) * 100);
        char timeBuf[32];
        std::snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d.%02d", mins, secs, ms);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.9f, 1.0f, 1.0f));
        ImGui::SetWindowFontScale(1.3f);

        float timeTextW = ImGui::CalcTextSize(timeBuf).x;
        ImGui::SetCursorPosX((winW - timeTextW) * 0.5f);
        ImGui::Text("%s", timeBuf);

        char deathBuf[32];
        std::snprintf(deathBuf, sizeof(deathBuf), "Deaths: %d", deaths);
        float deathTextW = ImGui::CalcTextSize(deathBuf).x;
        ImGui::SetCursorPosX((winW - deathTextW) * 0.5f);
        ImGui::Text("%s", deathBuf);

        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::Spacing();

        float btnW = winW - 32.0f;
        float btnH = 40.0f;

        if (hasNextLevel) {
            ImGui::SetCursorPosX(16.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.45f, 0.2f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.65f, 0.3f, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.8f, 0.4f, 1.0f));
            if (ImGui::Button("Next Level", ImVec2(btnW, btnH))) {
                action = LevelCompleteAction::NextLevel;
            }
            ImGui::PopStyleColor(3);
            ImGui::Spacing();
        }

        ImGui::SetCursorPosX(16.0f);
        if (ImGui::Button("Restart", ImVec2(btnW, btnH))) {
            action = LevelCompleteAction::Restart;
        }

        ImGui::Spacing();
        ImGui::SetCursorPosX(16.0f);
        if (ImGui::Button("Main Menu", ImVec2(btnW, btnH))) {
            action = LevelCompleteAction::MainMenu;
        }

        ImGui::End();

        return action;
    }
};
