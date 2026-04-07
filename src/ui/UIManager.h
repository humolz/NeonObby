#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

class UIManager {
public:
    void init(GLFWwindow* window) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        applyNeonTheme();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 410");
    }

    void shutdown() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void beginFrame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void endFrame() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

private:
    void applyNeonTheme() {
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 8.0f;
        style.FrameRounding = 4.0f;
        style.GrabRounding = 4.0f;
        style.WindowBorderSize = 1.0f;
        style.FrameBorderSize = 1.0f;
        style.WindowPadding = ImVec2(16, 16);
        style.FramePadding = ImVec2(8, 6);
        style.ItemSpacing = ImVec2(10, 10);

        ImVec4* c = style.Colors;

        // Dark background
        c[ImGuiCol_WindowBg]        = ImVec4(0.05f, 0.05f, 0.10f, 0.92f);
        c[ImGuiCol_PopupBg]         = ImVec4(0.06f, 0.06f, 0.12f, 0.95f);
        c[ImGuiCol_Border]          = ImVec4(0.0f, 0.6f, 1.0f, 0.4f);

        // Text
        c[ImGuiCol_Text]            = ImVec4(0.85f, 0.90f, 1.0f, 1.0f);
        c[ImGuiCol_TextDisabled]    = ImVec4(0.35f, 0.40f, 0.50f, 1.0f);

        // Headers
        c[ImGuiCol_Header]          = ImVec4(0.0f, 0.4f, 0.7f, 0.4f);
        c[ImGuiCol_HeaderHovered]   = ImVec4(0.0f, 0.5f, 1.0f, 0.5f);
        c[ImGuiCol_HeaderActive]    = ImVec4(0.0f, 0.6f, 1.0f, 0.6f);

        // Buttons — cyan neon
        c[ImGuiCol_Button]          = ImVec4(0.0f, 0.35f, 0.55f, 0.7f);
        c[ImGuiCol_ButtonHovered]   = ImVec4(0.0f, 0.55f, 0.85f, 0.85f);
        c[ImGuiCol_ButtonActive]    = ImVec4(0.0f, 0.70f, 1.0f, 1.0f);

        // Frame (input fields)
        c[ImGuiCol_FrameBg]         = ImVec4(0.06f, 0.08f, 0.15f, 0.8f);
        c[ImGuiCol_FrameBgHovered]  = ImVec4(0.08f, 0.12f, 0.22f, 0.9f);
        c[ImGuiCol_FrameBgActive]   = ImVec4(0.0f, 0.3f, 0.5f, 0.9f);

        // Title
        c[ImGuiCol_TitleBg]         = ImVec4(0.04f, 0.04f, 0.08f, 1.0f);
        c[ImGuiCol_TitleBgActive]   = ImVec4(0.0f, 0.25f, 0.45f, 1.0f);
        c[ImGuiCol_TitleBgCollapsed]= ImVec4(0.02f, 0.02f, 0.06f, 0.8f);

        // Scrollbar
        c[ImGuiCol_ScrollbarBg]     = ImVec4(0.03f, 0.03f, 0.06f, 0.8f);
        c[ImGuiCol_ScrollbarGrab]   = ImVec4(0.0f, 0.4f, 0.6f, 0.6f);

        // Separator
        c[ImGuiCol_Separator]       = ImVec4(0.0f, 0.5f, 0.8f, 0.3f);

        // Scale up font
        ImGuiIO& io = ImGui::GetIO();
        io.FontGlobalScale = 1.2f;
    }
};
