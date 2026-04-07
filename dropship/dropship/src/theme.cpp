#include "pch.h"

#define IMGUI_DEFINE_MATH_OPERATORS // https://github.com/ocornut/imgui/issues/2832

# include "imgui-docking/imgui.h"
# include "theme.h"

void setTheme(THEME theme)
{
    if (theme == dark)
    {
        ImGui::StyleColorsDark();
    }
    else
    {
        ImGui::StyleColorsLight();
    }

    // spacing and padding is not overriden by changing colors

    auto io = ImGui::GetIO();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowBorderSize = 0.0f;
    style.PopupBorderSize = 0.0f;
    style.ChildBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;

    style.WindowRounding = io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable ? 22.0f : 18.0f;
    style.PopupRounding = 18.0f;
    style.ChildRounding = 18.0f;
    style.FrameRounding = 14.0f;
    style.GrabRounding = 14.0f;
    style.ScrollbarRounding = 14.0f;
    style.TabRounding = 14.0f;
    style.SelectableRounding = 14.0f;

    style.WindowPadding = { 20.0f, 18.0f };
    style.WindowTitleAlign = { 0.0f, 0.5f };
    style.FramePadding = { 14.0f, 10.0f };
    style.ItemSpacing = { 12.0f, 12.0f };
    style.ItemInnerSpacing = { 8.0f, 6.0f };
    style.CellPadding = { 10.0f, 8.0f };
    style.IndentSpacing = 20.0f;
    style.GrabMinSize = 10.0f;
    style.ScrollbarSize = 12.0f;
    style.DisabledAlpha = 0.55f;

    if (theme == dark)
    {
        style.Colors[ImGuiCol_ModalWindowDimBg] = { 0.0f, 0.0f, 0.0f, 0.90f };
    }
    else if (theme == light)
    {
        style.Colors[ImGuiCol_Text] = { 0.12f, 0.16f, 0.22f, 1.0f };
        style.Colors[ImGuiCol_TextDisabled] = { 0.45f, 0.50f, 0.58f, 1.0f };
        style.Colors[ImGuiCol_WindowBg] = { 0.97f, 0.98f, 0.99f, 0.96f };
        style.Colors[ImGuiCol_ChildBg] = { 0.93f, 0.95f, 0.98f, 0.88f };
        style.Colors[ImGuiCol_PopupBg] = { 0.98f, 0.99f, 1.00f, 0.98f };
        style.Colors[ImGuiCol_Border] = { 0.82f, 0.87f, 0.92f, 0.70f };
        style.Colors[ImGuiCol_BorderShadow] = { 0.00f, 0.00f, 0.00f, 0.00f };
        style.Colors[ImGuiCol_FrameBg] = { 0.10f, 0.15f, 0.21f, 0.05f };
        style.Colors[ImGuiCol_FrameBgHovered] = { 0.10f, 0.15f, 0.21f, 0.08f };
        style.Colors[ImGuiCol_FrameBgActive] = { 0.10f, 0.15f, 0.21f, 0.11f };
        style.Colors[ImGuiCol_TitleBg] = { 0.97f, 0.98f, 0.99f, 0.94f };
        style.Colors[ImGuiCol_TitleBgActive] = { 0.97f, 0.98f, 0.99f, 0.98f };
        style.Colors[ImGuiCol_ScrollbarBg] = { 0.00f, 0.00f, 0.00f, 0.00f };
        style.Colors[ImGuiCol_ScrollbarGrab] = { 0.52f, 0.59f, 0.69f, 0.35f };
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = { 0.52f, 0.59f, 0.69f, 0.48f };
        style.Colors[ImGuiCol_ScrollbarGrabActive] = { 0.52f, 0.59f, 0.69f, 0.60f };
        style.Colors[ImGuiCol_CheckMark] = { 0.14f, 0.45f, 0.93f, 1.0f };
        style.Colors[ImGuiCol_SliderGrab] = { 0.14f, 0.45f, 0.93f, 0.78f };
        style.Colors[ImGuiCol_SliderGrabActive] = { 0.14f, 0.45f, 0.93f, 1.0f };
        style.Colors[ImGuiCol_Button] = { 0.10f, 0.15f, 0.21f, 0.05f };
        style.Colors[ImGuiCol_ButtonHovered] = { 0.10f, 0.15f, 0.21f, 0.09f };
        style.Colors[ImGuiCol_ButtonActive] = { 0.10f, 0.15f, 0.21f, 0.14f };
        style.Colors[ImGuiCol_Header] = { 0.14f, 0.45f, 0.93f, 0.12f };
        style.Colors[ImGuiCol_HeaderHovered] = { 0.14f, 0.45f, 0.93f, 0.18f };
        style.Colors[ImGuiCol_HeaderActive] = { 0.14f, 0.45f, 0.93f, 0.24f };
        style.Colors[ImGuiCol_Separator] = { 0.80f, 0.85f, 0.90f, 0.85f };
        style.Colors[ImGuiCol_SeparatorHovered] = { 0.14f, 0.45f, 0.93f, 0.35f };
        style.Colors[ImGuiCol_SeparatorActive] = { 0.14f, 0.45f, 0.93f, 0.50f };
        style.Colors[ImGuiCol_ResizeGrip] = { 0.14f, 0.45f, 0.93f, 0.18f };
        style.Colors[ImGuiCol_ResizeGripHovered] = { 0.14f, 0.45f, 0.93f, 0.34f };
        style.Colors[ImGuiCol_ResizeGripActive] = { 0.14f, 0.45f, 0.93f, 0.48f };
        style.Colors[ImGuiCol_Tab] = { 0.10f, 0.15f, 0.21f, 0.04f };
        style.Colors[ImGuiCol_TabHovered] = { 0.14f, 0.45f, 0.93f, 0.18f };
        style.Colors[ImGuiCol_TabActive] = { 0.14f, 0.45f, 0.93f, 0.12f };
        style.Colors[ImGuiCol_NavHighlight] = { 0.14f, 0.45f, 0.93f, 0.75f };
        style.Colors[ImGuiCol_DockingPreview] = { 0.14f, 0.45f, 0.93f, 0.20f };
        style.Colors[ImGuiCol_ModalWindowDimBg] = { 0.08f, 0.10f, 0.16f, 0.28f };
    }
}

void ToggleButton(const char* str_id, bool* v)
{
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    float height = ImGui::GetFrameHeight();
    float width = height * 1.55f;
    float radius = height * 0.50f;

    ImGui::InvisibleButton(str_id, ImVec2(width, height));
    if (ImGui::IsItemClicked())
        *v = !*v;

    float t = *v ? 1.0f : 0.0f;

    ImGuiContext& g = *GImGui;
    float ANIM_SPEED = 0.08f;
    if (g.LastActiveId == g.CurrentWindow->GetID(str_id))// && g.LastActiveIdTimer < ANIM_SPEED)
    {
        float t_anim = ImSaturate(g.LastActiveIdTimer / ANIM_SPEED);
        t = *v ? (t_anim) : (1.0f - t_anim);
    }

    static const auto color_toggle_background = ImColor::HSV(fmod(-0.02f, 1.0f) / 14.0f, 0.4f, 1.0f, 1.0f);


    draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), color_toggle_background, height * 0.5f);
    draw_list->AddCircleFilled(ImVec2(p.x + radius + t * (width - radius * 2.0f), p.y + radius), radius - 1.5f, IM_COL32(255, 255, 255, 255));
}

