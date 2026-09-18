/**
 * \file components_list_detail.hpp
 * \brief Pure parse of `config/components.list`.
 *
 * Shared by the DLL (`logging::config::enabled`) and Main. Included by
 * Catch2 tests. Not a module interface.
 *
 * Names are case-sensitive and lowercase-only. Invalid names are not
 * normalized. Known specials (`logger`, `dash`, `slash`) use the same
 * on/off rules as other names but are never v1 host children.
 */
#pragma once

#include <array>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ac::config::components_list {

    inline constexpr std::array<std::string_view, 3> specials {
        "logger",
        "dash",
        "slash"
    };

    struct ParseResult {
        bool ok {false};
        std::string error;
        std::vector<std::string> enabled;
        std::vector<std::string> specials;
        std::vector<std::string> invalid_names;
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
    inline std::string_view trim_list_line(const std::string_view value) noexcept {
        const auto first = value.find_first_not_of(" \t\r");
        if (first == std::string_view::npos) {
            return {};
        }
        const auto last = value.find_last_not_of(" \t\r");
        return value.substr(first, last - first + 1);
    }

    inline void append_list_tokens(
        const std::string_view line,
        std::vector<std::string_view>& tokens
    ) {
        tokens.clear();
        std::size_t i = 0;
        while (i < line.size()) {
            while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) {
                ++i;
            }
            if (i >= line.size()) {
                break;
            }
            const auto begin = i;
            while (i < line.size() && line[i] != ' ' && line[i] != '\t') {
                ++i;
            }
            tokens.push_back(line.substr(begin, i - begin));
        }
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

    inline void apply_list_entry(
        ParseResult& result,
        std::unordered_map<std::string, bool>& enabled_by_name,
        std::unordered_map<std::string, bool>& seen,
        const std::vector<std::string_view>& tokens
    ) {
        if (tokens.empty()) {
            return;
        }

        const auto name = tokens.front();
        if (!is_valid_component_name(name)) {
            record_unique(result.invalid_names, name);
            return;
        }

        const std::string key {name};
        if (seen[key]) {
            enabled_by_name[key] = false;
            return;
        }
        seen[key] = true;

        auto& names = is_special_name(name) ? result.specials : result.enabled;
        if (tokens.size() == 1 ||
            (tokens.size() == 2 && tokens[1] == "on")) {
            enabled_by_name[key] = true;
            names.push_back(key);
            return;
        }

        enabled_by_name[key] = false;
    }

    [[nodiscard]]
    inline ParseResult parse(const std::string_view text) {
        ParseResult result;
        std::unordered_map<std::string, bool> enabled_by_name;
        std::unordered_map<std::string, bool> seen;
        std::string current_section;
        bool has_section = false;
        std::vector<std::string_view> tokens;
        std::size_t line_start = 0;

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
                    if (current_section == "components") {
                        has_section = true;
                    }
                }
                else if (current_section == "components") {
                    append_list_tokens(line, tokens);
                    apply_list_entry(result, enabled_by_name, seen, tokens);
                }
            }

            if (line_end == std::string_view::npos) {
                break;
            }
            line_start = line_end + 1;
        }

        if (!has_section) {
            result.ok = false;
            result.error =
                "config/components.list is missing a [components] section; "
                "optional components are disabled.";
            result.enabled.clear();
            result.specials.clear();
            result.invalid_names.clear();
            return result;
        }

        std::vector<std::string> enabled;
        enabled.reserve(result.enabled.size());
        for (const auto& name : result.enabled) {
            if (enabled_by_name[name]) {
                enabled.push_back(name);
            }
        }
        result.enabled = std::move(enabled);

        std::vector<std::string> specials;
        specials.reserve(result.specials.size());
        for (const auto& name : result.specials) {
            if (enabled_by_name[name]) {
                specials.push_back(name);
            }
        }
        result.specials = std::move(specials);
        result.ok = true;
        return result;
    }

    [[nodiscard]]
    inline ParseResult load(const std::filesystem::path& path) {
        std::ifstream input(path, std::ios::binary);
        if (!input) {
            ParseResult result;
            result.error =
                "Unable to open config/components.list; optional components "
                "are disabled.";
            return result;
        }

        const std::string bytes(
            (std::istreambuf_iterator<char>(input)),
            std::istreambuf_iterator<char>()
        );
        if (!input && !input.eof()) {
            ParseResult result;
            result.error =
                "Unable to read config/components.list; optional components "
                "are disabled.";
            return result;
        }

        return parse(bytes);
    }

} // namespace ac::config::components_list
