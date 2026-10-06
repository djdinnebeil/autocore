/**
 * \file logger_merge_config.ixx
 * \brief Cached `config/logger.ini` merge policy for Logger and Main.
 *
 * The file is read once per process and is not written here.
 */
module;

#include "logger_merge_detail.hpp"

export module logger_merge_config;

import std;
import auto_core.core.ini;
import auto_core.core.paths;

namespace {

    struct Cache {
        std::uint64_t merge_interval_seconds = 60;
        bool merge_logs_on_shutdown = true;
        bool ini_missing = false;
        std::string report = "Logger configuration:\n";

        Cache() {
            const auto ini_path =
                ac::paths::config_directory() / "logger.ini";
            const auto document = ac::ini::read(ini_path);
            if (!document) {
                std::error_code exists_error;
                const bool present =
                    std::filesystem::exists(ini_path, exists_error);
                ini_missing = !present && !exists_error;
                report += ini_missing
                    ? "logger.ini unavailable; using defaults\n"
                    : "logger.ini unreadable; using defaults\n";
                report += "merge_interval_seconds = 60\n";
                report += "merge_logs_on_shutdown = on\n";
                return;
            }

            const auto resolved = ac::logger::config::detail::resolve_merge({
                .merge_interval_seconds = document->find(
                    "logger", "merge_interval_seconds"
                ),
                .merge_logs_on_shutdown = document->find(
                    "logger", "merge_logs_on_shutdown"
                )
            });
            merge_interval_seconds = resolved.merge_interval_seconds;
            merge_logs_on_shutdown = resolved.merge_logs_on_shutdown;
            report = resolved.report;
        }
    };

    const Cache& cache() {
        static const Cache value;
        return value;
    }

} // namespace

export namespace ac::logger::config {

    [[nodiscard]]
    std::uint64_t merge_interval_seconds() {
        return cache().merge_interval_seconds;
    }

    [[nodiscard]]
    bool merge_logs_on_shutdown() {
        return cache().merge_logs_on_shutdown;
    }

    [[nodiscard]]
    bool ini_missing() {
        return cache().ini_missing;
    }

    [[nodiscard]]
    std::string_view configuration_report() {
        return cache().report;
    }

} // namespace ac::logger::config
