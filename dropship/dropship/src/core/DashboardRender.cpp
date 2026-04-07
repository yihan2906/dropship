#include "pch.h"

#include "Dashboard.h"

#include <fstream>
#include <wincrypt.h>
#include <wininet.h>

#pragma comment(lib, "wininet.lib")

extern std::unique_ptr<std::vector<std::shared_ptr<Endpoint2>>> g_endpoints;
extern std::unique_ptr<Settings> g_settings;
extern std::unique_ptr<core::tunneling::Tunneling> g_tunneling;
extern std::unique_ptr<util::watcher::window::WindowWatcher> g_window_watcher;

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

    std::wstring getExecutablePath()
    {
        wchar_t path[MAX_PATH] {};
        const DWORD length = ::GetModuleFileNameW(nullptr, path, MAX_PATH);
        if (length == 0 || length == MAX_PATH) {
            throw std::runtime_error("failed to resolve executable path");
        }

        return std::wstring(path, length);
    }

    std::string toHexString(const BYTE* bytes, DWORD size)
    {
        static const char hex[] = "0123456789abcdef";

        std::string result;
        result.resize(size * 2);

        for (DWORD index = 0; index < size; index++) {
            result[(index * 2) + 0] = hex[(bytes[index] >> 4) & 0x0F];
            result[(index * 2) + 1] = hex[bytes[index] & 0x0F];
        }

        return result;
    }

    std::string calculateFileSha512(const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::binary);
        if (!input.is_open()) {
            throw std::runtime_error("failed to open executable");
        }

        HCRYPTPROV provider = 0;
        HCRYPTHASH hash = 0;

        if (!::CryptAcquireContextW(&provider, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
            throw std::runtime_error("CryptAcquireContextW failed");
        }

        if (!::CryptCreateHash(provider, CALG_SHA_512, 0, 0, &hash)) {
            ::CryptReleaseContext(provider, 0);
            throw std::runtime_error("CryptCreateHash failed");
        }

        std::string buffer(8192, '\0');
        while (input) {
            input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            const auto read_count = input.gcount();
            if (read_count <= 0) {
                continue;
            }

            if (!::CryptHashData(hash, reinterpret_cast<const BYTE*>(buffer.data()), static_cast<DWORD>(read_count), 0)) {
                ::CryptDestroyHash(hash);
                ::CryptReleaseContext(provider, 0);
                throw std::runtime_error("CryptHashData failed");
            }
        }

        BYTE digest[64] {};
        DWORD digest_size = sizeof(digest);
        if (!::CryptGetHashParam(hash, HP_HASHVAL, digest, &digest_size, 0)) {
            ::CryptDestroyHash(hash);
            ::CryptReleaseContext(provider, 0);
            throw std::runtime_error("CryptGetHashParam failed");
        }

        ::CryptDestroyHash(hash);
        ::CryptReleaseContext(provider, 0);

        return toHexString(digest, digest_size);
    }

    std::string downloadUrlText(const std::wstring& url)
    {
        HINTERNET internet = ::InternetOpenW(L"dropship/1.0", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
        if (!internet) {
            throw std::runtime_error("InternetOpenW failed");
        }

        static constexpr wchar_t headers[] =
            L"Accept: application/vnd.github+json\r\n"
            L"X-GitHub-Api-Version: 2022-11-28\r\n";

        HINTERNET request = ::InternetOpenUrlW(
            internet,
            url.c_str(),
            headers,
            static_cast<DWORD>(std::size(headers) - 1),
            INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE,
            0
        );

        if (!request) {
            ::InternetCloseHandle(internet);
            throw std::runtime_error("InternetOpenUrlW failed");
        }

        std::string response;
        char buffer[4096] {};
        DWORD bytes_read = 0;

        while (::InternetReadFile(request, buffer, sizeof(buffer), &bytes_read) && bytes_read > 0) {
            response.append(buffer, bytes_read);
        }

        ::InternetCloseHandle(request);
        ::InternetCloseHandle(internet);

        return response;
    }

    std::string fetchLatestReleaseDate()
    {
        const auto response = downloadUrlText(L"https://api.github.com/repos/stowmyy/dropship/releases/latest");
        const auto release = json::parse(response);
        const auto published_at = release.at("published_at").get<std::string>();

        if (published_at.size() < 10) {
            throw std::runtime_error("release date missing");
        }

        return published_at.substr(0, 10);
    }

    struct BuildInfoPayload {
        std::string hash_text;
        std::string release_date_text;
    };

    struct BuildInfoState {
        BuildInfoState() :
            build_info_future(std::async(std::launch::async, []() {
                auto hash = calculateFileSha512(std::filesystem::path(getExecutablePath()));
                return BuildInfoPayload {
                    .hash_text = hash.substr(0, 9),
                    .release_date_text = fetchLatestReleaseDate(),
                };
            }))
        {}

        std::future<BuildInfoPayload> build_info_future;
        std::string hash_text { "loading..." };
        std::string release_date_text { "loading..." };
        bool resolved { false };
    };

    BuildInfoState& getBuildInfoState()
    {
        static BuildInfoState state;

        if (!state.resolved && state.build_info_future.valid() && state.build_info_future.wait_for(0ms) == std::future_status::ready) {
            try {
                const auto payload = state.build_info_future.get();
                state.hash_text = payload.hash_text;
                state.release_date_text = payload.release_date_text;
            }
            catch (...) {
                state.hash_text = "unavailable";
                state.release_date_text = "unavailable";
            }

            state.resolved = true;
        }

        return state;
    }

    void renderBuildInfoBadge()
    {
        ImDrawList* list = ImGui::GetWindowDrawList();
        const auto& style = ImGui::GetStyle();
        auto& build_info = getBuildInfoState();

        static const char* label = "BUILD HASH";
        static const char* release_label = "RELEASE DATE";
        static const char* detail = "MANUAL UPDATE ONLY";

        const ImVec2 anchor = ImGui::GetCursorScreenPos();
        const float available_width = ImGui::GetContentRegionAvail().x;

        const auto label_size = font_subtitle->CalcTextSizeA(13.0f, FLT_MAX, 0.0f, label);
        const auto hash_size = font_subtitle->CalcTextSizeA(18.0f, FLT_MAX, 0.0f, build_info.hash_text.c_str());
        const auto release_label_size = font_subtitle->CalcTextSizeA(13.0f, FLT_MAX, 0.0f, release_label);
        const auto release_date_size = font_subtitle->CalcTextSizeA(16.0f, FLT_MAX, 0.0f, build_info.release_date_text.c_str());
        const auto detail_size = font_subtitle->CalcTextSizeA(12.0f, FLT_MAX, 0.0f, detail);

        float content_width = label_size.x;
        content_width = (std::max)(content_width, hash_size.x);
        content_width = (std::max)(content_width, release_label_size.x);
        content_width = (std::max)(content_width, release_date_size.x);
        content_width = (std::max)(content_width, detail_size.x);

        const float badge_width = content_width + 28.0f;
        const float badge_height = label_size.y + hash_size.y + release_label_size.y + release_date_size.y + detail_size.y + 28.0f;
        const ImVec2 badge_min = anchor + ImVec2((std::max)(0.0f, available_width - badge_width), 0.0f);
        const ImVec2 badge_max = badge_min + ImVec2(badge_width, badge_height);

        static const ImU32 badge_background = ImGui::ColorConvertFloat4ToU32({ 0.10f, 0.16f, 0.28f, 0.84f });
        static const ImU32 badge_border = ImGui::ColorConvertFloat4ToU32({ 0.25f, 0.56f, 0.98f, 0.34f });
        static const ImU32 label_color = ImGui::ColorConvertFloat4ToU32({ 0.61f, 0.79f, 1.00f, 0.96f });
        static const ImU32 hash_color = ImGui::ColorConvertFloat4ToU32({ 1.00f, 1.00f, 1.00f, 1.00f });
        const ImU32 detail_color = ImGui::ColorConvertFloat4ToU32({
            style.Colors[ImGuiCol_TextDisabled].x,
            style.Colors[ImGuiCol_TextDisabled].y,
            style.Colors[ImGuiCol_TextDisabled].z,
            0.94f
        });

        list->AddRectFilled(badge_min, badge_max, badge_background, 16.0f);
        list->AddRect(badge_min, badge_max, badge_border, 16.0f, 0, 1.0f);
        list->AddText(font_subtitle, 13.0f, badge_min + ImVec2(14.0f, 8.0f), label_color, label);
        list->AddText(font_subtitle, 18.0f, badge_min + ImVec2(14.0f, 22.0f), hash_color, build_info.hash_text.c_str());
        list->AddText(font_subtitle, 13.0f, badge_min + ImVec2(14.0f, 26.0f + hash_size.y), label_color, release_label);
        list->AddText(font_subtitle, 16.0f, badge_min + ImVec2(14.0f, 40.0f + hash_size.y), hash_color, build_info.release_date_text.c_str());
        list->AddText(font_subtitle, 12.0f, badge_min + ImVec2(14.0f, 42.0f + hash_size.y + release_date_size.y), detail_color, detail);
    }

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

    renderBuildInfoBadge();

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



