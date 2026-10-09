/**
 * \file settings_app.ixx
 * \brief Top-level Settings menu. It delegates and writes no configuration.
 */
module;

#include "settings_catalog.hpp"

#include <Windows.h>
#include <charconv>
#include <iostream>
#include <string>

export module auto_core.main.settings_app;

import auto_core.core.component;
import auto_core.core.paths;
import auto_core.core.shell;

namespace {

    ac::Component settings_app {"auto_core_settings"};

    [[nodiscard]]
    bool sentinel_exists() {
        std::error_code error;
        return std::filesystem::exists(
            ac::paths::config_directory() / "auto_core.ini",
            error
        ) && !error;
    }

    [[nodiscard]]
    int run_and_wait(const std::filesystem::path& executable_path) {
        STARTUPINFOW startup_info {};
        startup_info.cb = sizeof(startup_info);
        PROCESS_INFORMATION process_info {};
        std::wstring command_line = L"\"" + executable_path.wstring() + L"\"";
        if (!CreateProcessW(
                executable_path.c_str(),
                command_line.data(),
                nullptr,
                nullptr,
                FALSE,
                0,
                nullptr,
                executable_path.parent_path().c_str(),
                &startup_info,
                &process_info
            )) {
            settings_app.log_print(
                "Unable to start {}.",
                executable_path.string()
            );
            return 1;
        }

        CloseHandle(process_info.hThread);
        WaitForSingleObject(process_info.hProcess, INFINITE);
        DWORD exit_code = 1;
        if (!GetExitCodeProcess(process_info.hProcess, &exit_code)) {
            CloseHandle(process_info.hProcess);
            settings_app.log_print(
                "Unable to read the exit code from {}.",
                executable_path.string()
            );
            return 1;
        }
        CloseHandle(process_info.hProcess);
        return static_cast<int>(exit_code);
    }

    [[nodiscard]]
    bool ensure_initialized() {
        if (ac::main::settings::startup_step(sentinel_exists()) ==
            ac::main::settings::StartupStep::show_menu) {
            return true;
        }

        const auto helper = ac::paths::bin_directory() / "auto_core_init.exe";
        settings_app.log_print(
            "config/auto_core.ini is missing. Launching auto_core_init.exe."
        );
        (void)run_and_wait(helper);
        if (ac::main::settings::recheck_after_init(sentinel_exists()) ==
            ac::main::settings::InitRecheck::show_menu) {
            return true;
        }

        settings_app.log_print(
            "Auto Core is not initialized. config/auto_core.ini was not created."
        );
        return false;
    }

    [[nodiscard]]
    std::string trim_line(const std::string_view value) {
        const auto first = value.find_first_not_of(" \t\r");
        if (first == std::string_view::npos) {
            return {};
        }
        const auto last = value.find_last_not_of(" \t\r");
        return std::string {value.substr(first, last - first + 1)};
    }

    void show_menu(const std::vector<ac::main::settings::MenuRow>& rows) {
        settings_app.log_print("Auto Core Settings");
        for (std::size_t index = 0; index < rows.size(); ++index) {
            settings_app.log_print("{}. {}", index + 1, rows[index].label);
        }
        settings_app.log_print("0. Exit");
        settings_app.lognl_print("Choice: ");
    }

} // namespace

export int run_settings_application() {
    ac::shell::set_process_app_user_model_id();
    SetConsoleTitleW(L"Auto Core Settings");

    if (!ensure_initialized()) {
        return 1;
    }

    while (true) {
        const auto identities = ac::main::settings::discover_settings_executables(
            ac::paths::bin_directory()
        );
        const auto rows = ac::main::settings::menu_rows(identities);
        show_menu(rows);

        std::string line;
        if (!std::getline(std::cin, line)) {
            settings_app.log("Input ended.");
            return 1;
        }
        settings_app.log(std::string_view {line});

        const auto choice = trim_line(line);
        if (choice == "0") {
            return 0;
        }

        unsigned int value = 0;
        const auto* const end = choice.data() + choice.size();
        const auto parsed = std::from_chars(choice.data(), end, value);
        if (choice.empty() ||
            parsed.ec != std::errc {} ||
            parsed.ptr != end ||
            value == 0 ||
            static_cast<std::size_t>(value) > rows.size()) {
            settings_app.log_print(
                "Enter a number from 0 to {}.",
                rows.size()
            );
            continue;
        }

        const auto& row = rows[static_cast<std::size_t>(value - 1)];
        (void)run_and_wait(ac::paths::bin_directory() / row.executable);
    }
}
