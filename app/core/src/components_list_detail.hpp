/**
 * \file components_list_detail.hpp
 * \brief Pure parse of `components.list` `[components]` and `*_ac.exe` discovery.
 *
 * Shared by the DLL logging configuration and Main. Included by
 * Catch2 tests. Not a module interface.
 *
 * Names are case-sensitive and lowercase-only. Invalid names are not
 * normalized. `discover_ac_executables` returns every valid `*_ac.exe`
 * name, including known specials (`dash`, `slash`). Callers split those
 * two into `ParseResult.specials`; they are never v1 host children.
 */
#pragma once

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ac::config::components_list {

    inline constexpr std::array<std::string_view, 2> specials {
        "dash",
        "slash"
    };

    struct MalformedValue {
        std::string name;
        std::string value;
    };

    struct ParseResult {
        bool ok {false};
        std::string error;
        std::vector<std::string> enabled;
        std::vector<std::string> specials;
        std::vector<std::string> listed;
        std::vector<std::string> invalid_names;
        std::vector<MalformedValue> malformed_values;
    };

    struct RuntimeCatalog {
        ParseResult result;
        bool used_discovery {false};
        std::string io_error;
    };

    [[nodiscard]]
    inline bool is_special_name(const std::string_view name) noexcept {
        for (const auto special : specials) {
            if (special == name) {
                return true;
            }
        }
        return false;
    }

    enum class ListedEnablement {
        enabled,
        disabled,
        unavailable
    };

    [[nodiscard]]
    inline bool special_enabled(
        const ParseResult& result,
        const std::string_view name
    ) noexcept {
        if (!result.ok) {
            return false;
        }
        for (const auto& enabled : result.specials) {
            if (enabled == name) {
                return true;
            }
        }
        return false;
    }

    /**
     * \brief Classifies one name from an already loaded parse.
     *
     * `!result.ok` is unavailable. A successful parse treats blank or `on`
     * as enabled. `off`, a malformed value, or a missing name is disabled.
     */
    [[nodiscard]]
    inline ListedEnablement listed_enablement(
        const ParseResult& result,
        const std::string_view name
    ) noexcept {
        if (!result.ok) {
            return ListedEnablement::unavailable;
        }
        const bool on = is_special_name(name)
            ? special_enabled(result, name)
            : std::ranges::find(result.enabled, name) != result.enabled.end();
        return on ? ListedEnablement::enabled : ListedEnablement::disabled;
    }

    [[nodiscard]]
    inline bool is_valid_component_name(const std::string_view name) noexcept {
        if (name.empty() || name.front() < 'a' || name.front() > 'z') {
            return false;
        }
        for (const char character : name) {
            const bool letter = character >= 'a' && character <= 'z';
            const bool digit = character >= '0' && character <= '9';
            if (!letter && !digit && character != '_') {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]]
    inline bool name_is_listed(
        const ParseResult& result,
        const std::string_view name
    ) noexcept {
        for (const auto& listed : result.listed) {
            if (listed == name) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]]
    inline std::string_view trim_list_line(const std::string_view value) noexcept {
        const auto first = value.find_first_not_of(" \t\r");
        if (first == std::string_view::npos) {
            return {};
        }
        const auto last = value.find_last_not_of(" \t\r");
        return value.substr(first, last - first + 1);
    }

    inline void record_unique(
        std::vector<std::string>& names,
        const std::string_view name
    ) {
        for (const auto& existing : names) {
            if (existing == name) {
                return;
            }
        }
        names.emplace_back(name);
    }

    [[nodiscard]]
    inline bool has_ac_exe_suffix(const std::string_view filename) noexcept {
        constexpr std::string_view suffix = "_ac.exe";
        if (filename.size() <= suffix.size()) {
            return false;
        }
        const auto tail = filename.substr(filename.size() - suffix.size());
        for (std::size_t i = 0; i < suffix.size(); ++i) {
            char character = tail[i];
            if (character >= 'A' && character <= 'Z') {
                character = static_cast<char>(character - 'A' + 'a');
            }
            if (character != suffix[i]) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]]
    inline ParseResult catalog_from_names(const std::vector<std::string>& names) {
        ParseResult result;
        result.ok = true;
        for (const auto& name : names) {
            if (!is_valid_component_name(name)) {
                continue;
            }
            record_unique(result.listed, name);
            if (is_special_name(name)) {
                record_unique(result.specials, name);
            }
            else {
                record_unique(result.enabled, name);
            }
        }
        return result;
    }

    [[nodiscard]]
    inline std::vector<std::string> discover_ac_executables(
        const std::filesystem::path& directory
    ) {
        std::vector<std::string> names;
        std::error_code error;
        const auto iterator = std::filesystem::directory_iterator(
            directory,
            error
        );
        if (error) {
            return names;
        }

        for (const auto& entry : iterator) {
            std::error_code status_error;
            if (!entry.is_regular_file(status_error) || status_error) {
                continue;
            }

            const auto u8_name = entry.path().filename().u8string();
            const std::string filename(u8_name.begin(), u8_name.end());
            if (!has_ac_exe_suffix(filename)) {
                continue;
            }

            const auto name = filename.substr(0, filename.size() - 7);
            if (!is_valid_component_name(name)) {
                continue;
            }
            record_unique(names, name);
        }

        std::ranges::sort(names);
        return names;
    }

    [[nodiscard]]
    inline ParseResult parse(const std::string_view text) {
        ParseResult result;
        std::vector<std::pair<std::string, std::string>> entries;
        std::string current_section;
        std::size_t line_start = 0;

        const auto upsert = [&](
            const std::string_view name,
            const std::string_view value
        ) {
            for (auto& entry : entries) {
                if (entry.first == name) {
                    entry.second = std::string {value};
                    return;
                }
            }
            entries.emplace_back(name, value);
        };

        while (line_start <= text.size()) {
            const auto line_end = text.find('\n', line_start);
            const auto length = line_end == std::string_view::npos
                ? text.size() - line_start
                : line_end - line_start;
            const std::string_view line =
                trim_list_line(text.substr(line_start, length));

            if (!line.empty() && !line.starts_with('#') &&
                !line.starts_with(';')) {
                if (line.starts_with('[') && line.ends_with(']')) {
                    current_section = std::string {
                        trim_list_line(line.substr(1, line.size() - 2))
                    };
                }
                else if (current_section == "components") {
                    const auto split = line.find_first_of(" \t");
                    const auto name = split == std::string_view::npos
                        ? line
                        : trim_list_line(line.substr(0, split));
                    const auto value = split == std::string_view::npos
                        ? std::string_view {}
                        : trim_list_line(line.substr(split + 1));
                    if (!is_valid_component_name(name)) {
                        record_unique(result.invalid_names, name);
                    }
                    else {
                        upsert(name, value);
                    }
                }
            }

            if (line_end == std::string_view::npos) {
                break;
            }
            line_start = line_end + 1;
        }

        for (const auto& entry : entries) {
            record_unique(result.listed, entry.first);
            const bool on = entry.second.empty() || entry.second == "on";
            if (on) {
                auto& names = is_special_name(entry.first)
                    ? result.specials
                    : result.enabled;
                names.push_back(entry.first);
            }
            else if (entry.second != "off") {
                result.malformed_values.push_back(
                    {.name = entry.first, .value = entry.second}
                );
            }
        }

        result.ok = true;
        return result;
    }

    [[nodiscard]]
    inline ParseResult load(const std::filesystem::path& path) {
        std::ifstream input(path, std::ios::binary);
        if (!input) {
            ParseResult result;
            result.error =
                "Unable to open components.list. Run "
                "components_editor.exe to generate it. Optional components "
                "are disabled; the file will not be created.";
            return result;
        }

        const std::string bytes(
            (std::istreambuf_iterator<char>(input)),
            std::istreambuf_iterator<char>()
        );
        if (!input && !input.eof()) {
            ParseResult result;
            result.error =
                "Unable to read components.list. Run "
                "components_editor.exe to generate it. Optional components "
                "are disabled; the file will not be created.";
            return result;
        }

        return parse(bytes);
    }

    [[nodiscard]]
    inline RuntimeCatalog load_runtime_catalog(
        const std::filesystem::path& list_path,
        const std::filesystem::path& bin_directory
    ) {
        RuntimeCatalog catalog;
        catalog.result = load(list_path);
        if (catalog.result.ok) {
            return catalog;
        }

        catalog.io_error = catalog.result.error;
        catalog.used_discovery = true;
        catalog.result = catalog_from_names(
            discover_ac_executables(bin_directory)
        );
        return catalog;
    }

} // namespace ac::config::components_list
