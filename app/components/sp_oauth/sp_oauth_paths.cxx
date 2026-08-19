module sp_oauth_paths;

import std;
import <Windows.h>;

namespace sp_oauth::paths {

    namespace {

        std::filesystem::path find_executable_directory() {
            std::wstring buffer(260, L'\0');

            while (true) {
                const DWORD size =
                    static_cast<DWORD>(buffer.size());

                const DWORD length = GetModuleFileNameW(
                    nullptr,
                    buffer.data(),
                    size
                );

                if (length == 0) {
                    return L".";
                }

                if (length < size) {
                    buffer.resize(length);

                    return std::filesystem::path {buffer}
                    .parent_path();
                }

                if (buffer.size() >
                    (std::numeric_limits<DWORD>::max)() / 2) {
                    return L".";
                }

                buffer.resize(buffer.size() * 2);
            }
        }

    }

    const std::filesystem::path&
        executable_directory() noexcept {
        static const std::filesystem::path directory =
            find_executable_directory();

        return directory;
    }

    const std::filesystem::path&
        star_directory() noexcept {
        static const std::filesystem::path directory =
            executable_directory() / "star";

        return directory;
    }

}
