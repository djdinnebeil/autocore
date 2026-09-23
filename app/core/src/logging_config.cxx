module;

#include "components_list_detail.hpp"
#include "logging_config_detail.hpp"

/**
 * \file logging_config.cxx
 * \brief Loads logger.ini and components.list for the shared logging subsystem.
 */
module auto_core.core.logging.config;

import std;
import auto_core.core.encoding;
import auto_core.core.ini;
import auto_core.core.paths;

namespace ac::logging::config {

    namespace {
        struct Data {
            bool enabled = false;
            bool write_to_console = false;
            std::filesystem::path directory =
                ac::paths::log_directory();
            std::filesystem::path components_directory =
                directory / "components";
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
                enabled = ac::config::components_list::special_enabled(
                    catalog.result,
                    "logger"
                );
                const bool logger_malformed = [&catalog] {
                    if (catalog.used_discovery) {
                        return false;
                    }
                    for (const auto& entry : catalog.result.malformed_values) {
                        if (entry.name == "logger") {
                            return true;
                        }
                    }
                    return false;
                }();

                const auto document = ac::ini::read(
                    ac::paths::config_directory() / "logger.ini"
                );
                if (!document) {
                    report += "logger.ini unavailable; using defaults\n";
                    if (logger_malformed) {
                        report +=
                            "logger has a malformed [components] value; "
                            "logger is disabled\n";
                    }
                    report += enabled
                        ? "logger enabled in components.list\n"
                        : "logger not enabled in components.list\n";
                    return;
                }

                const auto directory_value =
                    document->find("logger", "directory");
                const detail::Settings resolved = detail::resolve(
                    {
                        .write_to_console = document->find(
                            "logger", "write_to_console"
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
                write_to_console = resolved.write_to_console;
                directory = resolved.directory;
                components_directory = resolved.components_directory;
                report = resolved.report;
                if (catalog.used_discovery) {
                    report +=
                        "components.list unavailable; discovering *_ac.exe "
                        "in the binary directory. Run components_editor.exe to "
                        "generate it. The file will not be created.\n";
                }
                report += enabled
                    ? "logger enabled in components.list\n"
                    : "logger not enabled in components.list\n";
                if (logger_malformed) {
                    report +=
                        "logger has a malformed [components] value; "
                        "logger is disabled\n";
                }
            }
        };

        const Data& data() {
            static const Data value;
            return value;
        }
    }

    bool enabled() {
        return data().enabled;
    }

    bool write_to_console() {
        return data().write_to_console;
    }

    const std::filesystem::path& directory() {
        return data().directory;
    }

    const std::filesystem::path& components_directory() {
        return data().components_directory;
    }

    std::string_view configuration_report() {
        return data().report;
    }

} // namespace ac::logging::config
