#include "pch.h"

#include "Dashboard.h"


extern std::unique_ptr<std::vector<std::shared_ptr<Endpoint2>>> g_endpoints;
extern std::unique_ptr<Settings> g_settings;
extern std::unique_ptr<core::tunneling::Tunneling> g_tunneling;
extern std::unique_ptr<util::watcher::window::WindowWatcher> g_window_watcher;
extern std::unique_ptr<Updater> g_updater;

#include "images.h"

extern ImFont* font_title;
extern ImFont* font_subtitle;
extern ImFont* font_text;

namespace {
    using EndpointRef = std::shared_ptr<Endpoint2>;

    struct EndpointGroup {
        const char* title;
        std::vector<const char*> endpoints;
    };

    EndpointRef findEndpoint(const char* title) {
        for (auto& endpoint : (*g_endpoints)) {
            if ((*endpoint).getTitle() == title) {
                return endpoint;
            }
        }
        return nullptr;
    }

    void syncEndpointState(const EndpointRef& endpoint) {
        if ((*endpoint).getBlockDesired()) {
            (*g_settings).addBlockedEndpoint((*endpoint).getTitle());
        }
        else {
            (*g_settings).removeBlockedEndpoint((*endpoint).getTitle());
        }
    }
}


void renderBackground() {
    ImVec2 const windowPos = ImGui::GetWindowPos();
    ImVec2 const windowSize = ImGui::GetWindowSize();
    ImDrawList* bg_list = ImGui::GetBackgroundDrawList();
    static const float rounding = 24.0f;
    const float hero_height = std::min(windowSize.y, 188.0f);

    static const ImU32 shadow_far = ImGui::ColorConvertFloat4ToU32({ 0.06f, 0.10f, 0.18f, 0.10f });
    static const ImU32 shadow_near = ImGui::ColorConvertFloat4ToU32({ 0.06f, 0.10f, 0.18f, 0.06f });
    static const ImU32 shell = ImGui::ColorConvertFloat4ToU32({ 0.98f, 0.99f, 1.00f, 0.95f });
    static const ImU32 border = ImGui::ColorConvertFloat4ToU32({ 0.82f, 0.87f, 0.92f, 0.70f });
    static const ImU32 hero_overlay_top = ImGui::ColorConvertFloat4ToU32({ 0.83f, 0.90f, 0.99f, 0.68f });
    static const ImU32 hero_overlay_bottom = ImGui::ColorConvertFloat4ToU32({ 0.96f, 0.97f, 1.00f, 0.24f });
    static const ImU32 body = ImGui::ColorConvertFloat4ToU32({ 0.99f, 0.99f, 1.00f, 0.90f });
    static const ImU32 hero_image_tint = ImGui::ColorConvertFloat4ToU32({ 1.00f, 1.00f, 1.00f, 0.72f });

    bg_list->AddRectFilled(windowPos + ImVec2(0, 18), windowPos + windowSize + ImVec2(0, 18), shadow_far, rounding);
    bg_list->AddRectFilled(windowPos + ImVec2(0, 9), windowPos + windowSize + ImVec2(0, 9), shadow_near, rounding);
    bg_list->AddRectFilled(windowPos, windowPos + windowSize, shell, rounding);
    bg_list->AddImageRounded(_get_texture("background_app"), windowPos, windowPos + ImVec2(windowSize.x, hero_height + 46.0f), ImVec2(0, 0), ImVec2(1, 1), hero_image_tint, rounding);
    bg_list->AddRectFilledMultiColor(windowPos, windowPos + ImVec2(windowSize.x, hero_height), hero_overlay_top, hero_overlay_top, hero_overlay_bottom, hero_overlay_bottom);
    bg_list->AddRectFilled(windowPos + ImVec2(0, hero_height), windowPos + windowSize, body, rounding, ImDrawFlags_RoundCornersBottom);
    bg_list->AddLine(windowPos + ImVec2(22.0f, hero_height), windowPos + ImVec2(windowSize.x - 22.0f, hero_height), border, 1.0f);
    bg_list->AddRect(windowPos, windowPos + windowSize, border, rounding, 0, 1.0f);
}

void renderButtons(std::vector<std::unique_ptr<PopupButton>>& buttons) {
    static auto &style = ImGui::GetStyle();
    const int columns = static_cast<int>(buttons.size());
    const auto width = (ImGui::GetContentRegionAvail().x / columns) - ((style.ItemSpacing.x / static_cast<float>(columns)) * (columns - 1));

    for (int i = 0; i < static_cast<int>(buttons.size()); i++) {
        auto& button = buttons[i];
        (*button).render2({ width, 46 });
        if (!(*button).getTooltip().empty()) ImGui::SetItemTooltip((*button).getTooltip().c_str());

        const bool end_of_row = ((i + 1) % columns) == 0;
        if (!end_of_row) {
            ImGui::SameLine();
        }
    }
    ImGui::Dummy({ 0, 1 });
}

void renderHeader(std::string title, std::string description) {

    static auto& style = ImGui::GetStyle();
    ImDrawList* list = ImGui::GetWindowDrawList();
    const auto widgetPos = ImGui::GetCursorScreenPos();
    static const auto eyebrow = "SERVER ROUTING CONTROL";
    static const ImU32 color_text = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);
    static const ImU32 color_text_soft = ImGui::ColorConvertFloat4ToU32({ style.Colors[ImGuiCol_Text].x, style.Colors[ImGuiCol_Text].y, style.Colors[ImGuiCol_Text].z, 0.74f });
    static const ImU32 color_accent = ImGui::ColorConvertFloat4ToU32({ 0.14f, 0.45f, 0.93f, 1.0f });

    ImGui::BeginGroup();
    {
        list->AddText(font_subtitle, 14.0f, widgetPos, color_accent, eyebrow);
        ImGui::Dummy({ 0, 18.0f });
        list->AddText(font_title, font_title->FontSize, ImGui::GetCursorScreenPos() - ImVec2(1, 0), color_text, title.c_str());
        ImGui::Dummy({ 0, font_title->FontSize - 14.0f });

        list->AddText(font_subtitle, 16.0f, ImGui::GetCursorScreenPos(), color_text_soft, description.c_str());
        ImGui::Dummy({ 0, font_subtitle->FontSize + 2.0f });
    }
    ImGui::EndGroup();
}

void renderEndpoints() {
    static const std::vector<EndpointGroup> groups {
        { "US + Americas", { "USA - Central", "USA - East", "USA - West", "Brazil" } },
        { "Europe + Middle East", { "Finland", "Netherlands", "Saudi Arabia" } },
        { "Asia + Pacific", { "Singapore", "Tokyo", "South Korea", "Taiwan", "Australia" } },
    };

    static auto& style = ImGui::GetStyle();
    const float spacing = style.ItemSpacing.x;
    const float group_width = (ImGui::GetContentRegionAvail().x - (spacing * 2.0f)) / 3.0f;
    const float group_height = (std::max)(360.0f, ImGui::GetContentRegionAvail().y - 6.0f);
    const float group_bottom_padding = style.WindowPadding.y + style.FramePadding.y;
    static const ImVec4 group_backgrounds[] {
        { 0.93f, 0.96f, 1.00f, 0.74f },
        { 0.95f, 0.97f, 0.98f, 0.78f },
        { 0.95f, 0.98f, 0.96f, 0.78f },
    };
    static const ImVec4 group_accents[] {
        { 0.14f, 0.45f, 0.93f, 1.0f },
        { 0.10f, 0.60f, 0.53f, 1.0f },
        { 0.96f, 0.57f, 0.16f, 1.0f },
    };
    static const ImU32 color_title = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);
    static const ImU32 color_title_soft = ImGui::ColorConvertFloat4ToU32({ style.Colors[ImGuiCol_Text].x, style.Colors[ImGuiCol_Text].y, style.Colors[ImGuiCol_Text].z, 0.58f });

    ImGui::Dummy({ 0, 6 });
    for (int group_index = 0; group_index < static_cast<int>(groups.size()); group_index++) {
        const auto& group = groups[group_index];
        const auto& group_bg = group_backgrounds[group_index];
        const auto& group_accent = group_accents[group_index];
        const ImU32 color_accent = ImGui::ColorConvertFloat4ToU32(group_accent);

        ImGui::PushStyleColor(ImGuiCol_ChildBg, group_bg);
        ImGui::BeginChild(group.title, ImVec2(group_width, group_height), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        {
            ImDrawList* child_list = ImGui::GetWindowDrawList();
            const ImVec2 child_pos = ImGui::GetWindowPos();
            const ImVec2 child_size = ImGui::GetWindowSize();

            child_list->AddRectFilled(child_pos + ImVec2(14.0f, 14.0f), child_pos + ImVec2(child_size.x - 14.0f, 18.0f), color_accent, 999.0f);
            child_list->AddRect(child_pos, child_pos + child_size, ImGui::ColorConvertFloat4ToU32({ group_accent.x, group_accent.y, group_accent.z, 0.18f }), 18.0f, 0, 1.0f);

            ImGui::Dummy({ 0, 8 });
            child_list->AddText(font_subtitle, 14.0f, ImGui::GetCursorScreenPos(), color_accent, "REGION GROUP");
            ImGui::Dummy({ 0, 14 });
            child_list->AddText(font_subtitle, 18.0f, ImGui::GetCursorScreenPos(), color_title, group.title);
            ImGui::Dummy({ 0, 18 });
            child_list->AddText(font_subtitle, 13.0f, ImGui::GetCursorScreenPos(), color_title_soft, "Toggle regions below");
            ImGui::Dummy({ 0, 18 });

            int hue_index = group_index * 8;
            for (int endpoint_index = 0; endpoint_index < static_cast<int>(group.endpoints.size()); endpoint_index++) {
                auto endpoint = findEndpoint(group.endpoints[endpoint_index]);
                if (!endpoint) {
                    continue;
                }

                (*endpoint).render(hue_index++, true);

                if (ImGui::IsItemActivated()) {
                    syncEndpointState(endpoint);
                }
            }

            ImGui::Dummy({ 0, group_bottom_padding });
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();

        if (group_index < static_cast<int>(groups.size()) - 1) {
            ImGui::SameLine();
        }
    }
}

void renderFooter(bool* open) {

    static auto& style = ImGui::GetStyle();
    ImDrawList* list = ImGui::GetWindowDrawList();

    static const ImU32 white = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, 1 });
    static const ImU32 color_text = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);

    ImGui::Dummy({ ImGui::GetContentRegionAvail().x, ImGui::GetFont()->FontSize + (style.FramePadding.y * 2) });
    list->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), white, 5);
    list->AddText(ImGui::GetItemRectMin() + style.FramePadding + ImVec2(2, 0), color_text, "Advanced");

    static ImVec2 frame = ImVec2(24, 24);
    auto const pos = ImVec2(ImGui::GetItemRectMax() - frame - style.FramePadding);
    auto const uv_min = !*open ? ImVec2(0, 0) : ImVec2(0, 1);
    auto const uv_max = !*open ? ImVec2(1, 1) : ImVec2(1, 0);
    list->AddImage(_get_texture("icon_angle"), pos, pos + frame, uv_min, uv_max, color_text);

    if (ImGui::IsItemClicked())
        *open = !*open;

    if (*open)
    {
        ImGui::BeginGroup();
        {
            ImGui::Indent(style.FramePadding.x);
            {
                ImGui::Text("Available in a future update.");
                // static bool test;
                // ToggleButton("test", &test);
            }
            ImGui::Unindent();
        }
        ImGui::EndGroup();

    }
}

void renderNotice() {
    static const ImU32 white = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, 1 });
    static const ImU32 white2 = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, .8f });
    static const ImU32 white4 = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, .6f });
    static const ImU32 transparent = ImGui::ColorConvertFloat4ToU32({ 0, 0, 0, 0 });
    
    ImDrawList* list = ImGui::GetWindowDrawList();
    ImDrawList* bg_list = ImGui::GetBackgroundDrawList();

    static const auto padding = ImVec2(12, 6);

    static const auto height = 52;

    list->PushClipRect(ImGui::GetCursorScreenPos() + ImVec2(0, padding.y), ImGui::GetCursorScreenPos() + ImVec2(ImGui::GetContentRegionAvail().x, 64), true);
    
    ImGui::Dummy(padding);
    ImGui::SameLine(NULL, 0);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, transparent);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, white4);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, white);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, white2);

    // if scrollbar
    // ImGui::BeginChild("notice", ImVec2(ImGui::GetContentRegionAvail().x - padding.x, height), 0);

    // if no scrollbar
    ImGui::BeginChild("notice", ImVec2(ImGui::GetContentRegionAvail().x, height), 0);
    {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + padding.y);
        ImGui::PushStyleColor(ImGuiCol_Text, white);
        {
            ImGui::Dummy({ 20, 20 });
            list->AddImage((void*)_get_texture("icon_bolt"), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), white);
            ImGui::SameLine();

            list->AddText(font_title, 20, ImGui::GetCursorScreenPos() - ImVec2(0, 1), white, "NOTICE");
            ImGui::PushFont(font_subtitle);
            {
                ImGui::SameLine(145);
                ImGui::TextUnformatted("02/28/2026  GUE4 and GMEC2 updated. Click \"unblock\" once.");
                // ImGui::TextWrapped("New server - GUE4 (USA - East) has been added. \n\nPlease let us know if you have any issues.\n\n:)");
                // ImGui::TextWrapped("The blocklist has been updated, please let us know if you have any connection issues.");
                // ImGui::TextWrapped("To report a connection issue: Please check which server you are connected to with CTRL");
            }
            ImGui::PopFont();
        }
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor(4);

    static const auto bg_color = ImGui::ColorConvertFloat4ToU32({ 0.95f, 0.58f, 0.15f, 0.92f });

    // background texture
    {
        bg_list->AddRectFilled(ImGui::GetItemRectMin() - ImVec2(padding.x, 0), ImGui::GetItemRectMax(), bg_color, 9);
    }

    {
        static const auto color = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, 0.11f });
        const auto pos = ImGui::GetItemRectMin() - padding;

        static const auto image = _get_image("background_diagonal");
        list->AddImage(image.texture, pos, pos + ImVec2((float)image.width, (float)image.height), ImVec2(0, 0), ImVec2(1, 1), color);
    }

    list->PopClipRect();

    //ImGui::Spacing();
}

void Dashboard::render() {

    /* background */
    renderBackground();

    if (!g_updater) {
        ImGui::TextColored(ImVec4{ 1, 0.6f, 0.6f, 1 }, "TEST VERSION - Updater disabled\nUpdate manually");
    }

    /* notice */
    renderNotice();

    /* header */
    const auto whitelist_only = g_settings && (*g_settings).getAppSettings().options.whitelist_only;
    renderHeader(
        "OW2 // DROPSHIP",
        whitelist_only
            ? "Whitelist mode is on. Everything starts blocked. Click a blocked region to allow it."
            : "Deselecting regions will block them permanently until you select them again."
    );

    if (g_settings)
    {
        (*g_settings).renderWaitingStatus();
    }

    renderButtons(this->header_actions);

    if (g_tunneling)
    {
        g_tunneling->render();
    }

    /* endpoints */
    ImGui::BeginChild("endpoint_groups", ImVec2(0, 0), false);
    renderEndpoints();
    ImGui::EndChild();

    /* footer */
    //renderFooter(&(this->footer));
}



