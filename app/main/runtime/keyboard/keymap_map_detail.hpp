/**
 * \file keymap_map_detail.hpp
 * \brief Parse and format `keymap.map` lines (`key = primary | secondary`).
 */
#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace ac::keymap::map_file {

    [[nodiscard]]
    inline std::string_view trim(const std::string_view value) noexcept {
        const auto first = value.find_first_not_of(" \t\r\n");
        if (first == std::string_view::npos) {
            return {};
        }
        const auto last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1);
    }

    [[nodiscard]]
    inline std::size_t find_delimiter(
        const std::string_view value,
        const char delimiter
    ) noexcept {
        int parenthesis_depth = 0;
        bool inside_quotes = false;

        for (std::size_t index = 0; index < value.size(); ++index) {
            const char character = value[index];

            if (character == '"' &&
                (index == 0 || value[index - 1] != '\\')) {
                inside_quotes = !inside_quotes;
                continue;
            }

            if (inside_quotes) {
                continue;
            }

            if (character == '(') {
                ++parenthesis_depth;
            }
            else if (character == ')') {
                if (parenthesis_depth == 0) {
                    return std::string_view::npos;
                }
                --parenthesis_depth;
            }
            else if (character == delimiter && parenthesis_depth == 0) {
                return index;
            }
        }

        return std::string_view::npos;
    }

    struct Mapping {
        std::string_view key;
        std::string_view primary;
        std::string_view secondary;
    };

    [[nodiscard]]
    inline bool is_ignored_line(const std::string_view trimmed) noexcept {
        return trimmed.empty() ||
            trimmed.starts_with(';') ||
            trimmed.starts_with('#') ||
            (trimmed.size() >= 2 &&
                trimmed.front() == '[' &&
                trimmed.back() == ']');
    }

    enum class Slot { primary, secondary };

    /**
     * \brief `primary` and `secondary` are unset only in their own slot.
     * The other word in that slot is invalid syntax.
     */
    [[nodiscard]]
    inline std::optional<std::string_view> unset_placeholder(
        const std::string_view side,
        const Slot slot
    ) noexcept {
        if (side == "primary") {
            if (slot != Slot::primary) {
                return std::nullopt;
            }
            return std::string_view {};
        }
        if (side == "secondary") {
            if (slot != Slot::secondary) {
                return std::nullopt;
            }
            return std::string_view {};
        }
        return side;
    }

    [[nodiscard]]
    inline std::optional<Mapping> sides_from_actions(
        const std::string_view key,
        const std::string_view actions,
        const char delimiter
    ) {
        const auto split = find_delimiter(actions, delimiter);
        if (split == std::string_view::npos) {
            if (!actions.empty()) {
                return std::nullopt;
            }
            return Mapping {.key = key, .primary = {}, .secondary = {}};
        }

        const auto remainder = actions.substr(split + 1);
        if (find_delimiter(remainder, delimiter) != std::string_view::npos) {
            return std::nullopt;
        }

        const auto primary = unset_placeholder(
            trim(actions.substr(0, split)),
            Slot::primary
        );
        const auto secondary = unset_placeholder(
            trim(remainder),
            Slot::secondary
        );
        if (!primary || !secondary) {
            return std::nullopt;
        }

        return Mapping {
            .key = key,
            .primary = *primary,
            .secondary = *secondary
        };
    }

    [[nodiscard]]
    inline std::optional<Mapping> parse_line(const std::string_view line) {
        const auto trimmed = trim(line);
        if (is_ignored_line(trimmed)) {
            return std::nullopt;
        }

        const auto equals = trimmed.find('=');
        if (equals == std::string_view::npos) {
            return std::nullopt;
        }

        const auto key = trim(trimmed.substr(0, equals));
        if (key.empty()) {
            return std::nullopt;
        }

        return sides_from_actions(
            key,
            trim(trimmed.substr(equals + 1)),
            '|'
        );
    }

    [[nodiscard]]
    inline std::optional<Mapping> parse_legacy_line(const std::string_view line) {
        const auto trimmed = trim(line);
        if (is_ignored_line(trimmed)) {
            return std::nullopt;
        }

        const auto equals = trimmed.find('=');
        if (equals == std::string_view::npos) {
            return std::nullopt;
        }

        const auto opening_brace = trimmed.find('{', equals);
        const auto closing_brace = trimmed.rfind('}');
        if (opening_brace == std::string_view::npos ||
            closing_brace == std::string_view::npos ||
            closing_brace <= opening_brace + 1) {
            return parse_line(trimmed);
        }

        const auto key = trim(trimmed.substr(0, equals));
        if (key.empty()) {
            return std::nullopt;
        }

        const auto actions = trimmed.substr(
            opening_brace + 1,
            closing_brace - opening_brace - 1
        );
        return sides_from_actions(key, trim(actions), ',');
    }

    [[nodiscard]]
    inline std::string format_line(
        const std::string_view key,
        const std::string_view primary,
        const std::string_view secondary
    ) {
        if (primary.empty() && secondary.empty()) {
            return std::string {key} + " =\n";
        }
        return std::string {key} + " = " + std::string {primary} + " | " +
            std::string {secondary} + "\n";
    }

} // namespace ac::keymap::map_file
