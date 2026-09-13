module;

#include "configured_directory.hpp"

module auto_core.core.paths;

import std;
import auto_core.core.encoding;
import auto_core.core.ini;
import <Windows.h>;

namespace ac::paths {

    namespace {

        std::filesystem::path get_executable_directory() {
            std::wstring buffer(260, L'\0');

            while (true) {
                if (buffer.size() >
                    static_cast<std::size_t>(
                        (std::numeric_limits<DWORD>::max)()
                        )) {
                    throw std::length_error(
                        "Executable path exceeds the Win32 path length limit"
                    );
                }

                const DWORD size = static_cast<DWORD>(
                    buffer.size()
                    );

                const DWORD length = GetModuleFileNameW(
                    nullptr,
                    buffer.data(),
                    size
                );

                if (length == 0) {
                    throw std::system_error(
                        static_cast<int>(GetLastError()),
                        std::system_category(),
                        "GetModuleFileNameW failed"
                    );
                }

                if (length < size) {
                    buffer.resize(length);

                    return std::filesystem::path {buffer}
                    .parent_path();
                }

                if (buffer.size() >
                    (std::numeric_limits<DWORD>::max)() / 2) {
                    throw std::length_error(
                        "Executable path exceeds the Win32 path length limit"
                    );
                }

                buffer.resize(buffer.size() * 2);
            }
        }

    }

    const std::filesystem::path&
        executable_directory() {
        static const std::filesystem::path directory =
            get_executable_directory();

        return directory;
    }

    const std::filesystem::path&
        config_directory() {
        static const std::filesystem::path directory =
            executable_directory() / "config";

        return directory;
    }

    const std::filesystem::path&
        keymap_directory() {
        static const std::filesystem::path directory =
            executable_directory() / "keymap";

        return directory;
    }

    const std::filesystem::path&
        keymap_file() {
        static const std::filesystem::path file =
            keymap_directory() / "bindings.ini";

        return file;
    }

    const std::filesystem::path&
        keymap_settings_file() {
        static const std::filesystem::path file =
            config_directory() / "keymap.ini";

        return file;
    }

    const std::filesystem::path&
        keymap_components_directory() {
        static const std::filesystem::path directory =
            keymap_directory() / "components";

        return directory;
    }

    const std::filesystem::path&
        keymap_commands_file() {
        static const std::filesystem::path file =
            keymap_directory() / "keymap_commands.txt";

        return file;
    }

    const std::filesystem::path&
        taskbar_directory() {
        static const std::filesystem::path directory = [] {
            const std::filesystem::path default_directory =
                executable_directory() / "taskbar";

            const auto document = ac::ini::read(
                config_directory() / "taskbar.ini"
            );
            if (!document) {
                return default_directory;
            }

            const auto value = document->find("taskbar", "directory");
            if (!value) {
                return default_directory;
            }

            try {
                return ac::paths::detail::resolve_configured_directory(
                    std::filesystem::path {
                        ac::encoding::to_utf16(*value)
                    },
                    default_directory,
                    executable_directory()
                );
            }
            catch (...) {
                return default_directory;
            }
        }();

        return directory;
    }

    const std::filesystem::path&
        taskbar_applications_directory() {
        static const std::filesystem::path directory =
            taskbar_directory() / "applications";

        return directory;
    }

    const std::filesystem::path&
        spotify_directory() {
        static const std::filesystem::path directory = [] {
            const std::filesystem::path default_directory =
                executable_directory() / "spotify";

            const auto document = ac::ini::read(
                config_directory() / "spotify.ini"
            );
            if (!document) {
                return default_directory;
            }

            const auto value = document->find("spotify", "directory");
            if (!value) {
                return default_directory;
            }

            try {
                return ac::paths::detail::resolve_configured_directory(
                    std::filesystem::path {
                        ac::encoding::to_utf16(*value)
                    },
                    default_directory,
                    executable_directory()
                );
            }
            catch (...) {
                return default_directory;
            }
        }();

        return directory;
    }

    const std::filesystem::path&
        journal_directory() {
        static const std::filesystem::path directory = [] {
            const std::filesystem::path default_directory =
                executable_directory() / "journal";

            const auto document = ac::ini::read(
                config_directory() / "journal.ini"
            );
            if (!document) {
                return default_directory;
            }

            const auto value = document->find("journal", "directory");
            if (!value) {
                return default_directory;
            }

            try {
                return ac::paths::detail::resolve_configured_directory(
                    std::filesystem::path {
                        ac::encoding::to_utf16(*value)
                    },
                    default_directory,
                    executable_directory()
                );
            }
            catch (...) {
                return default_directory;
            }
        }();

        return directory;
    }

    const std::filesystem::path&
        writer_directory() {
        static const std::filesystem::path directory = [] {
            const std::filesystem::path default_directory =
                executable_directory() / "writer";

            const auto document = ac::ini::read(
                config_directory() / "writer.ini"
            );
            if (!document) {
                return default_directory;
            }

            const auto value = document->find("writer", "directory");
            if (!value) {
                return default_directory;
            }

            try {
                return ac::paths::detail::resolve_configured_directory(
                    std::filesystem::path {
                        ac::encoding::to_utf16(*value)
                    },
                    default_directory,
                    executable_directory()
                );
            }
            catch (...) {
                return default_directory;
            }
        }();

        return directory;
    }

    const std::filesystem::path&
        notes_directory() {
        static const std::filesystem::path directory = [] {
            const std::filesystem::path default_directory =
                executable_directory() / "notes";

            const auto document = ac::ini::read(
                config_directory() / "writer.ini"
            );
            if (!document) {
                return default_directory;
            }

            const auto value = document->find("writer", "notes_directory");
            if (!value) {
                return default_directory;
            }

            try {
                return ac::paths::detail::resolve_configured_directory(
                    std::filesystem::path {
                        ac::encoding::to_utf16(*value)
                    },
                    default_directory,
                    executable_directory()
                );
            }
            catch (...) {
                return default_directory;
            }
        }();

        return directory;
    }

    const std::filesystem::path&
        log_directory() {
        static const std::filesystem::path directory =
            executable_directory() / "logs";

        return directory;
    }

    const std::filesystem::path&
        error_log_directory() {
        static const std::filesystem::path directory =
            executable_directory() / "errors";

        return directory;
    }

}
