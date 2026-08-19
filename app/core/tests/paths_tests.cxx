#include "catch_amalgamated.hpp"

#include <array>
#include <filesystem>
#include <memory>
#include <system_error>
#include <Windows.h>

import auto_core.paths;

namespace {

    std::filesystem::path process_executable_directory() {
        std::array<wchar_t, 32768> buffer {};
        const DWORD length = GetModuleFileNameW(
            nullptr,
            buffer.data(),
            static_cast<DWORD>(buffer.size())
        );

        if (length == 0) {
            throw std::system_error(
                static_cast<int>(GetLastError()),
                std::system_category(),
                "GetModuleFileNameW failed in paths test"
            );
        }

        if (length == buffer.size()) {
            throw std::length_error(
                "Test executable path exceeds the Win32 path length limit"
            );
        }

        return std::filesystem::path {buffer.data(), buffer.data() + length}
            .parent_path();
    }

    template<typename Accessor>
    void check_stable_reference(Accessor accessor) {
        CHECK(std::addressof(accessor()) == std::addressof(accessor()));
    }

} // namespace

TEST_CASE(
    "Executable directory matches the running process",
    "[paths][windows-integration]"
) {
    CHECK(
        ac::paths::executable_directory() ==
        process_executable_directory()
    );
}

TEST_CASE(
    "Auto Core paths are derived from the executable directory",
    "[paths][windows-integration]"
) {
    const auto& executable = ac::paths::executable_directory();
    const auto& keymap = ac::paths::keymap_directory();

    CHECK(ac::paths::config_directory() == executable / "config");
    CHECK(keymap == executable / "keymap");
    CHECK(ac::paths::keymap_file() == keymap / "keymap.ini");
    CHECK(
        ac::paths::keymap_settings_file() ==
        keymap / "keymap_settings.ini"
    );
    CHECK(
        ac::paths::keymap_commands_file() ==
        keymap / "keymap_commands.txt"
    );
    CHECK(ac::paths::notepad_directory() == executable / "notepad");
    CHECK(ac::paths::star_directory() == executable / "star");
    CHECK(ac::paths::link_directory() == executable / "link");
    CHECK(ac::paths::log_directory() == executable / "logs");
    CHECK(ac::paths::error_log_directory() == executable / "errors");
}

TEST_CASE(
    "Path accessors return stable process-lifetime references",
    "[paths][windows-integration]"
) {
    check_stable_reference(ac::paths::executable_directory);
    check_stable_reference(ac::paths::config_directory);
    check_stable_reference(ac::paths::keymap_directory);
    check_stable_reference(ac::paths::keymap_file);
    check_stable_reference(ac::paths::keymap_settings_file);
    check_stable_reference(ac::paths::keymap_commands_file);
    check_stable_reference(ac::paths::notepad_directory);
    check_stable_reference(ac::paths::star_directory);
    check_stable_reference(ac::paths::link_directory);
    check_stable_reference(ac::paths::log_directory);
    check_stable_reference(ac::paths::error_log_directory);
}
