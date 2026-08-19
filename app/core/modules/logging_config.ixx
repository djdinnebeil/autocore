/**
 * \file logging_config.ixx
 * \brief Provides shared logging configuration.
 */
module;

#include "ac_api.hpp"

export module auto_core.logging.config;

import std;

export namespace ac::logging::config {

    /** Logger and central forwarding are enabled by default. */
    [[nodiscard]] AC_API bool enabled();

    /** Component log messages are not written to the console by default. */
    [[nodiscard]] AC_API bool write_to_console();

    /**
     * Returns the configured log directory. A missing or empty setting uses
     * `ac::paths::log_directory()`. Relative paths are resolved against the
     * executable directory.
     */
    [[nodiscard]] AC_API const std::filesystem::path& directory();

    /** Returns a startup report describing applied logger settings. */
    [[nodiscard]] AC_API std::string_view configuration_report();

} // namespace ac::logging::config
