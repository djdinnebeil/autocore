module;

#include "logging_config_detail.hpp"

/**
 * \file logging_config.cxx
 * \brief Loads logger.ini for the shared logging subsystem.
 */
module auto_core.logging.config;

import std;
import auto_core.encoding;
import auto_core.ini;
import auto_core.paths;

namespace ac::logging::config {

    namespace {
        struct Data {
            bool enabled = true;
            bool write_to_console = false;
            std::filesystem::path directory =
                ac::paths::log_directory();
            std::string report = "Logging configuration:\n";

            Data() {
                const auto document = ac::ini::read(
                    ac::paths::config_directory() / "logger.ini"
                );
                if (!document) {
                    report += "logger.ini unavailable; using defaults\n";
                    return;
                }

                const auto directory_value =
                    document->find("logger", "directory");
                const detail::Settings resolved = detail::resolve(
                    {
                        .enabled = document->find("logger", "enabled"),
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
                enabled = resolved.enabled;
                write_to_console = resolved.write_to_console;
                directory = resolved.directory;
                report = resolved.report;
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

    std::string_view configuration_report() {
        return data().report;
    }

} // namespace ac::logging::config
