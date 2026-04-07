#pragma once

#include <imgui.h>
#include <algorithm>

class DeathOverlay {
public:
    // Returns true while the animation is still playing
    bool render(float deathTimer) {
        ImGuiIO& io = ImGui::GetIO();

        // Fade timing
        float fadeInEnd = 0.6f;     // fully black by 0.6s
        float textAppear = 0.8f;    // "You Died" appears
        float totalDuration = 2.2f; // total animation length

        if (deathTimer > totalDuration) return false; // animation done

        // Compute fade alpha (0 → 1 over fadeInEnd seconds)
        float alpha = std::min(deathTimer / fadeInEnd, 1.0f);

        // Dark overlay
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("##DeathOverlay", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(ImVec2(0, 0), io.DisplaySize,
                          IM_COL32(5, 0, 0, static_cast<int>(alpha * 220)));
        ImGui::End();

        // "You Died" text
        if (deathTimer > textAppear) {
            float textAlpha = std::min((deathTimer - textAppear) / 0.3f, 1.0f);

            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.45f),
                                     0, ImVec2(0.5f, 0.5f));
            ImGui::Begin("##DeathText", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground |
                ImGuiWindowFlags_AlwaysAutoResize);

            ImGui::SetWindowFontScale(2.5f);
            ImGui::PushStyleColor(ImGuiCol_Text,
                ImVec4(1.0f, 0.15f, 0.1f, textAlpha));
            ImGui::Text("You Died");
            ImGui::PopStyleColor();
            ImGui::SetWindowFontScale(1.0f);

            ImGui::End();
        }

        return true;
    }
};
