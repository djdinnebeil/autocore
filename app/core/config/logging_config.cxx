module;

#include "logging_config_detail.hpp"

/**
 * \file logging_config.cxx
 * \brief Loads logging_policy.ini for Auto Core processes.
 */
module auto_core.core.logging.config;

import std;
import auto_core.core.encoding;
import auto_core.core.ini;
import auto_core.core.paths;

namespace ac::logging::config {

    namespace {
        ac::logging::config::LogLevel to_public_level(
            const detail::LogLevel level
        ) noexcept {
            switch (level) {
            case detail::LogLevel::trace:
                return LogLevel::trace;
            case detail::LogLevel::debug:
                return LogLevel::debug;
            case detail::LogLevel::info:
                return LogLevel::info;
            case detail::LogLevel::alert:
                return LogLevel::alert;
            }
            return LogLevel::debug;
        }

        detail::LoggingFallback to_detail_fallback(
            const LoggingFallback fallback
        ) noexcept {
            return fallback == LoggingFallback::off
                ? detail::LoggingFallback::off
                : detail::LoggingFallback::global_default;
        }

        struct SharedData {
            bool disable_all = false;
            LogLevel level = LogLevel::debug;
            bool log_console_output = true;
            bool write_logs_to_files = true;
            bool write_logs_to_console = false;
            bool write_alerts_to_console = true;
            bool component_logging_default = true;
            std::int64_t max_directory_mb = 100;
            std::int64_t retention_days = 30;
            std::filesystem::path directory = ac::paths::log_directory();
            std::filesystem::path components_directory =
                directory / "components";
            bool ini_missing = false;
            std::string report;
            bool experimental = true;

            SharedData() {
                const auto list_path = ac::paths::components_list_file();
                std::error_code list_error;
                const bool list_present =
                    std::filesystem::exists(list_path, list_error);
                const auto catalog_note =
                    (!list_present && !list_error)
                        ? std::string {
                            "components.list is missing. Run "
                            "components_editor.exe to generate it. The file "
                            "will not be created.\n"
                        }
                        : std::string {};

                const auto ini_path =
                    ac::paths::config_directory() / "logging_policy.ini";
                const auto document = ac::ini::read(ini_path);
                if (!document) {
                    std::error_code exists_error;
                    const bool present =
                        std::filesystem::exists(ini_path, exists_error);
                    ini_missing = !present && !exists_error;
                    const detail::LoggingSettings resolved = detail::resolve_logging(
                        {},
                        ac::paths::log_directory(),
                        ac::paths::installation_root()
                    );
                    disable_all = resolved.disable_all;
                    level = to_public_level(resolved.level);
                    log_console_output = resolved.log_console_output;
                    write_logs_to_files = resolved.write_logs_to_files;
                    write_logs_to_console = resolved.write_logs_to_console;
                    write_alerts_to_console = resolved.write_alerts_to_console;
                    component_logging_default = resolved.component_logging_default;
                    max_directory_mb = resolved.max_directory_mb;
                    retention_days = resolved.retention_days;
                    directory = resolved.directory;
                    components_directory = resolved.components_directory;
                    report = ini_missing
                        ? "logging_policy.ini unavailable; using defaults\n"
                        : "logging_policy.ini unreadable; using defaults\n";
                    report += resolved.report;
                    report += catalog_note;
                    experimental = resolved.experimental;
                    return;
                }

                const auto directory_value =
                    document->find("logging", "directory");
                const detail::LoggingSettings resolved = detail::resolve_logging(
                    {
                        .disable_all = document->find("logging", "disable_all"),
                        .level = document->find("logging", "level"),

                        .log_console_output = document->find(
                            "logging", "log_console_output"
                        ),
                        .write_logs_to_files = document->find(
                            "logging", "write_logs_to_files"
                        ),
                        .write_logs_to_console = document->find(
                            "logging", "write_logs_to_console"
                        ),
                        .write_alerts_to_console = document->find(
                            "logging", "write_alerts_to_console"
                        ),
                        .component_logging_default = document->find(
                            "logging", "component_logging_default"
                        ),
                        .max_directory_mb = document->find(
                            "logging", "max_directory_mb"
                        ),
                        .retention_days = document->find(
                            "logging", "retention_days"
                        ),
                        .directory = directory_value
                            ? std::optional<std::filesystem::path> {
                                ac::encoding::to_utf16(*directory_value)
                            }
                            : std::nullopt,
                        .experimental = document->find(
                            "logging", "experimental"
                        )
                    },
                    ac::paths::log_directory(),
                    ac::paths::installation_root()
                );
                disable_all = resolved.disable_all;
                level = to_public_level(resolved.level);
                log_console_output = resolved.log_console_output;
                write_logs_to_files = resolved.write_logs_to_files;
                write_logs_to_console = resolved.write_logs_to_console;
                write_alerts_to_console = resolved.write_alerts_to_console;
                component_logging_default = resolved.component_logging_default;
                max_directory_mb = resolved.max_directory_mb;
                retention_days = resolved.retention_days;
                directory = resolved.directory;
                components_directory = resolved.components_directory;
                report = resolved.report;
                report += catalog_note;
                experimental = resolved.experimental;
            }
        };

        const SharedData& shared_data() {
            static const SharedData value;
            return value;
        }

        struct FamilyCache {
            std::mutex mutex;
            std::vector<std::pair<std::string, bool>> entries;
        };

        FamilyCache& family_cache() {
            static FamilyCache cache;
            return cache;
        }

        std::string family_key(const LoggingScope& scope) {
            std::string key {scope.name};
            key.push_back('\n');
            key.push_back(
                scope.fallback == LoggingFallback::off ? '0' : '1'
            );
            return key;
        }

        bool read_family_logging(const LoggingScope& scope) {
            const auto ini_path = scope.name == "auto_core"
                ? ac::paths::config_directory() / "auto_core.ini"
                : ac::paths::component_config_file(scope.name);
            const auto document = ac::ini::read(ini_path);
            std::optional<std::string> owned;
            std::optional<std::string_view> logging;
            if (document) {
                if (const auto value = document->find(scope.name, "logging")) {
                    owned = std::string {*value};
                    logging = *owned;
                }
            }
            return detail::resolve_component_logging(
                to_detail_fallback(scope.fallback),
                logging,
                shared_data().component_logging_default
            );
        }
    }

    void resolve() {
        (void)shared_data();
    }

    bool disable_all() {
        return shared_data().disable_all;
    }

    bool experimental() {
        return shared_data().experimental;
    }

    bool log_console_output() {
        return shared_data().log_console_output;
    }

    bool write_logs_to_files() {
        return shared_data().write_logs_to_files;
    }

    bool write_logs_to_console() {
        return shared_data().write_logs_to_console;
    }

    LogLevel level() {
        return shared_data().level;
    }

    bool write_alerts_to_console() {
        return shared_data().write_alerts_to_console;
    }

    bool component_logging_default() {
        return shared_data().component_logging_default;
    }

    bool component_logging_enabled(const LoggingScope& scope) {
        if (shared_data().disable_all) {
            return false;
        }

        auto& cache = family_cache();
        const auto key = family_key(scope);
        std::scoped_lock lock(cache.mutex);
        for (const auto& entry : cache.entries) {
            if (entry.first == key) {
                return entry.second;
            }
        }
        const bool enabled = read_family_logging(scope);
        cache.entries.emplace_back(key, enabled);
        return enabled;
    }

    const std::filesystem::path& directory() {
        return shared_data().directory;
    }

    std::int64_t max_directory_mb() {
        return shared_data().max_directory_mb;
    }

    std::int64_t retention_days() {
        return shared_data().retention_days;
    }

    const std::filesystem::path& components_directory() {
        return shared_data().components_directory;
    }

    bool logging_ini_missing() {
        return shared_data().ini_missing;
    }

    std::string_view configuration_report() {
        return shared_data().report;
    }

} // namespace ac::logging::config
