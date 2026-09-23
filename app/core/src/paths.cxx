module;

#include "configured_directory.hpp"
#include "installation_layout.hpp"

module auto_core.core.paths;

import std;
import auto_core.core.encoding;
import auto_core.core.ini;
import <Windows.h>;

namespace ac::paths {

    namespace {

        std::filesystem::path get_image_path() {
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
                    return std::filesystem::path {buffer};
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

        const detail::InstallationLayout& cached_layout() {
            static const detail::InstallationLayout layout =
                detail::layout_from_image_path(get_image_path());
            return layout;
        }

    }

    const std::filesystem::path&
        bin_directory() {
        return cached_layout().bin_directory;
    }

    const std::filesystem::path&
        installation_root() {
        return cached_layout().installation_root;
    }

    const std::filesystem::path&
        config_directory() {
        static const std::filesystem::path directory =
            installation_root() / "config";

        return directory;
    }

    const std::filesystem::path&
        keymap_directory() {
        static const std::filesystem::path directory =
            installation_root() / "keymap";

        return directory;
    }

    const std::filesystem::path&
        keymap_file() {
        static const std::filesystem::path file =
            installation_root() / "keymap.map";

        return file;
    }

    const std::filesystem::path&
        components_list_file() {
        static const std::filesystem::path file =
            installation_root() / "components.list";

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
                installation_root() / "taskbar";

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
                    installation_root()
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
                installation_root() / "components" / "spotify";

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
                    installation_root()
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
                installation_root() / "components" / "journal";

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
                    installation_root()
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
                installation_root() / "writer";

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
                    installation_root()
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
                installation_root() / "notes";

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
                    installation_root()
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
            installation_root() / "logs";

        return directory;
    }

    const std::filesystem::path&
        error_log_directory() {
        static const std::filesystem::path directory =
            installation_root() / "errors";

        return directory;
    }

}
