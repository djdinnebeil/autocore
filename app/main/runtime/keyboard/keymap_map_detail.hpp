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
    inline bool is_section_header(const std::string_view trimmed) noexcept {
        return trimmed.size() >= 2 &&
            trimmed.front() == '[' &&
            trimmed.back() == ']';
    }

    [[nodiscard]]
    inline bool is_commands_header(const std::string_view trimmed) noexcept {
        return trimmed == "[commands]";
    }

    [[nodiscard]]
    inline bool is_ignored_line(const std::string_view trimmed) noexcept {
        return trimmed.empty() ||
            trimmed.starts_with(';') ||
            trimmed.starts_with('#') ||
            is_section_header(trimmed);
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

    /**
     * \brief Whether `text` already ends with a blank line.
     *
     * A terminated whitespace-only line counts. The check looks at the end of
     * `text`, so earlier blank lines are left unchanged.
     */
    [[nodiscard]]
    inline bool ends_with_blank_line(const std::string_view text) noexcept {
        if (text.empty() || text.back() != '\n') {
            return false;
        }
        const auto last_newline = text.size() - 1;
        const auto previous = last_newline == 0
            ? std::string_view::npos
            : text.rfind('\n', last_newline - 1);
        const auto line_begin = previous == std::string_view::npos
            ? 0
            : previous + 1;
        auto last_line = text.substr(line_begin, last_newline - line_begin);
        if (!last_line.empty() && last_line.back() == '\r') {
            last_line.remove_suffix(1);
        }
        return trim(last_line).empty();
    }

    /**
     * \brief Inserts one blank line when `prefix` has content and no trailing
     * blank line. An empty prefix is left empty. Existing trailing blank
     * lines are preserved.
     */
    inline void ensure_blank_line(
        std::string& prefix,
        const std::string_view newline
    ) {
        if (prefix.empty()) {
            return;
        }
        if (prefix.back() != '\n') {
            prefix += newline;
        }
        if (!ends_with_blank_line(prefix)) {
            prefix += newline;
        }
    }

} // namespace ac::keymap::map_file
