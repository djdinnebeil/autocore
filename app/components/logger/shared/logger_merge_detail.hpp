/**
 * \file logger_merge_detail.hpp
 * \brief Pure resolution of `logger.ini` merge keys.
 */
#pragma once

#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace ac::logger::config::detail {

    struct MergeRaw {
        std::optional<std::string_view> merge_interval_seconds;
        std::optional<std::string_view> merge_logs_on_shutdown;
    };

    struct MergeSettings {
        std::uint64_t merge_interval_seconds;
        bool merge_logs_on_shutdown;
        std::string report;
    };

    [[nodiscard]]
    inline std::optional<bool> parse_on_off(
        const std::optional<std::string_view> value
    ) noexcept {
        if (value == "on") {
            return true;
        }
        if (value == "off") {
            return false;
        }
        return std::nullopt;
    }

    [[nodiscard]]
    inline std::optional<std::uint64_t> parse_interval(
        const std::optional<std::string_view> value
    ) noexcept {
        if (!value || value->empty()) {
            return std::nullopt;
        }
        std::int64_t parsed = 0;
        const auto* const end = value->data() + value->size();
        const auto result = std::from_chars(value->data(), end, parsed);
        if (result.ec != std::errc {} || result.ptr != end || parsed < 0) {
            return std::nullopt;
        }
        return static_cast<std::uint64_t>(parsed);
    }

    /**
     * \brief Resolves merge keys.
     *
     * Interval `0` is kept. A missing or invalid interval uses 60.
     * Shutdown merge defaults to on. Boolean values are `on` and `off`.
     */
    [[nodiscard]]
    inline MergeSettings resolve_merge(const MergeRaw& raw) {
        MergeSettings settings {
            .merge_interval_seconds = 60,
            .merge_logs_on_shutdown = true,
            .report = "Logger configuration:\n"
        };

        if (const auto value = parse_interval(raw.merge_interval_seconds)) {
            settings.merge_interval_seconds = *value;
        }
        else {
            settings.report +=
                "merge_interval_seconds missing or invalid; using 60\n";
        }
        settings.report += "merge_interval_seconds = " +
            std::to_string(settings.merge_interval_seconds) + "\n";

        if (const auto value = parse_on_off(raw.merge_logs_on_shutdown)) {
            settings.merge_logs_on_shutdown = *value;
        }
        else {
            settings.report +=
                "merge_logs_on_shutdown missing or invalid; using on\n";
        }
        settings.report += settings.merge_logs_on_shutdown
            ? "merge_logs_on_shutdown = on\n"
            : "merge_logs_on_shutdown = off\n";

        settings.report += "logger settings loaded\n";
        return settings;
    }

} // namespace ac::logger::config::detail
