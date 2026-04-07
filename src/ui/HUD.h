#pragma once

#include <imgui.h>
#include <cstdio>

struct HUDData {
    float timerElapsed = 0.0f;
    bool timerRunning = false;
    int deathCount = 0;
    int checkpointsCurrent = 0;
    int checkpointsTotal = 0;
    float playerSpeed = 0.0f;
    bool levelComplete = false;
};

class HUD {
public:
    void render(const HUDData& data) {
        // Timer — top center
        {
            ImGuiIO& io = ImGui::GetIO();
            float windowWidth = 200.0f;
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f - windowWidth * 0.5f, 10.0f));
            ImGui::SetNextWindowSize(ImVec2(windowWidth, 0));
            ImGui::Begin("##Timer", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);

            int mins = static_cast<int>(data.timerElapsed) / 60;
            int secs = static_cast<int>(data.timerElapsed) % 60;
            int ms = static_cast<int>((data.timerElapsed - static_cast<int>(data.timerElapsed)) * 100);

            char buf[32];
            std::snprintf(buf, sizeof(buf), "%02d:%02d.%02d", mins, secs, ms);

            // Large centered timer text
            ImGui::PushStyleColor(ImGuiCol_Text, data.levelComplete
                ? ImVec4(1.0f, 1.0f, 0.0f, 1.0f)
                : ImVec4(0.0f, 0.9f, 1.0f, 1.0f));
            float textW = ImGui::CalcTextSize(buf).x;
            ImGui::SetCursorPosX((windowWidth - textW) * 0.5f);
            ImGui::SetWindowFontScale(1.8f);
            ImGui::Text("%s", buf);
            ImGui::SetWindowFontScale(1.0f);
            ImGui::PopStyleColor();

            ImGui::End();
        }

        // Stats — top left
        {
            ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f));
            ImGui::SetNextWindowSize(ImVec2(180, 0));
            ImGui::Begin("##Stats", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.75f, 0.9f, 0.9f));
            ImGui::Text("Deaths: %d", data.deathCount);
            if (data.checkpointsTotal > 0) {
                ImGui::Text("Checkpoint: %d/%d", data.checkpointsCurrent, data.checkpointsTotal);
            }
            ImGui::Text("Speed: %.1f", data.playerSpeed);
            ImGui::PopStyleColor();

            ImGui::End();
        }

        // Controls hint — bottom center
        {
            ImGuiIO& io = ImGui::GetIO();
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y - 30.0f),
                                     0, ImVec2(0.5f, 1.0f));
            ImGui::Begin("##Controls", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs |
                ImGuiWindowFlags_AlwaysAutoResize);

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.45f, 0.6f, 0.6f));
            ImGui::Text("ESC Pause  |  R Restart  |  C Crouch  |  Z Prone");
            ImGui::PopStyleColor();

            ImGui::End();
        }
    }
};
