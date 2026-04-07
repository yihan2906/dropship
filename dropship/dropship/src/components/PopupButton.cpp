#include "pch.h"

#include "PopupButton.h"

PopupButton::PopupButton(std::string title, std::string icon, std::vector<Action> const actions, int i, std::string tooltip):
	_title(title),
	_icon(icon),
    _actions(actions),
    _i(i),

	//popup_name(title + "_popup")
	_popup_name(title),
    _tooltip(tooltip)
{
	
}

void PopupButton::render2(const ImVec2& size) {

    static auto& style = ImGui::GetStyle();
    ImDrawList* list = ImGui::GetWindowDrawList();

    const ImVec4 accent = (ImVec4)ImColor::HSV(1.0f - ((this->_i + 1) / 32.0f), 0.55f, 0.92f, 1.0f);
    const ImU32 color_accent = ImGui::ColorConvertFloat4ToU32(accent);
    const ImU32 color_text = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);
    const ImU32 color_text_soft = ImGui::ColorConvertFloat4ToU32({ style.Colors[ImGuiCol_Text].x, style.Colors[ImGuiCol_Text].y, style.Colors[ImGuiCol_Text].z, 0.88f });
    const ImU32 color_border = ImGui::ColorConvertFloat4ToU32({ accent.x, accent.y, accent.z, 0.22f });
    const ImU32 color_border_hover = ImGui::ColorConvertFloat4ToU32({ accent.x, accent.y, accent.z, 0.38f });
    const ImU32 color_background = ImGui::ColorConvertFloat4ToU32({ 1.0f, 1.0f, 1.0f, 0.70f });
    const ImU32 color_background_hover = ImGui::ColorConvertFloat4ToU32({ accent.x, accent.y, accent.z, 0.14f });
    const ImU32 color_icon_chip = ImGui::ColorConvertFloat4ToU32({ accent.x, accent.y, accent.z, 0.16f });
    const float rounding = 14.0f;

    {
        static const auto font = font_text;
        const auto text_size = font->CalcTextSizeA(20, FLT_MAX, 0.0f, this->_title.c_str());

        ImGui::Dummy(size);
        auto hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
        auto active = ImGui::IsItemActive();

        const ImVec2 min = ImGui::GetItemRectMin();
        const ImVec2 max = ImGui::GetItemRectMax();
        const ImVec2 item_size = ImGui::GetItemRectSize();

        list->AddRectFilled(min, max, hovered ? color_background_hover : color_background, rounding);
        list->AddRect(min, max, hovered || active ? color_border_hover : color_border, rounding, 0, hovered || active ? 1.5f : 1.0f);

        const float chip_size = item_size.y - 14.0f;
        const ImVec2 chip_min = min + ImVec2(8.0f, 7.0f);
        const ImVec2 chip_max = chip_min + ImVec2(chip_size, chip_size);
        list->AddRectFilled(chip_min, chip_max, color_icon_chip, 10.0f);

        const ImVec2 frame = { 18.0f, 18.0f };
        const ImVec2 icon_min = chip_min + ((chip_max - chip_min - frame) * 0.5f);
        list->AddImage((void*)_get_texture(this->_icon), icon_min, icon_min + frame, ImVec2(0, 0), ImVec2(1, 1), color_accent);

        const ImVec2 text_pos = { chip_max.x + 10.0f, min.y + ((item_size.y - text_size.y) * 0.5f) - 3.0f };
        list->AddText(font, 20, text_pos, hovered ? color_text : color_text_soft, this->_title.c_str());

        if (active) {
            list->AddRect(min + ImVec2(1.0f, 1.0f), max - ImVec2(1.0f, 1.0f), color_border_hover, rounding, 0, 1.0f);
        }
    }


    if (ImGui::IsItemClicked()) {
        if (this->_actions.size() != 1) {
            ImGui::OpenPopup(this->_popup_name.c_str());
        }
        else {
            this->_actions[0].action();
        }
    }

    this->renderPopup();
}

const std::string& PopupButton::getTooltip() const { return this->_tooltip; }

void PopupButton::render() {

	ImDrawList* list = ImGui::GetWindowDrawList();

	/*const auto color_button = ImColor::HSV(0, 0.4f, 1, 1.f);
	const auto color_button_hover = ImColor::HSV(0, 0.25f, 1, 1.f);*/

    const ImU32 color_button = (ImU32) ImColor::HSV(1.0f - ((this->_i + 1) / 32.0f), 0.4f, 1.0f, 1.0f);
    const ImU32 color_button_hover = (ImU32) ImColor::HSV(1.0f - ((this->_i + 1) / 32.0f), 0.3f, 1.0f, 1.0f);
    const ImU32 color_secondary_faded = (ImU32) ImColor::HSV(1.0f - ((this->_i + 1) / 32.0f), 0.2f, 1.0f, 0.4f * 1.0f);

	//ImGui::SameLine();
	ImGui::Dummy({ 24.0f, 24.0f });

	list->AddImage((void*)_get_texture(this->_icon), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), ImGui::IsItemHovered() ? color_button_hover : color_button);

	if (ImGui::IsItemClicked()) {
        printf("opening %s", this->_popup_name.c_str());
		ImGui::OpenPopup(this->_popup_name.c_str());
	}

    this->renderPopup();
}


void PopupButton::renderPopup() {

    // Always center this window when appearing
    ImVec2 center = ImGui::GetWindowViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 14.0f));

    if (ImGui::BeginPopupModal(this->_popup_name.c_str(), NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImDrawList* list = ImGui::GetWindowDrawList();

        // handle close
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsWindowHovered() && !ImGui::IsWindowAppearing())
            ImGui::CloseCurrentPopup();

        ImGui::PushItemFlag(ImGuiItemFlags_SelectableDontClosePopup, true);
        ImGui::PushFont(font_subtitle);
        {
            int i = 1;

            for (auto& item : this->_actions) {
                const ImU32 color = (ImU32) ImColor::HSV(1.0f - ((i + 1) / 32.0f), 0.4f, 1.0f, 1.0f);
                // const ImU32 color_secondary = (ImU32) ImColor::HSV(1.0f - ((i + 1) / 32.0f), 0.3f, 1.0f, 1.0f);
                const ImU32 color_secondary_faded = (ImU32) ImColor::HSV(1.0f - ((i + 1) / 32.0f), 0.2f, 1.0f, 0.4f * 1.0f);



                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, color_secondary_faded);
                {
                    if (item.state == nullptr) {
                        if (ImGui::MenuItem(item.title.c_str(), item.description.c_str(), nullptr, !item.disabled)) item.action();
                    }
                    else {
                        bool checked = item.state();
                        // .. &checked, ..
                        if (ImGui::MenuItem(item.title.c_str(), item.description.empty() ? (checked ? "on" : "off") : (checked ? item.description.c_str() : nullptr), nullptr, !item.disabled)) item.action();
                    }
                    if (!item.tooltip.empty()) ImGui::SetItemTooltip(item.tooltip.c_str());
                    if (item.external) {
                        static auto offset = ImVec2(-8, 8);
                        static auto frame = ImVec2(18, 18);

                        const ImVec2 pos = ImVec2(ImGui::GetItemRectMax().x - frame.x, ImGui::GetItemRectMin().y) + offset;
                        list->AddImage(_get_texture("icon_outside_window"), pos, pos + frame, ImVec2(0, 0), ImVec2(1, 1), color);
                    }
                }
                ImGui::PopStyleColor();
                i ++;

                if (item.divide_next) {
                    ImGui::Spacing();
                }
            }

        }
        ImGui::PopFont();
        ImGui::PopItemFlag();

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}
