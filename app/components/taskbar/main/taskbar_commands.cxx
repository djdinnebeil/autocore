module;

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

module taskbar_commands;

import std;
import auto_core.core.console;
import auto_core.taskbar;
import command_registry;
import taskbar_logging;
import taskbar_protocol;

namespace {
    bool launch_application(
        const std::string_view command,
        const std::string_view display_name,
        const wchar_t* verb,
        const wchar_t* executable,
        const wchar_t* arguments = nullptr,
        const wchar_t* working_directory = nullptr
    ) {
        const HINSTANCE result = ShellExecuteW(
            nullptr,
            verb,
            executable,
            arguments,
            working_directory,
            SW_SHOWNORMAL
        );
        const auto shell_result = reinterpret_cast<std::intptr_t>(result);
        if (shell_result <= 32) {
            taskbar_component().log_print(
                "Unable to launch {}. ShellExecute error: {}.",
                display_name,
                shell_result
            );
            return false;
        }
        taskbar_component().log_main(
            "{}() - launched {} through ShellExecuteW",
            command,
            display_name
        );
        return true;
    }
}

void activate_auto_core() {
    if (ac::taskbar::try_activate_native("auto_core")) {
        taskbar_component().log_main(
            "activate_auto_core() - used the current Winkey mapping"
        );
        return;
    }

    const auto activated = ac::console::activate();
    if (!activated) {
        taskbar_component().log_print(
            "Unable to activate the shared Auto Core console: {}.",
            ac::console::error_message(activated.error())
        );
        return;
    }
    taskbar_component().log_main(
        "activate_auto_core() - activated the shared Auto Core console"
    );
}

void launch_powershell() {
    (void)launch_application(
        "launch_powershell",
        "PowerShell",
        L"open",
        L"powershell.exe"
    );
}

void launch_gitbash() {
    (void)launch_application(
        "launch_gitbash",
        "Git Bash",
        L"open",
        LR"(C:\Program Files\Git\git-bash.exe)",
        L"--cd-to-home"
    );
}

void activate_wordpad() {
    if (ac::taskbar::try_activate_native("wordpad")) {
        taskbar_component().log_main(
            "activate_wordpad() - used the current Winkey mapping"
        );
        return;
    }

    (void)launch_application(
        "activate_wordpad", "WordPad", L"open", L"wordpad.exe"
    );
}

void activate_powershell_in_admin() {
    if (ac::taskbar::try_activate_native("powershell_admin")) {
        taskbar_component().log_main(
            "activate_powershell_in_admin() - used the current Winkey mapping"
        );
        return;
    }

    (void)launch_application(
        "activate_powershell_in_admin",
        "PowerShell Administrator",
        L"runas",
        L"powershell.exe"
    );
}

void refresh_taskbar_positions() {
    taskbar_component().log_main("refresh_taskbar_positions");
    if (!ac::taskbar::request_refresh()) {
        taskbar_component().log_print(
            "Unable to update Winkey mappings."
        );
        return;
    }
    taskbar_component().log_print("Winkey mappings updated");
}

command_registry::Registry create_taskbar_command_registry() {
    command_registry::Registry registry;
    namespace commands = ac::protocol::taskbar::commands;

    registry.add(
        std::string {commands::activate_auto_core},
        &::activate_auto_core
    );
    registry.add(
        std::string {commands::launch_powershell},
        &::launch_powershell
    );
    registry.add(
        std::string {commands::activate_powershell_in_admin},
        &::activate_powershell_in_admin
    );
    registry.add(
        std::string {commands::activate_wordpad},
        &::activate_wordpad
    );
    registry.add(
        std::string {commands::launch_gitbash},
        &::launch_gitbash
    );
    registry.add(
        std::string {commands::refresh_taskbar_positions},
        &::refresh_taskbar_positions
    );

    return registry;
}
