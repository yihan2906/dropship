#include "pch.h"

#include "Settings.h"

#include <fstream>


extern std::unique_ptr<std::vector<std::shared_ptr<Endpoint2>>> g_endpoints;
extern std::unique_ptr<Firewall> g_firewall;
extern std::unique_ptr<util::watcher::window::WindowWatcher> g_window_watcher;

extern ImFont* font_subtitle;

namespace dropship::settings {
	void to_json(json& j, const dropship_app_settings& p) {

		j = json {
			{"options", {
				{ "ping_servers", p.options.ping_servers },
				{ "tunneling", p.options.tunneling },
				{ "whitelist_only", p.options.whitelist_only },
			}},
			{"config", {
				{ "blocked_endpoints", p.config.blocked_endpoints },
				{ "allowed_endpoints", p.config.allowed_endpoints },
			}},
		};

		if (p.config.tunneling_path)
		{
			j["/config/tunneling_path"_json_pointer] = p.config.tunneling_path.value();
		}
	}


	void from_json(const json& j, dropship_app_settings& p) {
		if (j.contains("/options/ping_servers"_json_pointer)) j.at("/options/ping_servers"_json_pointer).get_to(p.options.ping_servers);
		if (j.contains("/options/tunneling"_json_pointer)) j.at("/options/tunneling"_json_pointer).get_to(p.options.tunneling);
		if (j.contains("/options/whitelist_only"_json_pointer)) j.at("/options/whitelist_only"_json_pointer).get_to(p.options.whitelist_only);

		if (j.contains("/config/blocked_endpoints"_json_pointer)) j.at("/config/blocked_endpoints"_json_pointer).get_to(p.config.blocked_endpoints);
		if (j.contains("/config/allowed_endpoints"_json_pointer)) j.at("/config/allowed_endpoints"_json_pointer).get_to(p.config.allowed_endpoints);

		/* optional values */
		//if (j.contains("/config/tunneling_path"_json_pointer)) j.at("/config/tunneling_path"_json_pointer).get_to(p.config.tunneling_path);
		const auto pt = "/config/tunneling_path"_json_pointer;
		if (j.contains(pt)) {
			p.config.tunneling_path = std::make_optional<std::filesystem::path>(j.at(pt));
		}
	}
}




const dropship::settings::dropship_app_settings& Settings::getAppSettings()
{
	return this->_dropship_app_settings;
};


bool Settings::isEndpointBlocked(const std::string& endpoint_title) const
{
	if (!this->__ow2_endpoints.contains(endpoint_title)) {
		return false;
	}

	if (this->_dropship_app_settings.options.whitelist_only) {
		return !this->_dropship_app_settings.config.allowed_endpoints.contains(endpoint_title);
	}

	return this->_dropship_app_settings.config.blocked_endpoints.contains(endpoint_title);
}


std::set<std::string> Settings::getBlockedEndpointTitles() const
{
	std::set<std::string> result;

	for (const auto& [title, endpoint] : this->__ow2_endpoints)
	{
		if (this->isEndpointBlocked(title)) {
			result.insert(title);
		}
	}

	return result;
}


void Settings::syncEndpointDesiredStates()
{
	for (auto& endpoint : (*g_endpoints)) {
		(*endpoint).setBlockDesired(this->isEndpointBlocked((*endpoint).getTitle()));
	}
}


std::string Settings::getAllBlockedAddresses() {

	std::string result;

	for (const auto& e : this->getBlockedEndpointTitles())
	{
		if (this->__ow2_endpoints.contains(e)) {

			auto& endpoint = this->__ow2_endpoints.at(e);

			for (auto s : endpoint.blocked_servers)
			{
				auto& server = this->__ow2_servers.at(s);
				result += server.block;
				result += ',';
			}
		}
	}

	if (!result.empty()) {
		result.pop_back();
	}

	// throws errors
	//println("blocking {}", result);

	return result;

}



std::filesystem::path Settings::getStoragePath() const {
	wchar_t local_app_data[MAX_PATH] = { 0 };
	const auto length = ::GetEnvironmentVariableW(L"LOCALAPPDATA", local_app_data, MAX_PATH);

	if (length > 0 && length < MAX_PATH) {
		return std::filesystem::path(local_app_data).append("dropship").append("settings.json");
	}

	return std::filesystem::current_path().append("settings.json");
}

std::optional<json> Settings::readStoragePatch__file() {

#ifdef _DEBUG
	util::timer::Timer timer("readStoragePatch__file");
#endif

	const auto path = this->getStoragePath();
	if (!std::filesystem::exists(path)) {
		return std::nullopt;
	}

	std::ifstream input(path, std::ios::binary);
	if (!input.is_open()) {
		println("failed to open settings file: {}", path.string());
		return std::nullopt;
	}

	try {
		return std::make_optional<json>(json::parse(input));
	}
	catch (const json::exception& e) {
		println("settings file parse error: {}", e.what());
	}

	return std::nullopt;
}

void Settings::tryLoadSettingsFromStorage() {
	auto loaded_settings = this->readStoragePatch();
	if (loaded_settings) {

		try {
			json settings = this->__default_dropship_app_settings;

			//settings.merge_patch(loaded_settings.value());

			/* merge obects: false */
			settings.update(loaded_settings.value(), false);

			this->_dropship_app_settings = settings;
		}
		catch (json::exception& e) {
			println("tryLoadSettingsFromStorage() json error: {}", e.what());
		}
	}
}


void Settings::tryWriteSettingsToStorage(bool force) {

#ifdef _DEBUG
	util::timer::Timer timer("tryWriteSettingsToStorage");
#endif

	const auto game_open = g_window_watcher && (*g_window_watcher).isActive();
	if (game_open && !force)
	{
		//this->_waiting_for_config_write = true;
		auto any = false;
		for (auto& e : (*g_endpoints)) {
			if ((*e).getBlockDesired() != (*e)._getBlockedState()) { any = true; };
		}
		this->_waiting_for_config_write = any;
	}
	else if (force && this->_waiting_for_config_write)
	{
		this->_waiting_for_config_write = false;
	}

	try {
		const auto path = this->getStoragePath();
		std::filesystem::create_directories(path.parent_path());

		std::ofstream output(path, std::ios::binary | std::ios::trunc);
		if (!output.is_open()) {
			println("failed to open settings file for write: {}", path.string());
		}
		else {
			output << json(this->_dropship_app_settings).dump(4);
		}
	}
	catch (const std::exception& e) {
		println("settings file write error: {}", e.what());
	}

	if (!this->_waiting_for_config_write || force)
	{
		(*g_firewall).applyRuleState(this->getAllBlockedAddresses(), this->getAppSettings().options.tunneling ? this->getAppSettings().config.tunneling_path : std::nullopt);

		// TODO if not failed
		for (auto& endpoint : (*g_endpoints)) {
			(*endpoint)._setBlockedState(this->isEndpointBlocked((*endpoint).getTitle()));
		}
	}

	else
	{
		println("waiting for game close");
	}

}

//json mergeSettings(const json& a, const json& b) {
//
//	println("a {}", a.dump(4));
//	println("b {}", b.dump(4));
//
//	throw std::runtime_error("xd");
//
//	/* calculate diff */
//	json merge_patch = a;
//
//	//merge_patch.merge_patch(b);
//	for (auto& b : a.flatten()) {
//		println("ac: {}", b.dump(4));
//	}
//
//	println("merge: {}", merge_patch.dump(4));
//
//	//a.merge_patch(b);
//
//	return a;
//}



std::optional<json> Settings::readStoragePatch() {
	return this->readStoragePatch__file();
}


Settings::Settings() {

	if (!g_endpoints) throw std::runtime_error("settings depends on g_endpoints.");
	if (!g_firewall) throw std::runtime_error("settings depends on g_firewall.");


	this->tryLoadSettingsFromStorage();

	/* load local settings file if present, otherwise use defaults */

	(*g_endpoints).reserve(this->__ow2_endpoints.size());

	for (auto& [key, e] : this->__ow2_endpoints)
	{
		auto blocked = this->isEndpointBlocked(key);

		(*g_endpoints).push_back(std::make_shared<Endpoint2>(
			key,
			e.description,
			this->_dropship_app_settings.options.ping_servers ? e.ip_ping : "",
			blocked
		));
	}

	// Ensure the firewall rule matches the local settings file and clear any legacy rule metadata.
	this->tryWriteSettingsToStorage(true);


	/*std::cout << "__ow2_ranges: " << __ow2_servers.dump(4) << std::endl;
	std::cout << "__ow2_ranges: " << __ow2_ranges.dump(4) << std::endl;
	std::cout << "__ow2_endpoints: " << __ow2_endpoints.dump(4) << std::endl;
	std::cout << "__default: " << __default.dump(4) << std::endl;*/

}

Settings::~Settings() {


	printf("destructor");
}

void Settings::unblockAll() {
	for (auto& endpoint : (*g_endpoints)) {
		(*endpoint).setBlockDesired(false);
	}

	if (this->_dropship_app_settings.options.whitelist_only)
	{
		this->_dropship_app_settings.config.allowed_endpoints.clear();

		for (const auto& [title, endpoint] : this->__ow2_endpoints) {
			this->_dropship_app_settings.config.allowed_endpoints.insert(title);
		}
	}
	else
	{
		this->_dropship_app_settings.config.blocked_endpoints.clear();
	}

	this->tryWriteSettingsToStorage(true); // force
}

void Settings::blockAll() {
	for (auto& endpoint : (*g_endpoints)) {
		(*endpoint).setBlockDesired(true);
	}

	if (this->_dropship_app_settings.options.whitelist_only)
	{
		this->_dropship_app_settings.config.allowed_endpoints.clear();
	}
	else
	{
		this->_dropship_app_settings.config.blocked_endpoints.clear();

		for (const auto& [title, endpoint] : this->__ow2_endpoints) {
			this->_dropship_app_settings.config.blocked_endpoints.insert(title);
		}
	}

	this->tryWriteSettingsToStorage();
}

void Settings::toggleOptionPingServers() {
	this->_dropship_app_settings.options.ping_servers = !this->_dropship_app_settings.options.ping_servers;
	this->tryWriteSettingsToStorage();

	// testing
	// for (auto& e : *g_endpoints) {
	// 	if (this->_dropship_app_settings.options.ping_servers)
	// 	{
	// 		e->start_pinging();
	// 	}
	// 	else
	// 	{
	// 		e->stop_pinging();
	// 	}
	// }
}

void Settings::toggleOptionTunneling() {
	this->_dropship_app_settings.options.tunneling = !this->_dropship_app_settings.options.tunneling;
	this->tryWriteSettingsToStorage();
}

void Settings::toggleOptionWhitelistOnly() {
	this->_dropship_app_settings.options.whitelist_only = !this->_dropship_app_settings.options.whitelist_only;
	this->syncEndpointDesiredStates();
	this->tryWriteSettingsToStorage();
}

void Settings::setConfigTunnelingPath(std::optional<std::filesystem::path> path)
{
	this->_dropship_app_settings.config.tunneling_path = path;
	this->tryWriteSettingsToStorage();
}


// todo std::unordered_set
void Settings::addBlockedEndpoint(std::string endpoint_title) {
	bool changed = false;

	if (this->_dropship_app_settings.options.whitelist_only)
	{
		auto const num_removed = this->_dropship_app_settings.config.allowed_endpoints.erase(endpoint_title);
		changed = num_removed > 0;
	}
	else
	{
		auto [_position, hasBeenInserted] = this->_dropship_app_settings.config.blocked_endpoints.insert(endpoint_title);
		changed = hasBeenInserted;
	}

	if (changed)
	{
		this->tryWriteSettingsToStorage();
	}

}

// todo std::unordered_set
void Settings::removeBlockedEndpoint(std::string endpoint_title) {
	bool changed = false;

	if (this->_dropship_app_settings.options.whitelist_only)
	{
		auto [_position, hasBeenInserted] = this->_dropship_app_settings.config.allowed_endpoints.insert(endpoint_title);
		changed = hasBeenInserted;
	}
	else
	{
		auto const num_removed = this->_dropship_app_settings.config.blocked_endpoints.erase(endpoint_title);
		changed = num_removed > 0;
	}

	if (changed)
	{
		this->tryWriteSettingsToStorage();
	}

}

void Settings::render() {
	if (g_window_watcher) {

		const auto game_open = (*g_window_watcher).isActive();

		// pause config writes when game is open
		if (game_open) {
			//this->_waiting_for_config_write = true;
		}

		// trigger a write when game is closed
		else
		{
			if (this->_waiting_for_config_write)
			{
				this->_waiting_for_config_write = false;
				this->tryWriteSettingsToStorage();
			}
		}

	}
}

void Settings::renderWaitingStatus()
{
	if (g_window_watcher) {

		if (this->_waiting_for_config_write)
		{
			//static auto& style = ImGui::GetStyle();
			ImDrawList* list = ImGui::GetWindowDrawList();

			static const ImU32 text_color = ImGui::ColorConvertFloat4ToU32({ 0.46f, 0.29f, 0.06f, 1.0f });
			static const ImU32 background = ImGui::ColorConvertFloat4ToU32({ 0.99f, 0.86f, 0.58f, 0.46f });
			static const ImU32 border = ImGui::ColorConvertFloat4ToU32({ 0.90f, 0.61f, 0.13f, 0.55f });
			static const std::string text = "RESTART OVERWATCH TO APPLY CHANGES";
			//static const std::string text = "GAME SHUT DOWN REQUIRED";

			static const auto font = font_subtitle;
			static const auto font_size = font->CalcTextSizeA(font_subtitle->FontSize, FLT_MAX, 0.0f, text.c_str());


			ImGui::Dummy({ ImGui::GetContentRegionAvail().x, font_size.y + 16 });

			list->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), background, 12.0f);
			list->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), border, 12.0f, 0, 1.0f);

			{
				static const auto color = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, 0.12f });
				const auto pos = ImGui::GetItemRectMin();

				static const auto image = _get_image("background_diagonal");

				list->PushClipRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
				list->AddImage(image.texture, pos, pos + ImVec2((float)image.width, (float)image.height), ImVec2(0, 0), ImVec2(1, 1), color);
				list->PopClipRect();
			}

			const auto pos = ImGui::GetItemRectMin() + ImVec2((ImGui::GetItemRectSize().x - font_size.x) / 2, 8 - 2);
			list->AddText(font_subtitle, font_subtitle->FontSize, pos, text_color, text.c_str());

		}

		/*static auto& style = ImGui::GetStyle();
		ImGui::PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_TextDisabled]);
		ImGui::TextWrapped("Click Unblock if not connecting");
		ImGui::PopStyleColor();*/
	}

	else {
		ImGui::TextDisabled("Do not block servers with game open");
	}
}
