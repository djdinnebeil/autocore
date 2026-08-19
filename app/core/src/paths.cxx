module auto_core.paths;

import std;
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
            keymap_directory() / "keymap.ini";

        return file;
    }

    const std::filesystem::path&
        keymap_settings_file() {
        static const std::filesystem::path file =
            keymap_directory() / "keymap_settings.ini";

        return file;
    }

    const std::filesystem::path&
        keymap_commands_file() {
        static const std::filesystem::path file =
            keymap_directory() / "keymap_commands.txt";

        return file;
    }

    const std::filesystem::path&
        notepad_directory() {
        static const std::filesystem::path directory =
            executable_directory() / "notepad";

        return directory;
    }

    const std::filesystem::path&
        star_directory() {
        static const std::filesystem::path directory =
            executable_directory() / "star";

        return directory;
    }

    const std::filesystem::path&
        link_directory() {
        static const std::filesystem::path directory =
            executable_directory() / "link";

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
