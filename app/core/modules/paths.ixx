/**
 * \file paths.ixx
 * \brief Provides paths to Auto Core files and directories.
 *
 * Paths are derived from the directory containing the running executable.
 * Accessors compute their result once and return a reference that remains
 * valid for the lifetime of the process. They do not create or validate the
 * referenced filesystem object.
 *
 * Derived accessors call `executable_directory` on first use and therefore
 * throw the same `std::system_error` or `std::length_error`.
 */
module;

#include "ac_api.hpp"

export module auto_core.core.paths;

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
     *
     * Cached for the process lifetime. The directory is not created or
     * validated.
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
     * \brief Returns `<keymap directory>/bindings.ini`.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        keymap_file();

    /**
     * \brief Returns `<config directory>/keymap.ini`.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        keymap_settings_file();

    /**
     * \brief Returns `<keymap directory>/components`.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        keymap_components_directory();

    /**
     * \brief Returns `<keymap directory>/keymap_commands.txt`.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        keymap_commands_file();

    /**
     * \brief Returns the Taskbar data directory.
     *
     * Reads `[taskbar] directory` from `config/taskbar.ini`. A relative path
     * is resolved against the executable directory. An absolute path is used
     * as-is. A missing file, missing key, or empty value keeps
     * `<executable directory>/taskbar`. The live file is not rewritten.
     * Cached for the process lifetime. The directory is not created or
     * validated.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        taskbar_directory();

    /**
     * \brief Returns `<taskbar directory>/applications`.
     *
     * Cached for the process lifetime. The directory is not created or
     * validated.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        taskbar_applications_directory();

    /**
     * \brief Returns the Spotify data directory.
     *
     * Reads `[spotify] directory` from `config/spotify.ini`. A relative path
     * is resolved against the executable directory. An absolute path is used
     * as-is. A missing file, missing key, or empty value keeps
     * `<executable directory>/spotify`. The live file is not rewritten.
     * Cached for the process lifetime. The directory is not created or
     * validated.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        spotify_directory();

    /**
     * \brief Returns the Journal data directory.
     *
     * Reads `[journal] directory` from `config/journal.ini`. A relative path
     * is resolved against the executable directory. An absolute path is used
     * as-is. A missing file, missing key, or empty value keeps
     * `<executable directory>/journal`. The live file is not rewritten.
     * Cached for the process lifetime. The directory is not created or
     * validated.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        journal_directory();

    /**
     * \brief Returns the Writer data directory.
     *
     * Reads `[writer] directory` from `config/writer.ini`. A relative path
     * is resolved against the executable directory. An absolute path is used
     * as-is. A missing file, missing key, or empty value keeps
     * `<executable directory>/writer`. The live file is not rewritten.
     * Cached for the process lifetime. The directory is not created or
     * validated.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        writer_directory();

    /**
     * \brief Returns the Writer notes directory.
     *
     * Reads `[writer] notes_directory` from `config/writer.ini`. A relative
     * path is resolved against the executable directory. An absolute path is
     * used as-is. A missing file, missing key, or empty value keeps
     * `<executable directory>/notes`. The live file is not rewritten.
     * Cached for the process lifetime. The directory is not created or
     * validated.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        notes_directory();

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
