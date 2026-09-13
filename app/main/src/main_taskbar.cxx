module auto_core.main.taskbar;

import std;
import auto_core.core.console;
import auto_core.taskbar;
import auto_core.main.application;

import <Windows.h>;

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
            auto_core.logg_and_print(
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
            auto_core.logg_and_print(
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

        auto_core.logg_and_logg(
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

        auto_core.logg_and_logg(
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
        ac::taskbar::end_native_cycle();
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
        auto_core.logg_and_logg("cycle window");
    }
    else {
        end_cycle_session();
        auto_core.logg_and_logg("window selected");
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
        auto_core.logg_and_logg(
            "Configured activation for '{}' used the current Winkey mapping.",
            name
        );
        return true;
    }

    auto_core.logg_and_logg(
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
        auto_core.logg_and_logg(
            "Emulated activation for '{}' launching a new session.",
            name
        );
        (void)launch_and_focus(name);
        return;
    }

    if (windows.size() == 1) {
        const HWND window = as_hwnd(windows.front());
        if (IsIconic(window)) {
            auto_core.logg_and_logg(
                "Emulated activation for '{}' restoring the window.",
                name
            );
            focus_window(windows.front(), name);
            return;
        }
        if (is_window_foreground(window)) {
            auto_core.logg_and_logg(
                "Emulated activation for '{}' minimizing the window.",
                name
            );
            ShowWindow(window, SW_MINIMIZE);
            return;
        }
        auto_core.logg_and_logg(
            "Emulated activation for '{}' switching into the window.",
            name
        );
        focus_window(windows.front(), name);
        return;
    }

    if (ac::taskbar::configured_multi_window_cycle(name)) {
        auto_core.logg_and_logg(
            "Emulated multi-window cycle for '{}' found {} windows.",
            name,
            windows.size()
        );
        begin_emulated_cycle(name, windows);
        return;
    }

    auto_core.logg_and_logg(
        "Emulated activation for '{}' focusing the top matching window.",
        name
    );
    focus_window(windows.front(), name);
}

void MainTaskbarState::activate_configured(
    const std::string_view name
) {
    if (try_activate_configured(name)) return;
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
    auto_core.logg_and_logg("refresh_firefox()");
    if (is_foreground_window_firefox()) {
        start_reddit_new_tab();
    }
    else {
        taskbar.activate_configured("firefox");
    }
}

/** \keymap_command */
void start_reddit_new_tab() {
    auto_core.logg_and_logg("start_reddit_new_tab()");
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
        auto_core.logg_and_logg("{}", command);
        taskbar.activate_configured(application);
    };
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
                auto_core.logg_and_logg(
                    "Ignoring configured taskbar command '{}' for key '{}' "
                    "because that command name is already registered.",
                    command.command,
                    command.application
                );
            }
            else {
                auto_core.logg_and_print(
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
