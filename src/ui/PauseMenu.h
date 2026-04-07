#pragma once

#include <imgui.h>

enum class PauseAction {
    None,
    Resume,
    Restart,
    Settings,
    MainMenu
};

class PauseMenu {
public:
    PauseAction render() {
        PauseAction action = PauseAction::None;

        ImGuiIO& io = ImGui::GetIO();

        // Semi-transparent dark overlay
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("##PauseOverlay", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(ImVec2(0, 0), io.DisplaySize, IM_COL32(5, 5, 15, 180));
        ImGui::End();

        // Pause window — sized to fit 4 buttons (Resume / Restart / Settings / Main Menu)
        float winW = 300.0f, winH = 340.0f;
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
                                 0, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(winW, winH));
        ImGui::Begin("##Pause", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        // Title
        ImGui::SetWindowFontScale(1.6f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.9f, 1.0f, 1.0f));
        float titleW = ImGui::CalcTextSize("PAUSED").x;
        ImGui::SetCursorPosX((winW - titleW) * 0.5f);
        ImGui::Text("PAUSED");
        ImGui::PopStyleColor();
        ImGui::SetWindowFontScale(1.0f);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Spacing();

        float btnW = winW - 32.0f;
        float btnH = 40.0f;

        ImGui::SetCursorPosX(16.0f);
        if (ImGui::Button("Resume", ImVec2(btnW, btnH))) {
            action = PauseAction::Resume;
        }

        ImGui::Spacing();
        ImGui::SetCursorPosX(16.0f);
        if (ImGui::Button("Restart Level", ImVec2(btnW, btnH))) {
            action = PauseAction::Restart;
        }

        ImGui::Spacing();
        ImGui::SetCursorPosX(16.0f);
        if (ImGui::Button("Settings", ImVec2(btnW, btnH))) {
            action = PauseAction::Settings;
        }

        ImGui::Spacing();
        ImGui::SetCursorPosX(16.0f);
        if (ImGui::Button("Main Menu", ImVec2(btnW, btnH))) {
            action = PauseAction::MainMenu;
        }

        ImGui::End();

        return action;
    }
};
