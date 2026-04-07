#include "pch.h"

#include "Endpoint.h"

#include "images.h"

extern ImFont* font_title;
extern ImFont* font_subtitle;
extern ImFont* font_text;

void Endpoint2::render(int i, bool compact) {
	const float available_width = ImGui::GetContentRegionAvail().x;
	const bool narrow = compact && available_width < 210.0f;
	const float card_height = compact ? 36.0f : 60.0f;
	const float title_font_size = compact ? (narrow ? 15.0f : 17.0f) : 31.0f;
	const float subtitle_font_size = compact ? (narrow ? 10.0f : 11.0f) : 20.0f;
	const float ping_font_size = compact ? (narrow ? 11.0f : 12.0f) : 21.0f;
	const ImVec2 icon_padding = compact ? (narrow ? ImVec2(9, 9) : ImVec2(10, 10)) : ImVec2(18, 18);
	const ImVec2 ping_frame = compact ? (narrow ? ImVec2(13, 13) : ImVec2(14, 14)) : ImVec2(22, 22);
	const float same_line_offset = compact ? 8.0f : 16.0f;

	//throw std::runtime_error("invalid runtime variable in array");

	if (*(this->ping.get())) {
		const auto ping_ms = (*(this->ping.get())).value();
		if (this->ping_ms_display != ping_ms)
		{
			static const float min_delay = 9.0f;
			static const float max_delay = 90.0f;

			if (ping_ms > 0 && this->ping_ms_display <= 0)
			{
				this->ping_ms_display = ping_ms;
			}
			else {
				float param = fmin((float)std::abs(ping_ms - this->ping_ms_display) / 24.0f, 1.0f);

				if (!(ImGui::GetFrameCount() % (int)fmax(min_delay, max_delay - (max_delay * param * half_pi)) != 0)) {


					if (this->ping_ms_display < ping_ms)
						this->ping_ms_display++;
					else
						this->ping_ms_display--;

					this->ping_ms_display = std::max(0, this->ping_ms_display);
				}
			}
		}
	}

	//
	//

	auto const w_list = ImGui::GetWindowDrawList();
	static auto& style = ImGui::GetStyle();

	//const auto grayed_out = this->ip_ping.value_or("").empty(); // || (endpoint._has_pinged && !(endpoint.ping > 0) && !endpoint._has_pinged_successfully);
	static const auto grayed_out = false;
	const auto unsynced = (this->blocked != this->blocked_desired);
	const ImVec4 accent = grayed_out
		? ImVec4(0.75f, 0.79f, 0.84f, 1.0f)
		: (ImVec4)ImColor::HSV(1.0f - ((i + 1) / 32.0f), 0.54f, 0.92f, 1.0f);
	const ImU32 color_accent = ImGui::ColorConvertFloat4ToU32(accent);
	const ImU32 color_accent_soft = ImGui::ColorConvertFloat4ToU32({ accent.x, accent.y, accent.z, this->blocked ? 0.08f : 0.16f });
	const ImU32 color_accent_soft_hover = ImGui::ColorConvertFloat4ToU32({ accent.x, accent.y, accent.z, this->blocked ? 0.12f : 0.22f });
	const ImU32 color_accent_chip = ImGui::ColorConvertFloat4ToU32({ accent.x, accent.y, accent.z, this->blocked ? 0.14f : 0.20f });
	const ImU32 color_border = ImGui::ColorConvertFloat4ToU32({ accent.x, accent.y, accent.z, this->blocked ? 0.22f : 0.30f });
	const ImU32 color_border_hover = ImGui::ColorConvertFloat4ToU32({ accent.x, accent.y, accent.z, 0.46f });
	const ImU32 color_card = ImGui::ColorConvertFloat4ToU32(this->blocked ? ImVec4(1.0f, 1.0f, 1.0f, 0.68f) : ImVec4(1.0f, 1.0f, 1.0f, 0.82f));
	const ImU32 color_title = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);
	const ImU32 color_title_muted = ImGui::ColorConvertFloat4ToU32({ style.Colors[ImGuiCol_Text].x, style.Colors[ImGuiCol_Text].y, style.Colors[ImGuiCol_Text].z, 0.82f });
	const ImU32 color_text_secondary = ImGui::ColorConvertFloat4ToU32({ style.Colors[ImGuiCol_Text].x, style.Colors[ImGuiCol_Text].y, style.Colors[ImGuiCol_Text].z, this->blocked ? 0.56f : 0.68f });
	const ImU32 transparent = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, 0.0f });
	const float rounding = compact ? 11.0f : 16.0f;

	ImGui::Dummy({ 0 ,0 });
	ImGui::SameLine(NULL, same_line_offset);

	ImGui::PushID(i);
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, transparent);
	ImGui::PushStyleColor(ImGuiCol_Header, transparent);
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, transparent);
	ImGui::PushStyleColor(ImGuiCol_NavHighlight, NULL);
	bool action = (ImGui::Selectable("##end", &(this->blocked_desired), ImGuiSelectableFlags_SelectOnClick, { ImGui::GetContentRegionAvail().x - (compact ? 8.0f : 16.0f), card_height }));
	//bool action = (ImGui::Selectable(("##" + this->title).c_str(), &(this->blocked_desired), ImGuiSelectableFlags_SelectOnClick, {ImGui::GetContentRegionAvail().x - 16, 74 - 9}));
	ImGui::PopStyleColor(4);
	ImGui::PopID();

	auto hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);

	const ImVec2 item_min = ImGui::GetItemRectMin();
	const ImVec2 item_max = ImGui::GetItemRectMax();
	const ImVec2 item_size = ImGui::GetItemRectSize();

	w_list->AddRectFilled(item_min, item_max, hovered ? color_accent_soft_hover : color_card, rounding);
	w_list->AddRectFilled(item_min, item_max, hovered ? color_accent_soft_hover : color_accent_soft, rounding);
	w_list->AddRect(item_min, item_max, hovered ? color_border_hover : color_border, rounding, 0, hovered ? 1.6f : 1.0f);

	const float accent_width = compact ? 5.0f : 7.0f;
	w_list->AddRectFilled(item_min, ImVec2(item_min.x + accent_width, item_max.y), color_accent, rounding);

	// unsynced background
	if (unsynced)
	{
		const auto color = ImGui::ColorConvertFloat4ToU32({ accent.x, accent.y, accent.z, 0.20f });
		const auto offset = (ImGui::GetFrameCount() / 4) % 40;
		const auto offset_vec = ImVec2((float)offset, (float)offset);
		const auto pos = item_min - ImVec2(40, 40) + offset_vec;

		static const auto image = _get_image("background_diagonal");

		w_list->PushClipRect(item_min, item_max, true);
		w_list->AddImage(image.texture, pos, pos + ImVec2((float) image.width, (float) image.height), ImVec2(0, 0), ImVec2(1, 1), color);
		w_list->PopClipRect();
	}

	// icon
	const auto icon_frame = ImVec2({ item_size.y, item_size.y });
	const auto icon = unsynced ? _get_texture("icon_wall_fire") : (this->blocked ? _get_texture("icon_block") : _get_texture("icon_allow"));
	const ImVec2 icon_chip_min = item_min + ImVec2(accent_width + 8.0f, compact ? 6.0f : 8.0f);
	const ImVec2 icon_chip_max = icon_chip_min + ImVec2(icon_frame.y - (compact ? 12.0f : 16.0f), icon_frame.y - (compact ? 12.0f : 16.0f));
	w_list->AddRectFilled(icon_chip_min, icon_chip_max, color_accent_chip, compact ? 8.0f : 10.0f);
	w_list->AddImage(icon, item_min + ImVec2(accent_width, 0.0f) + icon_padding, item_min + ImVec2(accent_width, 0.0f) + icon_frame - icon_padding, ImVec2(0, 0), ImVec2(1, 1), color_accent);

	// display 1
	auto pos = item_min + ImVec2(icon_frame.x + accent_width + 6.0f, compact ? 1.0f : 3.0f) + ImVec2(-2, 0);
	w_list->AddText(font_title, title_font_size, pos, this->blocked ? color_title_muted : color_title, this->title.c_str());

	// display 2
	pos += ImVec2(1, item_size.y - subtitle_font_size - (compact ? 4.0f : 13.0f));
	// w_list->AddText(font_subtitle, 24, pos, this->blocked ? color_secondary : color_text_secondary, this->description.c_str());
	const auto subtitle = compact ? this->description : ((narrow || !this->blocked) ? this->description : (this->description + " (blocked)"));
	w_list->AddText(font_subtitle, subtitle_font_size, pos, this->blocked ? color_text_secondary : color_accent, subtitle.c_str());

	if ((*(this->ping))) {

		// display 3 / (icon wifi)
		auto icon = _get_texture("icon_wifi");

		const auto ping = (*(this->ping.get())).value();

		if (ping > 90)
			icon = _get_texture("icon_wifi_poor");
		else if (ping > 40)
			icon = _get_texture("icon_wifi_fair");
		else if (ping < 0)
			icon = _get_texture("icon_wifi_slash");

		auto pos = item_max - ImVec2(ping_frame.x, item_size.y) + (style.FramePadding * ImVec2(-1, 1)) + ImVec2(-6, compact ? -3.0f : 1.0f);
		w_list->AddImage(icon, pos, pos + ping_frame, ImVec2(0, 0), ImVec2(1, 1), this->blocked ? color_text_secondary : color_title);

		// display 4
		if (!(ping < 0)) {
			auto const text = std::to_string(this->ping_ms_display);
			auto text_size = font_subtitle->CalcTextSizeA(ping_font_size, FLT_MAX, 0.0f, text.c_str());
			auto pos = item_max - style.FramePadding - text_size + ImVec2(-5, compact ? -2.0f : -3.0f);

			w_list->AddText(font_subtitle, ping_font_size, pos, this->blocked ? color_text_secondary : color_title, text.c_str());
		}
	}

	/* context menu*/
	/* {
		if (ImGui::BeginPopupContextItem()) // <-- use last item id as popup id
		{
			if (ImGui::MenuItem("All but this", nullptr, nullptr, true)) {};
			if (ImGui::MenuItem("Hide", nullptr, nullptr, true)) {};
			ImGui::EndPopup();
		}
		ImGui::SetItemTooltip("Right-click to open popup");
	} */

	ImGui::Dummy({ 0, compact ? 0.0f : 2.0f });
}
