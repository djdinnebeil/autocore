module;

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#include <TlHelp32.h>

module auto_core.main.components.taskbar;

import std;
import auto_core.main.application;
import auto_core.main.components;
import auto_core.core.console;
import auto_core.core.paths;
import auto_core.main.taskbar;
import taskbar_protocol;

namespace {
    std::unordered_set<DWORD> process_tree(const DWORD root) {
        std::unordered_set<DWORD> pids {root};
        if (root == 0) {
            return pids;
        }
        const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            return pids;
        }
        PROCESSENTRY32W entry {};
        entry.dwSize = sizeof(entry);
        bool grew = true;
        while (grew) {
            grew = false;
            if (!Process32FirstW(snapshot, &entry)) {
                break;
            }
            do {
                if (pids.contains(entry.th32ParentProcessID) &&
                    pids.insert(entry.th32ProcessID).second) {
                    grew = true;
                }
            } while (Process32NextW(snapshot, &entry));
        }
        CloseHandle(snapshot);
        return pids;
    }

    HWND find_visible_window(const std::unordered_set<DWORD>& pids) {
        struct Search {
            const std::unordered_set<DWORD>* pids;
            HWND window;
        } search {&pids, nullptr};
        EnumWindows(
            [](HWND window, LPARAM parameter) -> BOOL {
                auto& state = *reinterpret_cast<Search*>(parameter);
                if (!IsWindowVisible(window) ||
                    GetWindow(window, GW_OWNER) != nullptr) {
                    return TRUE;
                }
                DWORD process_id {};
                GetWindowThreadProcessId(window, &process_id);
                if (state.pids->contains(process_id)) {
                    state.window = window;
                    return FALSE;
                }
                return TRUE;
            },
            reinterpret_cast<LPARAM>(&search)
        );
        return search.window;
    }

    HWND wait_for_launched_window(const DWORD process_id) {
        constexpr auto timeout = std::chrono::seconds {5};
        constexpr auto poll = std::chrono::milliseconds {50};
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline) {
            const HWND window = find_visible_window(process_tree(process_id));
            if (window != nullptr) {
                return window;
            }
            std::this_thread::sleep_for(poll);
        }
        return find_visible_window(process_tree(process_id));
    }

    void launch_and_focus(
        const std::string_view command,
        const wchar_t* executable,
        const wchar_t* arguments = nullptr,
        const wchar_t* working_directory = nullptr,
        const wchar_t* verb = L"open"
    ) {
        SHELLEXECUTEINFOW execution {
            .cbSize = sizeof(execution),
            .fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC,
            .lpVerb = verb,
            .lpFile = executable,
            .lpParameters = arguments,
            .lpDirectory = working_directory,
            .nShow = SW_SHOWNORMAL,
        };
        if (!ShellExecuteExW(&execution)) {
            auto_core.log_and_print(
                "Unable to launch {}. ShellExecute error: {}.",
                command,
                GetLastError()
            );
            return;
        }

        DWORD process_id {};
        if (execution.hProcess != nullptr) {
            process_id = GetProcessId(execution.hProcess);
            (void)AllowSetForegroundWindow(process_id);
            (void)WaitForInputIdle(execution.hProcess, 2000);
        }

        const HWND window = wait_for_launched_window(process_id);
        if (execution.hProcess != nullptr) {
            CloseHandle(execution.hProcess);
        }
        if (window == nullptr) {
            auto_core.log_and_log(
                "{}() - launched, but no window was found to focus.",
                command
            );
            return;
        }
        if (const auto activated = ac::console::activate_window(window);
            !activated) {
            auto_core.log_and_print(
                "{}() - launched, but could not confirm focus: {}.",
                command,
                ac::console::error_message(activated.error())
            );
            return;
        }
        auto_core.log_and_log("{}() - launched and focused", command);
    }
}

void refresh_taskbar_positions() {
    auto_core.log_and_log("refresh_taskbar_positions");
    ac::main::components::invoke("taskbar", "refresh_taskbar_positions");
}

void invoke_taskbar_command(const std::string_view name) {
    ac::main::components::invoke("taskbar", name);
}

command_registry::Action taskbar_pipe_command(const std::string_view name) {
    return [value = std::string {name}] {
        ac::main::components::invoke("taskbar", value);
    };
}

void taskbar_component::runtime_commands::register_with(
    command_registry::Registry& registry
) {
    namespace commands = ac::protocol::taskbar::commands;

    registry.add(
        std::string {commands::refresh_taskbar_positions},
        &::refresh_taskbar_positions
    );
    registry.add(
        std::string {commands::activate_auto_core},
        taskbar_pipe_command(commands::activate_auto_core)
    );
    registry.add(
        std::string {commands::activate_wordpad},
        taskbar_activation_action("wordpad", commands::activate_wordpad)
    );
    registry.add(
        std::string {commands::activate_powershell_in_admin},
        taskbar_activation_action(
            "powershell_admin", commands::activate_powershell_in_admin
        )
    );
    registry.add(std::string {commands::launch_powershell}, [] {
        launch_and_focus("launch_powershell", L"powershell.exe");
    });
    registry.add(std::string {commands::launch_gitbash}, [] {
        launch_and_focus(
            "launch_gitbash",
            LR"(C:\Program Files\Git\git-bash.exe)",
            L"--cd-to-home"
        );
    });

    registry.add("launch_taskbar_config", [] {
        const auto executable =
            ac::paths::executable_directory() / "taskbar_config.exe";
        if (!ac::main::create_process_and_focus(
                executable, {}, CREATE_NEW_CONSOLE
            )) {
            auto_core.log_and_print(
                "Unable to start taskbar_config.exe."
            );
        }
    });
}
