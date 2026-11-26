import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.paths;
import auto_core.core.shell;

import <Windows.h>;
import <charconv>;
import <iostream>;

namespace {

ac::Component taskbar_settings {
    "taskbar_settings",
    ac::logging::config::LoggingScope {"taskbar"},
    ac::SharedLog {"taskbar"}
};

[[nodiscard]]
std::filesystem::path taskbar_executable() {
    return ac::paths::app_directory() / "auto_core_taskbar.exe";
}

[[nodiscard]]
std::optional<bool> policy_present(std::string& error) {
    const auto path = ac::paths::taskbar_policy_file();
    std::error_code code;
    const bool present = std::filesystem::exists(path, code);
    if (code) {
        error = std::format(
            "Failed to inspect {}: {}",
            path.string(),
            code.message()
        );
        return std::nullopt;
    }
    error.clear();
    return present;
}

[[nodiscard]]
std::wstring quote_argument(const std::wstring_view argument) {
    std::wstring quoted;
    quoted.push_back(L'"');
    quoted.append(argument);
    quoted.push_back(L'"');
    return quoted;
}

[[nodiscard]]
int launch_self(const std::vector<std::wstring>& arguments) {
    const auto executable_path = taskbar_executable();
    std::error_code error;
    if (!std::filesystem::exists(executable_path, error) || error) {
        taskbar_settings.log_alert("Missing {}.", executable_path.string());
        return 1;
    }

    std::wstring command_line = quote_argument(executable_path.wstring());
    for (const std::wstring& argument : arguments) {
        command_line += L' ';
        command_line += quote_argument(argument);
    }

    STARTUPINFOW startup_info {};
    startup_info.cb = sizeof(startup_info);
    PROCESS_INFORMATION process_info {};
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
        taskbar_settings.log_alert(
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
        taskbar_settings.log_alert(
            "Unable to read the exit code from auto_core_taskbar.exe."
        );
        return 1;
    }
    CloseHandle(process_info.hProcess);
    const int code = static_cast<int>(exit_code);
    if (code != 0) {
        taskbar_settings.log_alert("auto_core_taskbar.exe exited {}.", code);
    }
    return code;
}

struct MenuEntry {
    std::string label;
    std::vector<std::wstring> arguments;
    bool exit {false};
};

[[nodiscard]]
std::string trim_line(const std::string_view value) {
    const auto first = value.find_first_not_of(" \t\r");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r");
    return std::string {value.substr(first, last - first + 1)};
}

} // namespace

namespace ac::taskbar_app::settings {

int run_menu() {
    while (true) {
        std::string inspect_error;
        const auto present = policy_present(inspect_error);
        if (!inspect_error.empty()) {
            taskbar_settings.log_alert("{}", inspect_error);
        }
        const bool configuration = present.value_or(true);
        std::vector<MenuEntry> entries;
        if (configuration) {
            entries.push_back({"Configure", {L"--config"}, false});
        }
        else {
            entries.push_back({"Initialize", {L"--config", L"--init"}, false});
        }
        entries.push_back({
            "Discover taskbar applications",
            {L"--builder"},
            false
        });
        entries.push_back({"Exit", {}, true});

        taskbar_settings.log_alert("taskbar");
        for (std::size_t index = 0; index < entries.size(); ++index) {
            taskbar_settings.log_alert(
                "{}. {}",
                index + 1,
                entries[index].label
            );
        }
        taskbar_settings.lognl_alert("Choice: ");

        std::string line;
        if (!std::getline(std::cin, line)) {
            taskbar_settings.log_debug("Input ended.");
            return 1;
        }
        taskbar_settings.log_debug(std::string_view {line});

        unsigned int value = 0;
        const std::string choice = trim_line(line);
        const auto* const end = choice.data() + choice.size();
        const auto parsed = std::from_chars(choice.data(), end, value);
        if (choice.empty() ||
            parsed.ec != std::errc {} ||
            parsed.ptr != end ||
            value == 0 ||
            static_cast<std::size_t>(value) > entries.size()) {
            taskbar_settings.log_alert(
                "Enter a number from 1 to {}.",
                entries.size()
            );
            continue;
        }

        const MenuEntry& entry = entries[static_cast<std::size_t>(value - 1)];
        if (entry.exit) {
            return 0;
        }
        (void)launch_self(entry.arguments);
    }
}

int run() {
    ac::shell::set_process_app_user_model_id();
    return run_menu();
}

int run_after_discovery() {
    ac::shell::set_process_app_user_model_id();
    taskbar_settings.log_alert("Discovering taskbar applications");
    if (launch_self({L"--builder"}) == 0) {
        taskbar_settings.log_alert(
            "Taskbar application discovery complete."
        );
    }
    return run_menu();
}

} // namespace ac::taskbar_app::settings
