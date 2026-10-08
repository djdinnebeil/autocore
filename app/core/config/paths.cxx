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
        keymap_commands_file() {
        static const std::filesystem::path file =
            keymap_directory() / "keymap_commands.txt";

        return file;
    }

    const std::filesystem::path&
        configured_directory(
            const std::filesystem::path& ini_path,
            const std::string_view section,
            const std::string_view key,
            const std::filesystem::path& default_directory
        ) {
        struct CacheKey {
            std::filesystem::path ini_path;
            std::string section;
            std::string key;
            std::filesystem::path default_directory;

            [[nodiscard]]
            bool operator<(const CacheKey& other) const {
                if (ini_path != other.ini_path) {
                    return ini_path < other.ini_path;
                }
                if (section != other.section) {
                    return section < other.section;
                }
                if (key != other.key) {
                    return key < other.key;
                }
                return default_directory < other.default_directory;
            }
        };

        static std::mutex mutex;
        static std::map<CacheKey, std::filesystem::path> cache;

        const CacheKey cache_key {
            ini_path,
            std::string {section},
            std::string {key},
            default_directory
        };

        const std::lock_guard lock {mutex};
        if (const auto found = cache.find(cache_key); found != cache.end()) {
            return found->second;
        }

        const auto resolved = [&]() -> std::filesystem::path {
            const auto document = ac::ini::read(ini_path);
            if (!document) {
                return default_directory;
            }

            const auto value = document->find(section, key);
            if (!value) {
                return default_directory;
            }

            try {
                return detail::resolve_configured_directory(
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

        return cache.emplace(cache_key, resolved).first->second;
    }

    const std::filesystem::path&
        taskbar_directory() {
        static const std::filesystem::path& directory = configured_directory(
            config_directory() / "taskbar.ini",
            "taskbar",
            "directory",
            installation_root() / "taskbar"
        );

        return directory;
    }

    const std::filesystem::path&
        taskbar_applications_directory() {
        static const std::filesystem::path directory =
            taskbar_directory() / "applications";

        return directory;
    }

    const std::filesystem::path&
        writer_directory() {
        static const std::filesystem::path& directory = configured_directory(
            config_directory() / "writer.ini",
            "writer",
            "directory",
            installation_root() / "writer"
        );

        return directory;
    }

    const std::filesystem::path&
        writer_notes_directory() {
        static const std::filesystem::path directory = [] {
            const std::filesystem::path fallback =
                writer_directory() / "notes";

            const auto document = ac::ini::read(
                config_directory() / "writer.ini"
            );
            if (!document) {
                return fallback;
            }

            const auto value = document->find("writer", "notes_subdirectory");
            if (!value || value->empty()) {
                return fallback;
            }

            try {
                const std::filesystem::path configured {
                    ac::encoding::to_utf16(std::string {*value})
                };
                if (configured.empty() ||
                    configured.has_root_name() ||
                    configured.has_root_directory()) {
                    return fallback;
                }
                return (writer_directory() / configured).lexically_normal();
            }
            catch (...) {
                return fallback;
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
