module;

#include "components_list_detail.hpp"
#include "logging_config_detail.hpp"

/**
 * \file logging_config.cxx
 * \brief Loads logger.ini and components.list for the shared logging subsystem.
 */
module auto_core.core.logging.config;

import std;
import auto_core.core.config;
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
                ac::config::seed_missing_config_files();

                const auto list = ac::config::components_list::load(
                    ac::paths::config_directory() / "components.list"
                );
                enabled = ac::config::components_list::special_enabled(
                    list,
                    "logger"
                );

                const auto document = ac::ini::read(
                    ac::paths::config_directory() / "logger.ini"
                );
                if (!document) {
                    report += "logger.ini unavailable; using defaults\n";
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
                    ac::paths::executable_directory()
                );
                write_to_console = resolved.write_to_console;
                directory = resolved.directory;
                components_directory = resolved.components_directory;
                report = resolved.report;
                report += enabled
                    ? "logger enabled in components.list\n"
                    : "logger not enabled in components.list\n";
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
