module;

#include "components_list_detail.hpp"
#include "logging_config_detail.hpp"

/**
 * \file logging_config.cxx
 * \brief Loads logger.ini for the shared logging subsystem.
 */
module auto_core.core.logging.config;

import std;
import auto_core.core.encoding;
import auto_core.core.ini;
import auto_core.core.paths;

namespace ac::logging::config {

    namespace {
        struct Data {
            std::uint64_t merge_interval_seconds = 60;
            bool merge_logs_on_shutdown = true;
            bool write_logs_to_console = false;
            std::filesystem::path directory =
                ac::paths::log_directory();
            std::filesystem::path components_directory =
                directory / "components";
            bool ini_missing = false;
            std::string report = "Logging configuration:\n";

            Data() {
                const auto list_path = ac::paths::components_list_file();
                const auto catalog =
                    ac::config::components_list::load_runtime_catalog(
                        list_path,
                        ac::paths::bin_directory()
                    );
                if (catalog.used_discovery) {
                    report +=
                        "components.list unavailable; discovering *_ac.exe "
                        "in the binary directory. Run components_editor.exe to "
                        "generate it. The file will not be created.\n";
                }

                const auto ini_path =
                    ac::paths::config_directory() / "logger.ini";
                const auto document = ac::ini::read(ini_path);
                if (!document) {
                    std::error_code exists_error;
                    const bool present =
                        std::filesystem::exists(ini_path, exists_error);
                    ini_missing = !present && !exists_error;
                    report += "logger.ini unavailable; using defaults\n";
                    report += "directory = logs\n";
                    report += "merge_interval_seconds = 60\n";
                    report += "merge_logs_on_shutdown = on\n";
                    report += "write_logs_to_console = off\n";
                    return;
                }

                const auto directory_value =
                    document->find("logger", "directory");
                const detail::Settings resolved = detail::resolve(
                    {
                        .merge_interval_seconds = document->find(
                            "logger", "merge_interval_seconds"
                        ),
                        .merge_logs_on_shutdown = document->find(
                            "logger", "merge_logs_on_shutdown"
                        ),
                        .write_logs_to_console = document->find(
                            "logger", "write_logs_to_console"
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
                merge_interval_seconds = resolved.merge_interval_seconds;
                merge_logs_on_shutdown = resolved.merge_logs_on_shutdown;
                write_logs_to_console = resolved.write_logs_to_console;
                directory = resolved.directory;
                components_directory = resolved.components_directory;
                report = resolved.report;
                if (catalog.used_discovery) {
                    report +=
                        "components.list unavailable; discovering *_ac.exe "
                        "in the binary directory. Run components_editor.exe to "
                        "generate it. The file will not be created.\n";
                }
            }
        };

        const Data& data() {
            static const Data value;
            return value;
        }
    }

    std::uint64_t merge_interval_seconds() {
        return data().merge_interval_seconds;
    }

    bool merge_logs_on_shutdown() {
        return data().merge_logs_on_shutdown;
    }

    bool write_logs_to_console() {
        return data().write_logs_to_console;
    }

    const std::filesystem::path& directory() {
        return data().directory;
    }

    const std::filesystem::path& components_directory() {
        return data().components_directory;
    }

    bool ini_missing() {
        return data().ini_missing;
    }

    std::string_view configuration_report() {
        return data().report;
    }

} // namespace ac::logging::config
