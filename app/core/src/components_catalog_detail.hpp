/**
 * \file components_catalog_detail.hpp
 * \brief Parse, format, and merge `config/components.ini` `[settings]` and
 *        `components.list`. `components_config.exe` writes the INI;
 *        `components_editor.exe` writes the list.
 *
 * Runtime enablement uses `components_list_detail.hpp` on `components.list`.
 */
#pragma once

#include "components_list_detail.hpp"

#include <algorithm>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ac::config::components_catalog {

    enum class NewComponentsPolicy {
        prompt,
        on,
        off
    };

    struct Settings {
        NewComponentsPolicy new_components {NewComponentsPolicy::on};
        bool sort_components {true};
        bool remove_missing_components {false};
    };

    struct Entry {
        std::string name;
        bool enabled {true};
    };

    struct Document {
        Settings settings;
        std::vector<Entry> components;
    };

    [[nodiscard]]
    inline std::string ascii_lower(std::string_view value) {
        std::string lowered {value};
        for (char& character : lowered) {
            if (character >= 'A' && character <= 'Z') {
                character = static_cast<char>(character - 'A' + 'a');
            }
        }
        return lowered;
    }

    [[nodiscard]]
    inline const char* to_string(const NewComponentsPolicy policy) noexcept {
        switch (policy) {
        case NewComponentsPolicy::on:
            return "on";
        case NewComponentsPolicy::off:
            return "off";
        case NewComponentsPolicy::prompt:
        default:
            return "prompt";
        }
    }

    [[nodiscard]]
    inline NewComponentsPolicy parse_new_components(
        const std::string_view value
    ) noexcept {
        const auto lowered = ascii_lower(value);
        if (lowered == "on") {
            return NewComponentsPolicy::on;
        }
        if (lowered == "off") {
            return NewComponentsPolicy::off;
        }
        if (lowered == "prompt") {
            return NewComponentsPolicy::prompt;
        }
        return NewComponentsPolicy::prompt;
    }

    [[nodiscard]]
    inline bool parse_setting_on_off(
        const std::string_view value,
        const bool fallback
    ) noexcept {
        const auto lowered = ascii_lower(value);
        if (lowered == "on") {
            return true;
        }
        if (lowered == "off") {
            return false;
        }
        return fallback;
    }

    [[nodiscard]]
    inline bool listed(
        const Document& document,
        const std::string_view name
    ) noexcept {
        for (const auto& entry : document.components) {
            if (entry.name == name) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]]
    inline Settings parse_settings(const std::string_view text) {
        Settings settings;
        std::string current_section;
        std::size_t line_start = 0;

        while (line_start <= text.size()) {
            const auto line_end = text.find('\n', line_start);
            const auto length = line_end == std::string_view::npos
                ? text.size() - line_start
                : line_end - line_start;
            const std::string_view line = components_list::trim_list_line(
                text.substr(line_start, length)
            );

            if (!line.empty() && !line.starts_with('#') &&
                !line.starts_with(';')) {
                if (line.starts_with('[') && line.ends_with(']')) {
                    current_section = std::string {
                        components_list::trim_list_line(
                            line.substr(1, line.size() - 2)
                        )
                    };
                }
                else if (current_section == "settings") {
                    const auto equals = line.find('=');
                    if (equals != std::string_view::npos) {
                        const auto key = components_list::trim_list_line(
                            line.substr(0, equals)
                        );
                        const auto value = components_list::trim_list_line(
                            line.substr(equals + 1)
                        );
                        if (key == "new_components") {
                            settings.new_components =
                                parse_new_components(value);
                        }
                        else if (key == "sort_components") {
                            settings.sort_components = parse_setting_on_off(
                                value,
                                settings.sort_components
                            );
                        }
                        else if (key == "remove_missing_components") {
                            settings.remove_missing_components =
                                parse_setting_on_off(
                                    value,
                                    settings.remove_missing_components
                                );
                        }
                    }
                }
            }

            if (line_end == std::string_view::npos) {
                break;
            }
            line_start = line_end + 1;
        }

        return settings;
    }

    [[nodiscard]]
    inline Document parse_list(const std::string_view text) {
        Document document;
        const auto parsed = components_list::parse(text);
        for (const auto& name : parsed.listed) {
            const bool on = components_list::is_special_name(name)
                ? components_list::special_enabled(parsed, name)
                : std::ranges::find(parsed.enabled, name) !=
                    parsed.enabled.end();
            document.components.push_back(
                {.name = name, .enabled = on}
            );
        }
        return document;
    }

    [[nodiscard]]
    inline std::string format_settings(const Settings& settings) {
        std::string text;
        text += "[settings]\n";
        text += "new_components = ";
        text += to_string(settings.new_components);
        text += "\n";
        text += "sort_components = ";
        text += settings.sort_components ? "on" : "off";
        text += "\n";
        text += "remove_missing_components = ";
        text += settings.remove_missing_components ? "on" : "off";
        text += "\n";
        return text;
    }

    [[nodiscard]]
    inline std::string format_list(const Document& document) {
        std::string text =
            "# Leave blank for on; use explicit on/off to override\n"
            "[components]\n";
        for (const auto& entry : document.components) {
            text += entry.name;
            if (!entry.enabled) {
                text += " off";
            }
            text += "\n";
        }
        return text;
    }

    [[nodiscard]]
    inline std::string format(const Document& document) {
        return format_settings(document.settings);
    }

    /**
     * \brief Rewrites one `[components]` entry to explicit `name on` or
     *        `name off`. Other lines, comments, and order stay as they are.
     *        A missing name is appended at the end of that section.
     */
    [[nodiscard]]
    inline std::optional<std::string> set_listed_state(
        const std::string_view text,
        const std::string_view name,
        const bool enabled
    ) {
        if (!components_list::is_valid_component_name(name)) {
            return std::nullopt;
        }

        const std::string replacement =
            std::string {name} + (enabled ? " on" : " off");
        std::string output;
        output.reserve(text.size() + replacement.size() + 16);
        bool in_components = false;
        bool saw_components = false;
        bool found = false;
        std::string_view line_break = "\n";

        const auto append_replacement = [&]() {
            if (!output.empty() && output.back() != '\n') {
                output.append(line_break);
            }
            output += replacement;
            output.append(line_break);
        };

        std::size_t line_start = 0;
        while (line_start <= text.size()) {
            if (line_start == text.size() &&
                line_start != 0 &&
                text.back() == '\n') {
                break;
            }

            const auto line_end = text.find('\n', line_start);
            const auto raw_length = line_end == std::string_view::npos
                ? text.size() - line_start
                : line_end - line_start;
            auto raw = text.substr(line_start, raw_length);
            const bool crlf = !raw.empty() && raw.back() == '\r';
            if (crlf) {
                raw.remove_suffix(1);
            }
            if (line_end != std::string_view::npos) {
                line_break = crlf ? "\r\n" : "\n";
            }

            const auto trimmed = components_list::trim_list_line(raw);
            const bool skip = trimmed.empty() ||
                trimmed.starts_with('#') ||
                trimmed.starts_with(';');
            const bool section = !skip &&
                trimmed.starts_with('[') &&
                trimmed.ends_with(']');

            if (section) {
                const auto section_name = components_list::trim_list_line(
                    trimmed.substr(1, trimmed.size() - 2)
                );
                if (in_components && !found) {
                    append_replacement();
                    found = true;
                }
                in_components = section_name == "components";
                saw_components = saw_components || in_components;
            }
            else if (in_components && !skip) {
                const auto split = trimmed.find_first_of(" \t");
                const auto entry_name = split == std::string_view::npos
                    ? trimmed
                    : components_list::trim_list_line(trimmed.substr(0, split));
                if (entry_name == name) {
                    output += replacement;
                    if (line_end != std::string_view::npos) {
                        output.append(line_break);
                    }
                    found = true;
                    if (line_end == std::string_view::npos) {
                        break;
                    }
                    line_start = line_end + 1;
                    continue;
                }
            }

            output.append(text.data() + line_start, raw_length);
            if (line_end == std::string_view::npos) {
                break;
            }
            output.push_back('\n');
            line_start = line_end + 1;
        }

        if (!found) {
            if (!saw_components) {
                if (!output.empty() && output.back() != '\n') {
                    output.append(line_break);
                }
                output += "[components]";
                output.append(line_break);
            }
            append_replacement();
        }
        return output;
    }

    inline void apply_sort(Document& document) {
        if (!document.settings.sort_components) {
            return;
        }
        std::ranges::sort(
            document.components,
            {},
            &Entry::name
        );
    }

    template<typename Resolve>
    void add_if_missing(
        Document& document,
        const std::string_view name,
        Resolve&& resolve_enabled
    ) {
        if (!components_list::is_valid_component_name(name) ||
            listed(document, name)) {
            return;
        }
        document.components.push_back(
            {.name = std::string {name}, .enabled = resolve_enabled(name)}
        );
    }

    template<typename Resolve>
    void add_discovered(
        Document& document,
        const std::vector<std::string>& discovered,
        Resolve&& resolve_enabled
    ) {
        for (const auto& name : discovered) {
            add_if_missing(document, name, resolve_enabled);
        }
    }

    inline void remove_missing(
        Document& document,
        const std::vector<std::string>& discovered
    ) {
        if (!document.settings.remove_missing_components) {
            return;
        }
        std::erase_if(document.components, [&](const Entry& entry) {
            return std::ranges::find(discovered, entry.name) ==
                discovered.end();
        });
    }

    template<typename Resolve>
    void full_sync(
        Document& document,
        const std::vector<std::string>& discovered,
        Resolve&& resolve_enabled
    ) {
        remove_missing(document, discovered);
        add_discovered(document, discovered, resolve_enabled);
        apply_sort(document);
    }

    template<typename Resolve>
    void single_update(
        Document& document,
        const std::string_view name,
        Resolve&& resolve_enabled
    ) {
        add_if_missing(document, name, resolve_enabled);
        apply_sort(document);
    }

} // namespace ac::config::components_catalog
