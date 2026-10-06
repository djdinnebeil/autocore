module;

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#include <TlHelp32.h>

module auto_core.main.taskbar;

import std;
import auto_core.core.console;
import auto_core.core.paths;
import auto_core.taskbar;
import auto_core.main.application;
import taskbar_protocol;

namespace {
    constexpr auto launch_window_timeout = std::chrono::seconds {5};
    constexpr auto launch_window_poll = std::chrono::milliseconds {50};
    constexpr std::wstring_view disabled_fallback_executable = L"[]";

    HWND as_hwnd(const ac::taskbar::WindowHandle handle) noexcept {
        return static_cast<HWND>(handle);
    }

    bool is_window_foreground(const HWND window) noexcept {
        const HWND foreground = GetForegroundWindow();
        if (foreground == nullptr || window == nullptr) return false;
        if (foreground == window) return true;
        return GetAncestor(foreground, GA_ROOT) == window;
    }

    void focus_window(
        const ac::taskbar::WindowHandle handle,
        const std::string_view name
    ) {
        const auto activated = ac::console::activate_window(handle);
        if (!activated) {
            auto_core.log_print(
                "Unable to activate a window for '{}': {}.",
                name,
                ac::console::error_message(activated.error())
            );
        }
    }

    struct LaunchSpec {
        std::wstring executable;
        const wchar_t* verb {L"open"};
    };

    std::optional<LaunchSpec> resolve_launch(const std::string_view name) {
        const auto configured =
            ac::taskbar::configured_fallback_executable_path(name);
        if (name == "powershell_admin") {
            if (configured && !configured->empty()) {
                return LaunchSpec {.executable = *configured, .verb = L"runas"};
            }
            return LaunchSpec {
                .executable = L"powershell.exe",
                .verb = L"runas"
            };
        }
        if (configured && !configured->empty()) {
            return LaunchSpec {.executable = *configured};
        }
        if (name == "wordpad") {
            return LaunchSpec {.executable = L"wordpad.exe"};
        }
        return std::nullopt;
    }

    bool launch_executable(const LaunchSpec& launch) {
        const auto result = reinterpret_cast<std::intptr_t>(ShellExecuteW(
            nullptr,
            launch.verb,
            launch.executable.c_str(),
            nullptr,
            nullptr,
            SW_SHOWNORMAL
        ));
        return result > 32;
    }

    bool launch_and_focus(const std::string_view name) {
        const auto launch = resolve_launch(name);
        if (!launch) {
            auto_core.print(
                "Winkey mapping not set for '{}' and no executable fallback "
                "is configured.",
                name
            );
            return false;
        }

        if (launch->executable == disabled_fallback_executable) {
            auto_core.log_print(
                "No fallback executable is configured for '{}'.",
                name
            );
            return false;
        }

        if (!launch_executable(*launch)) {
            auto_core.print(
                "Unable to launch fallback for '{}'.",
                name
            );
            return false;
        }

        auto_core.log_main(
            "Launched configured fallback for '{}'.", name
        );

        const auto deadline =
            std::chrono::steady_clock::now() + launch_window_timeout;
        while (std::chrono::steady_clock::now() < deadline) {
            const auto windows = ac::taskbar::matching_windows(name);
            if (!windows.empty()) {
                focus_window(windows.front(), name);
                return true;
            }
            std::this_thread::sleep_for(launch_window_poll);
        }

        auto_core.log_main(
            "Launched '{}' but no matching window appeared in time.",
            name
        );
        return true;
    }
}

void MainTaskbarState::end_cycle_session() {
    if (emulated_cycle) {
        emulated_cycle = false;
        cycle_application.clear();
        cycle_windows.clear();
        cycle_index = 0;
    }
    else if (switch_set) {
        if (!ac::taskbar::end_native_cycle()) {
            return;
        }
    }
    switch_set = false;
}

void MainTaskbarState::switch_windows(const int keycode) {
    if (switch_keycode == keycode) {
        if (emulated_cycle) {
            advance_emulated_cycle();
        }
        else {
            (void)ac::taskbar::advance_native_cycle(switch_position);
        }
        auto_core.log_main("cycle window");
    }
    else {
        end_cycle_session();
        auto_core.log_main("window selected");
    }
}

bool MainTaskbarState::try_activate_configured(
    const std::string_view name
) {
    const auto decision = ac::taskbar::prepare_native_activation(name);
    if (!decision) return false;

    if (!decision->should_cycle) {
        if (!ac::taskbar::activate_native_position(decision->position)) {
            return false;
        }
        auto_core.log_main(
            "Configured activation for '{}' used the current Winkey mapping.",
            name
        );
        return true;
    }

    auto_core.log_main(
        "Configured multi-window cycle for '{}' found at least {} windows.",
        name,
        decision->matching_window_count
    );
    if (ac::taskbar::begin_native_cycle(decision->position)) {
        emulated_cycle = false;
        cycle_application.clear();
        cycle_windows.clear();
        switch_set = true;
        switch_position = decision->position;
        return true;
    }
    return false;
}

void MainTaskbarState::begin_emulated_cycle(
    const std::string_view name,
    std::vector<ac::taskbar::WindowHandle> windows
) {
    std::size_t index = 0;
    const HWND foreground = GetForegroundWindow();
    const HWND root = foreground == nullptr
        ? nullptr
        : GetAncestor(foreground, GA_ROOT);
    for (std::size_t i = 0; i < windows.size(); ++i) {
        const HWND window = as_hwnd(windows[i]);
        if (window == foreground || window == root) {
            index = (i + 1) % windows.size();
            break;
        }
    }

    focus_window(windows[index], name);
    cycle_application = std::string {name};
    cycle_windows = std::move(windows);
    cycle_index = index;
    emulated_cycle = true;
    switch_set = true;
}

void MainTaskbarState::advance_emulated_cycle() {
    const std::string name = cycle_application;
    auto windows = ac::taskbar::matching_windows(name);
    if (windows.size() < 2) {
        end_cycle_session();
        if (windows.size() == 1) {
            focus_window(windows.front(), name);
        }
        return;
    }

    std::size_t index = (cycle_index + 1) % windows.size();
    if (cycle_index < cycle_windows.size()) {
        const HWND current = as_hwnd(cycle_windows[cycle_index]);
        const auto found = std::ranges::find_if(
            windows,
            [current](const ac::taskbar::WindowHandle handle) {
                return as_hwnd(handle) == current;
            }
        );
        if (found != windows.end()) {
            const auto current_index = static_cast<std::size_t>(
                found - windows.begin()
            );
            index = (current_index + 1) % windows.size();
        }
    }

    cycle_windows = std::move(windows);
    cycle_index = index;
    focus_window(cycle_windows[cycle_index], cycle_application);
}

void MainTaskbarState::emulate_configured(const std::string_view name) {
    const auto windows = ac::taskbar::matching_windows(name);
    if (windows.empty()) {
        auto_core.log_main(
            "Emulated activation for '{}' launching a new session.",
            name
        );
        (void)launch_and_focus(name);
        return;
    }

    if (windows.size() == 1) {
        const HWND window = as_hwnd(windows.front());
        if (IsIconic(window)) {
            auto_core.log_main(
                "Emulated activation for '{}' restoring the window.",
                name
            );
            focus_window(windows.front(), name);
            return;
        }
        if (is_window_foreground(window)) {
            auto_core.log_main(
                "Emulated activation for '{}' minimizing the window.",
                name
            );
            ShowWindow(window, SW_MINIMIZE);
            return;
        }
        auto_core.log_main(
            "Emulated activation for '{}' switching into the window.",
            name
        );
        focus_window(windows.front(), name);
        return;
    }

    if (ac::taskbar::configured_multi_window_cycle(name)) {
        auto_core.log_main(
            "Emulated multi-window cycle for '{}' found {} windows.",
            name,
            windows.size()
        );
        begin_emulated_cycle(name, windows);
        return;
    }

    auto_core.log_main(
        "Emulated activation for '{}' focusing the top matching window.",
        name
    );
    focus_window(windows.front(), name);
}

void MainTaskbarState::activate_configured(
    const std::string_view name
) {
    if (try_activate_configured(name)) return;
    if (const auto path = ac::taskbar::configured_window_executable_path(name);
        path && *path == L"::runtime::") {
        auto_core.log_print(
            "Windows Notepad activation requires runtime executable-path "
            "resolution, which has not been implemented yet. If Notepad "
            "functionality is needed, move Notepad to taskbar positions 1 "
            "through 10."
        );
        return;
    }
    emulate_configured(name);
}

bool is_firefox_window_class(const std::wstring_view class_name) {
    return class_name == L"MozillaWindowClass" ||
        class_name.starts_with(L"Mozilla_firefox_");
}

bool is_foreground_window_firefox() {
    const HWND hwnd = GetForegroundWindow();

    if (hwnd == nullptr) {
        return false;
    }

    std::array<wchar_t, 256> class_name {};

    const int class_length = GetClassNameW(
        hwnd,
        class_name.data(),
        static_cast<int>(class_name.size())
    );

    if (class_length <= 0) {
        return false;
    }

    return is_firefox_window_class(
        std::wstring_view {
            class_name.data(),
            static_cast<std::size_t>(class_length)
        }
    );
}

/** \keymap_command */
void refresh_firefox() {
    auto_core.log_main("refresh_firefox()");
    if (is_foreground_window_firefox()) {
        start_reddit_new_tab();
    }
    else {
        taskbar.activate_configured("firefox");
    }
}

/** \keymap_command */
void start_reddit_new_tab() {
    auto_core.log_main("start_reddit_new_tab()");
    std::wstring url = L"https://www.reddit.com";
    std::wstring firefox_path = LR"(C:\Program Files\Mozilla Firefox\firefox.exe)";
    ShellExecuteW(0, 0, firefox_path.c_str(), url.c_str(), 0, SW_SHOW);
}

command_registry::Action configured_activation_action(
    std::string_view arguments
) {
    const auto first = arguments.find_first_not_of(" \t");
    if (first == std::string_view::npos) return {};
    const auto last = arguments.find_last_not_of(" \t");
    arguments = arguments.substr(first, last - first + 1);

    if (arguments.size() >= 2 &&
        ((arguments.front() == '"' && arguments.back() == '"') ||
         (arguments.front() == '\'' && arguments.back() == '\''))) {
        arguments.remove_prefix(1);
        arguments.remove_suffix(1);
    }
    if (arguments.empty() || arguments.contains(',')) return {};

    return taskbar_activation_action(arguments);
}

command_registry::Action taskbar_activation_action(
    const std::string_view application,
    const std::string_view command
) {
    std::string log_name {command};
    if (log_name.empty()) {
        log_name = "activate_";
        log_name += application;
    }
    return [
        application = std::string {application},
        command = std::move(log_name)
    ] {
        auto_core.log_main("{}", command);
        taskbar.activate_configured(application);
    };
}

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

void launch_shell_and_focus(
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
        auto_core.log_print(
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
        auto_core.log_main(
            "{}() - launched, but no window was found to focus.",
            command
        );
        return;
    }
    if (const auto activated = ac::console::activate_window(window);
        !activated) {
        auto_core.log_print(
            "{}() - launched, but could not confirm focus: {}.",
            command,
            ac::console::error_message(activated.error())
        );
        return;
    }
    auto_core.log_main("{}() - launched and focused", command);
}

} // namespace

void taskbar_runtime_commands::register_reserved_with(
    command_registry::Registry& registry
) {
    namespace commands = ac::protocol::taskbar::commands;

    registry.add(
        std::string {commands::activate_wordpad},
        taskbar_activation_action("wordpad", commands::activate_wordpad)
    );
    registry.add(
        std::string {commands::activate_powershell_in_admin},
        taskbar_activation_action(
            "powershell_admin",
            commands::activate_powershell_in_admin
        )
    );
    registry.add(std::string {commands::launch_powershell}, [] {
        launch_shell_and_focus("launch_powershell", L"powershell.exe");
    });
    registry.add(std::string {commands::launch_gitbash}, [] {
        launch_shell_and_focus(
            "launch_gitbash",
            LR"(C:\Program Files\Git\git-bash.exe)",
            L"--cd-to-home"
        );
    });
    registry.add("launch_taskbar_config", [] {
        const auto executable =
            ac::paths::bin_directory() / "taskbar_config.exe";
        if (!ac::main::create_process_and_focus(
                executable, {}, CREATE_NEW_CONSOLE
            )) {
            auto_core.log_print(
                "Unable to start taskbar_config.exe."
            );
        }
    });
}

void taskbar_runtime_commands::register_with(
    command_registry::Registry& registry
) {
    registry.add("refresh_firefox", &::refresh_firefox);
    registry.add("start_reddit_new_tab", &::start_reddit_new_tab);
    registry.add_factory(
        "activate",
        &::configured_activation_action,
        "activate(\"application\")"
    );

    for (const auto& command :
         ac::taskbar::configured_activation_commands()) {
        if (registry.contains(command.command)) {
            if (command.command == "activate_auto_core") {
                auto_core.log_main(
                    "Ignoring configured taskbar command '{}' for key '{}' "
                    "because that command name is already registered.",
                    command.command,
                    command.application
                );
            }
            else {
                auto_core.log_print(
                    "Ignoring configured taskbar command '{}' for key '{}' "
                    "because that command name is already registered.",
                    command.command,
                    command.application
                );
            }
            continue;
        }
        registry.add(
            command.command,
            taskbar_activation_action(command.application, command.command)
        );
    }
}
