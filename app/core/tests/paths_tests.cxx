#include "catch_amalgamated.hpp"

#include <array>
#include <filesystem>
#include <memory>
#include <system_error>
#include <Windows.h>

import auto_core.core.paths;

namespace {

    std::filesystem::path process_image_parent() {
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
    "Binary directory matches the running process image parent",
    "[paths][windows-integration]"
) {
    CHECK(ac::paths::bin_directory() == process_image_parent());
    CHECK(
        ac::paths::installation_root() ==
        ac::paths::bin_directory().parent_path()
    );
}

TEST_CASE(
    "Data paths are derived from the installation root",
    "[paths][unit]"
) {
    const auto& root = ac::paths::installation_root();
    const auto& config = ac::paths::config_directory();
    const auto& keymap = ac::paths::keymap_directory();

    CHECK(config == root / "config");
    CHECK(keymap == root / "keymap");
    CHECK(ac::paths::keymap_file() == root / "keymap.map");
    CHECK(ac::paths::components_list_file() == root / "components.list");
    CHECK(
        ac::paths::component_config_file("taskbar") ==
        config / "components" / "taskbar.ini"
    );
    CHECK(
        ac::paths::keymap_commands_file() ==
        keymap / "keymap_commands.txt"
    );
    CHECK(ac::paths::log_directory() == root / "data" / "logs");
    CHECK(ac::paths::error_log_directory() == root / "errors");
}

TEST_CASE(
    "Assembled dist app layout keeps data at the installation root",
    "[paths][runtime-layout]"
) {
    const auto dist_exe = std::filesystem::path {__FILE__}
        .parent_path()
        .parent_path()
        .parent_path()
        .parent_path() /
        "dist" / "app" / "auto_core.exe";
    std::error_code error;
    if (!std::filesystem::exists(dist_exe, error) || error) {
        SKIP("dist/app/auto_core.exe is not present");
    }

    const auto app = dist_exe.parent_path();
    const auto root = app.parent_path();
    CHECK(app.filename() == "app");
    CHECK(std::filesystem::exists(root / "config", error));
    CHECK_FALSE(std::filesystem::exists(app / "config", error));
}

TEST_CASE(
    "Path accessors return stable references",
    "[paths][unit]"
) {
    check_stable_reference(ac::paths::bin_directory);
    check_stable_reference(ac::paths::installation_root);
    check_stable_reference(ac::paths::config_directory);
    check_stable_reference(ac::paths::keymap_directory);
    check_stable_reference(ac::paths::keymap_file);
    check_stable_reference(ac::paths::components_list_file);
    check_stable_reference(ac::paths::log_directory);
    check_stable_reference(ac::paths::error_log_directory);
}
