/**
 * \file paths.ixx
 * \brief Provides paths to Auto Core files and directories.
 *
 * Binary paths are derived from the directory containing the running
 * executable (`bin_directory`). Configuration and runtime-data paths are
 * derived from that directory's parent (`installation_root`). Accessors
 * compute their result once and return a reference that remains valid for
 * the lifetime of the process. They do not create or validate the
 * referenced filesystem object. The process current working directory is
 * never used.
 *
 * Derived accessors throw the same `std::system_error` or
 * `std::length_error` as `bin_directory` on first use.
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
     * \return A process-lifetime reference to the binary directory.
     * \throws std::system_error if Windows cannot obtain the executable path.
     * \throws std::length_error if the executable path exceeds the Win32 path
     * length limit.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        bin_directory();

    /**
     * \brief Returns the parent of `bin_directory` (the installation root).
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        installation_root();

    /**
     * \brief Returns `<installation root>/config`.
     *
     * Cached for the process lifetime. The directory is not created or
     * validated.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        config_directory();

    /**
     * \brief Returns `<installation root>/keymap`.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        keymap_directory();

    /**
     * \brief Returns `<installation root>/keymap.map`.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        keymap_file();

    /**
     * \brief Returns `<installation root>/components.list`.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        components_list_file();

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
     * \brief Returns a process-lifetime directory from an INI setting.
     *
     * Reads `[section] key` from `ini_path`. A relative value is resolved
     * against `installation_root()`. An absolute value is used as-is. Either
     * result is lexically normalized. A missing file, missing key, empty
     * value, or unusable value returns `default_directory` unchanged. The
     * file is not rewritten. The same arguments return the same cached
     * reference for the rest of the process.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        configured_directory(
            const std::filesystem::path& ini_path,
            std::string_view section,
            std::string_view key,
            const std::filesystem::path& default_directory
        );

    /**
     * \brief Returns the Taskbar data directory.
     *
     * Reads `[taskbar] directory` from `config/taskbar.ini`. A relative path
     * is resolved against the installation root. An absolute path is used
     * as-is. A missing file, missing key, or empty value keeps
     * `<installation root>/taskbar`. The live file is not rewritten.
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
     * \brief Returns the Writer data directory.
     *
     * Reads `[writer] directory` from `config/writer.ini`. A relative path
     * is resolved against the installation root. An absolute path is used
     * as-is. A missing file, missing key, or empty value keeps
     * `<installation root>/writer`. The live file is not rewritten.
     * Cached for the process lifetime. The directory is not created or
     * validated.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        writer_directory();

    /**
     * \brief Returns the Writer notes directory.
     *
     * Reads `[writer] notes_subdirectory` from `config/writer.ini`. The
     * value must be relative: no root-name and no root-directory. A
     * relative value is resolved against `writer_directory()`. A missing
     * file, missing key, empty value, or non-relative value keeps
     * `<writer directory>/notes`. The live file is not rewritten.
     * Cached for the process lifetime. The directory is not created or
     * validated.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        writer_notes_directory();

    /**
     * \brief Returns `<installation root>/logs`.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        log_directory();

    /**
     * \brief Returns `<installation root>/errors`.
     */
    [[nodiscard]]
    AC_API const std::filesystem::path&
        error_log_directory();

} // namespace ac::paths
