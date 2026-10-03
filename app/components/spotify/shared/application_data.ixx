/**
 * \file application_data.ixx
 * \brief Parse and serialize `client.id` and `devices.list`.
 *
 * This module does not replace those files on disk. `tokens.map` lives in
 * `spotify_token_store`, which the editor does not link.
 */
export module spotify_application_data;

import std;
import auto_core.core.paths;

export namespace spotify::data {

struct Device {
    std::string name;
    std::string id;
};

[[nodiscard]] inline std::filesystem::path client_id_path() {
    return ac::paths::spotify_directory() / "client.id";
}

[[nodiscard]] inline std::filesystem::path devices_list_path() {
    return ac::paths::spotify_directory() / "devices.list";
}

[[nodiscard]] inline std::string_view trim_ws(std::string_view value) noexcept {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

[[nodiscard]] inline std::string ascii_lower(std::string text) {
    for (char& character : text) {
        character = static_cast<char>(
            std::tolower(static_cast<unsigned char>(character))
        );
    }
    return text;
}

/**
 * \brief The whole file is the client id.
 * \return Empty when the text is missing, empty, or contains an embedded newline.
 */
[[nodiscard]] inline std::optional<std::string> parse_client_id(
    std::string_view text
) {
    const auto trimmed = trim_ws(text);
    if (trimmed.empty()) {
        return std::nullopt;
    }
    if (trimmed.find('\n') != std::string_view::npos ||
        trimmed.find('\r') != std::string_view::npos) {
        return std::nullopt;
    }
    return std::string {trimmed};
}

[[nodiscard]] inline std::optional<std::string> load_client_id() {
    std::ifstream input(client_id_path(), std::ios::binary);
    if (!input) {
        return std::nullopt;
    }
    std::ostringstream contents;
    contents << input.rdbuf();
    return parse_client_id(contents.str());
}

/**
 * \brief Parses `name = id` lines. Splits on the last `=`.
 * Names are lowercased before duplicate comparison. The last id wins.
 * Unusable lines are skipped.
 */
[[nodiscard]] inline std::vector<Device> parse_devices(std::string_view text) {
    std::vector<Device> devices;
    std::size_t cursor = 0;
    while (cursor <= text.size()) {
        const auto end = text.find('\n', cursor);
        auto line = text.substr(
            cursor,
            end == std::string_view::npos ? std::string_view::npos : end - cursor
        );
        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        }
        cursor = end == std::string_view::npos ? text.size() + 1 : end + 1;

        if (trim_ws(line).empty()) {
            continue;
        }
        const auto split = line.rfind('=');
        if (split == std::string_view::npos) {
            continue;
        }
        const auto name = ascii_lower(std::string {trim_ws(line.substr(0, split))});
        const auto id = std::string {trim_ws(line.substr(split + 1))};
        if (name.empty() || id.empty()) {
            continue;
        }
        const auto existing = std::find_if(
            devices.begin(),
            devices.end(),
            [&](const Device& device) { return device.name == name; }
        );
        if (existing == devices.end()) {
            devices.push_back(Device {name, id});
        }
        else {
            existing->id = id;
        }
        if (end == std::string_view::npos) {
            break;
        }
    }
    return devices;
}

[[nodiscard]] inline std::string serialize_devices(
    std::span<const Device> devices
) {
    std::ostringstream output;
    for (const Device& device : devices) {
        output << device.name << " = " << device.id << '\n';
    }
    return output.str();
}

[[nodiscard]] inline std::vector<Device> load_devices() {
    std::ifstream input(devices_list_path(), std::ios::binary);
    if (!input) {
        return {};
    }
    std::ostringstream contents;
    contents << input.rdbuf();
    return parse_devices(contents.str());
}

} // namespace spotify::data
