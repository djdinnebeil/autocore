/**
 * \file paths.ixx
 * \brief Provides paths to Auto Core files and directories.
 *
 * Paths are derived from the directory containing the running executable.
 * Accessors compute their result once and return a reference that remains
 * valid for the lifetime of the process. They do not create or validate the
 * referenced filesystem object.
 */
module;

#include "ac_api.hpp"

export module auto_core.paths;

import std;

export namespace ac::paths {

    /**
     * \brief Returns the directory containing the running executable.
     *
     * The path is discovered on the first call and cached for the lifetime of
     * the process.
     *
     * \return A process-lifetime reference to the executable directory.
     * \throws std::system_error if Windows cannot obtain the executable path.
     * \throws std::length_error if the executable path exceeds the Win32 path
     * length limit.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        executable_directory();

    /**
     * \brief Returns `<executable directory>/config`.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        config_directory();

    /**
     * \brief Returns `<executable directory>/keymap`.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        keymap_directory();

    /**
     * \brief Returns `<keymap directory>/keymap.ini`.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        keymap_file();

    /**
     * \brief Returns `<keymap directory>/keymap_settings.ini`.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        keymap_settings_file();

    /**
     * \brief Returns `<keymap directory>/keymap_commands.txt`.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        keymap_commands_file();

    /**
     * \brief Returns `<executable directory>/notepad`.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        notepad_directory();

    /**
     * \brief Returns `<executable directory>/star`.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        star_directory();

    /**
     * \brief Returns `<executable directory>/link`.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        link_directory();

    /**
     * \brief Returns `<executable directory>/logs`.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        log_directory();

    /**
     * \brief Returns `<executable directory>/errors`.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        error_log_directory();

} // namespace ac::paths
