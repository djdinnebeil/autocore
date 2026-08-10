/**
 * \file config.ixx
 * \brief Provides access to Auto Core configuration settings.
 */
module;

#include "ac_api.hpp"

export module config;

import std;

export namespace ac::config {

    /**
     * \brief Returns a description of the successfully loaded configuration.
     */
    [[nodiscard]]
    AC_API std::string_view configuration_log() noexcept;

    /**
     * \brief Returns the directory containing the current executable.
     */
    [[nodiscard]]
    AC_API std::wstring_view executable_directory() noexcept;

    /**
     * \brief Returns the configured program title.
     */
    [[nodiscard]]
    AC_API std::wstring_view program_title() noexcept;

    /**
     * \brief Returns whether the keymap should be loaded from a file.
     */
    [[nodiscard]]
    AC_API bool use_keymap_file() noexcept;

    /**
     * \brief Returns whether detailed keymap tracing is enabled.
     */
    [[nodiscard]]
    AC_API bool keymap_trace_enabled() noexcept;

    /**
     * \brief Returns whether the server should be started.
     */
    [[nodiscard]]
    AC_API bool start_server() noexcept;

    /**
     * \brief Returns the configured server port.
     */
    [[nodiscard]]
    AC_API int port_number() noexcept;

    /**
     * \brief Returns the hour at which the extended day ends.
     */
    [[nodiscard]]
    AC_API int end_of_day() noexcept;

    /**
     * \brief Returns the base directory used for log files.
     */
    [[nodiscard]]
    AC_API std::string_view logger_directory() noexcept;

    /**
     * \brief Returns whether component log messages should also be
     * written to the console.
     */
    [[nodiscard]]
    AC_API bool send_logg_to_cout() noexcept;

    /**
     * \brief Finds the configured taskbar position for a program.
     *
     * \return The taskbar position, or std::nullopt when the program
     * is not configured.
     */
    [[nodiscard]]
    AC_API std::optional<int> taskbar_position(
        std::string_view program
    );

    /**
     * \brief Finds the program assigned to a taskbar position.
     *
     * \return The configured program name, or std::nullopt when the
     * position is not configured.
     */
    [[nodiscard]]
    AC_API std::optional<std::string_view> taskbar_program(
        int position
    );
}