#pragma once

// ImGui banner for the in-game updater. Drawn from MenuScene::renderUI() so it
// only appears on the main menu — no surprise modal during gameplay.
//
// The banner is silent for Idle / Checking / UpToDate / Failed states so the
// menu stays clean when there's nothing to show.

#include "net/Updater.h"
#include <imgui.h>

namespace UpdateUI {

// Set by applyUpdate() click — main loop watches this and exits cleanly.
inline bool g_quitRequested = false;

inline void render() {
    Updater::Status s = Updater::getStatus();

    // Hidden states — banner is invisible.
    if (s.state == Updater::State::Idle ||
        s.state == Updater::State::Checking ||
        s.state == Updater::State::UpToDate ||
        s.state == Updater::State::Failed) {
        return;
    }

    // Anchor to the bottom-right corner with a 24px margin.
    ImGuiIO& io = ImGui::GetIO();
    const float margin = 24.0f;
    const float bannerW = 360.0f;
    const float bannerH = (s.state == Updater::State::Downloading) ? 110.0f : 130.0f;

    ImGui::SetNextWindowPos(
        ImVec2(io.DisplaySize.x - bannerW - margin, io.DisplaySize.y - bannerH - margin),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(bannerW, bannerH), ImGuiCond_Always);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.03f, 0.05f, 0.10f, 0.92f));
    ImGui::PushStyleColor(ImGuiCol_Border,   ImVec4(0.0f, 0.85f, 1.0f, 0.95f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);

    ImGui::Begin("##UpdateBanner", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoSavedSettings);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.95f, 1.0f, 1.0f));
    ImGui::Text("UPDATE AVAILABLE");
    ImGui::PopStyleColor();
    ImGui::Separator();

    if (s.state == Updater::State::UpdateAvailable) {
        ImGui::TextWrapped("Version %s is ready to install.", s.latestVersion.c_str());
        ImGui::Spacing();
        if (ImGui::Button("Download & Install", ImVec2(-1, 32))) {
            Updater::downloadUpdate();
        }
    }
    else if (s.state == Updater::State::Downloading) {
        ImGui::Text("Downloading %s...", s.latestVersion.c_str());
        ImGui::Spacing();
        ImGui::ProgressBar(s.downloadProgress, ImVec2(-1, 22));
    }
    else if (s.state == Updater::State::ReadyToInstall) {
        ImGui::TextWrapped("Update %s downloaded.", s.latestVersion.c_str());
        ImGui::TextWrapped("The game will close to apply the update.");
        ImGui::Spacing();
        if (ImGui::Button("Restart & Install", ImVec2(-1, 32))) {
            if (Updater::applyUpdate()) {
                g_quitRequested = true;
            }
        }
    }

    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

} // namespace UpdateUI
