module;

#include "components_list_detail.hpp"
#include "logging_config_detail.hpp"

/**
 * \file logging_config.cxx
 * \brief Loads logging.ini and logger.ini for Auto Core processes.
 */
module auto_core.core.logging.config;

import std;
import auto_core.core.encoding;
import auto_core.core.ini;
import auto_core.core.paths;

namespace ac::logging::config {

    namespace {
        ac::logging::config::LogPrintMode to_public_mode(
            const detail::LogPrintMode mode
        ) noexcept {
            return mode == detail::LogPrintMode::log
                ? LogPrintMode::log
                : LogPrintMode::print;
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
            bool write_logs_to_files = true;
            bool write_logs_to_console = false;
            LogPrintMode log_print_mode = LogPrintMode::print;
            bool component_logging_default = true;
            std::filesystem::path directory = ac::paths::log_directory();
            std::filesystem::path components_directory =
                directory / "components";
            bool ini_missing = false;
            std::string report = "Logging configuration:\n";

            SharedData() {
                const auto list_path = ac::paths::components_list_file();
                const auto catalog =
                    ac::config::components_list::load_runtime_catalog(
                        list_path,
                        ac::paths::bin_directory()
                    );
                const auto catalog_note = catalog.used_discovery
                    ? std::string {
                        "components.list unavailable; discovering *_ac.exe "
                        "in the binary directory. Run components_editor.exe to "
                        "generate it. The file will not be created.\n"
                    }
                    : std::string {};

                const auto ini_path =
                    ac::paths::config_directory() / "logging.ini";
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
                    write_logs_to_files = resolved.write_logs_to_files;
                    write_logs_to_console = resolved.write_logs_to_console;
                    log_print_mode = to_public_mode(resolved.log_print_mode);
                    component_logging_default = resolved.component_logging_default;
                    directory = resolved.directory;
                    components_directory = resolved.components_directory;
                    report = ini_missing
                        ? "Logging configuration:\nlogging.ini unavailable; using defaults\n"
                        : "Logging configuration:\nlogging.ini unreadable; using defaults\n";
                    const auto body = resolved.report.find('\n');
                    if (body != std::string::npos) {
                        report += resolved.report.substr(body + 1);
                    }
                    report += catalog_note;
                    return;
                }

                const auto directory_value =
                    document->find("logging", "directory");
                const detail::LoggingSettings resolved = detail::resolve_logging(
                    {
                        .disable_all = document->find("logging", "disable_all"),
                        .write_logs_to_files = document->find(
                            "logging", "write_logs_to_files"
                        ),
                        .write_logs_to_console = document->find(
                            "logging", "write_logs_to_console"
                        ),
                        .log_print_mode = document->find(
                            "logging", "log_print_mode"
                        ),
                        .component_logging_default = document->find(
                            "logging", "component_logging_default"
                        ),
                        .directory = directory_value
                            ? std::optional<std::filesystem::path> {
                                ac::encoding::to_utf16(*directory_value)
                            }
                            : std::nullopt
                    },
                    ac::paths::log_directory(),
                    ac::paths::installation_root()
                );
                disable_all = resolved.disable_all;
                write_logs_to_files = resolved.write_logs_to_files;
                write_logs_to_console = resolved.write_logs_to_console;
                log_print_mode = to_public_mode(resolved.log_print_mode);
                component_logging_default = resolved.component_logging_default;
                directory = resolved.directory;
                components_directory = resolved.components_directory;
                report = resolved.report;
                report += catalog_note;
            }
        };

        struct LoggerData {
            std::uint64_t merge_interval_seconds = 60;
            bool merge_logs_on_shutdown = true;
            bool ini_missing = false;
            std::string report = "Logger configuration:\n";

            LoggerData() {
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

                const detail::LoggerSettings resolved = detail::resolve_logger(
                    {
                        .merge_interval_seconds = document->find(
                            "logger", "merge_interval_seconds"
                        ),
                        .merge_logs_on_shutdown = document->find(
                            "logger", "merge_logs_on_shutdown"
                        )
                    }
                );
                merge_interval_seconds = resolved.merge_interval_seconds;
                merge_logs_on_shutdown = resolved.merge_logs_on_shutdown;
                report = resolved.report;
            }
        };

        const SharedData& shared_data() {
            static const SharedData value;
            return value;
        }

        const LoggerData& logger_data() {
            static const LoggerData value;
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
            const auto ini_path = ac::paths::config_directory() /
                (std::string {scope.name} + ".ini");
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

    bool write_logs_to_files() {
        return shared_data().write_logs_to_files;
    }

    std::uint64_t merge_interval_seconds() {
        return logger_data().merge_interval_seconds;
    }

    bool merge_logs_on_shutdown() {
        return logger_data().merge_logs_on_shutdown;
    }

    bool write_logs_to_console() {
        return shared_data().write_logs_to_console;
    }

    LogPrintMode log_print_mode() {
        return shared_data().log_print_mode;
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

    const std::filesystem::path& components_directory() {
        return shared_data().components_directory;
    }

    bool logging_ini_missing() {
        return shared_data().ini_missing;
    }

    bool logger_ini_missing() {
        return logger_data().ini_missing;
    }

    std::string_view configuration_report() {
        return shared_data().report;
    }

    std::string_view logger_configuration_report() {
        return logger_data().report;
    }

} // namespace ac::logging::config
