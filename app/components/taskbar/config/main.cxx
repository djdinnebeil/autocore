import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.encoding;
import auto_core.core.ini;
import auto_core.core.paths;
import taskbar_defaults;
import components_editor_request;

import <Windows.h>;
import <iostream>;
import auto_core.core.shell;

namespace {

ac::Component taskbar_config {
    "taskbar_config",
    ac::logging::config::LoggingScope {"taskbar"}
};

struct Settings {
    std::string directory {std::string {taskbar::defaults::directory}};
    std::string mode {std::string {taskbar::defaults::mode}};
    bool logging = true;
};

std::wstring trim_wide(std::wstring_view value) {
    const auto first = value.find_first_not_of(L" \t\r\n");
    if (first == std::wstring_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(L" \t\r\n");
    return std::wstring {value.substr(first, last - first + 1)};
}

std::string ascii_lower(std::string_view value) {
    std::string result {value};
    for (char& character : result) {
        if (character >= 'A' && character <= 'Z') {
            character = static_cast<char>(character - 'A' + 'a');
        }
    }
    return result;
}

bool write_ini(const std::filesystem::path& path, const Settings& settings) {
    std::error_code create_error;
    std::filesystem::create_directories(path.parent_path(), create_error);
    if (create_error) {
        taskbar_config.log_print(
            "Failed to create config directory: {}",
            create_error.message()
        );
        return false;
    }
    const auto contents = taskbar::defaults::ini_for(
        settings.directory,
        settings.mode,
        settings.logging
    );
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        taskbar_config.log_print("Failed to create {}", path.string());
        return false;
    }
    output.write(
        contents.data(),
        static_cast<std::streamsize>(contents.size())
    );
    output.close();
    if (!output) {
        taskbar_config.log_print("Failed to write {}", path.string());
        return false;
    }
    taskbar_config.log_print("Wrote {}", path.string());
    return true;
}

Settings read_settings(const std::filesystem::path& path) {
    Settings settings;
    const auto document = ac::ini::read(path);
    if (!document) {
        taskbar_config.log_print(
            "config/taskbar.ini is malformed. Using compiled defaults until "
            "you save new settings."
        );
        return settings;
    }
    if (const auto directory = document->find("taskbar", "directory")) {
        if (!directory->empty()) {
            settings.directory = std::string {*directory};
        }
    }
    if (const auto mode = document->find("taskbar", "mode")) {
        const auto lower = ascii_lower(*mode);
        if (taskbar::defaults::is_mode(lower)) {
            settings.mode = lower;
        }
        else {
            taskbar_config.log_print(
                "config/taskbar.ini mode is invalid. Using live."
            );
        }
    }
    if (const auto logging = document->find("taskbar", "logging")) {
        if (*logging == "on" || *logging == "true") {
            settings.logging = true;
        }
        else if (*logging == "off" || *logging == "false") {
            settings.logging = false;
        }
        else {
            settings.logging = ac::logging::config::component_logging_default();
        }
    }
    else {
        settings.logging = ac::logging::config::component_logging_default();
    }
    return settings;
}

void say_restart() {
    std::wcout
        << L"Restart taskbar_ac.exe before directory and mode changes apply.\n";
}

std::optional<std::string> prompt_directory(
    const std::wstring_view label,
    const std::string_view current
) {
    std::wcout << label
               << L" ["
               << ac::encoding::to_utf16(current)
               << L"]: ";
    std::wcout.flush();
    std::wstring input;
    if (!std::getline(std::wcin, input)) {
        return std::nullopt;
    }
    const auto value = trim_wide(input);
    if (value.empty()) {
        return std::string {current};
    }
    return ac::encoding::to_utf8(value);
}

std::optional<std::string> read_console_line() {
    std::wstring input;
    if (!std::getline(std::wcin, input)) {
        return std::nullopt;
    }
    return ac::encoding::to_utf8(trim_wide(input));
}

std::optional<std::string> prompt_mode(std::string_view current) {
    while (true) {
        std::wcout << L"Taskbar mode ["
                   << ac::encoding::to_utf16(current)
                   << L"]: ";
        std::wcout.flush();
        const auto value = read_console_line();
        if (!value) {
            return std::nullopt;
        }
        if (value->empty()) {
            return std::string {current};
        }
        const auto lower = ascii_lower(*value);
        if (taskbar::defaults::is_mode(lower)) {
            return lower;
        }
        std::wcout << L"Enter live or cache.\n";
    }
}

std::optional<bool> prompt_yes(std::string_view question) {
    while (true) {
        std::wcout << ac::encoding::to_utf16(question) << L' ';
        std::wcout.flush();
        const auto answer = read_console_line();
        if (!answer) {
            return std::nullopt;
        }
        if (answer->empty() || *answer == "y" || *answer == "Y") {
            return true;
        }
        if (*answer == "n" || *answer == "N") {
            return false;
        }
        std::wcout << L"Enter Y or N.\n";
    }
}

std::optional<bool> prompt_init_logging() {
    while (true) {
        std::wcout << L"Enable logging [on]: ";
        std::wcout.flush();
        const auto answer = read_console_line();
        if (!answer) {
            return std::nullopt;
        }
        const auto token = ascii_lower(*answer);
        if (token.empty() || token == "on") {
            return true;
        }
        if (token == "off") {
            return false;
        }
        std::wcout << L"Enter on or off.\n";
    }
}

std::optional<Settings> prompt_init_settings() {
    const auto directory = prompt_directory(
        L"Taskbar directory",
        taskbar::defaults::directory
    );
    if (!directory) {
        return std::nullopt;
    }
    const auto mode = prompt_mode(taskbar::defaults::mode);
    if (!mode) {
        return std::nullopt;
    }
    const auto logging = prompt_init_logging();
    if (!logging) {
        return std::nullopt;
    }
    return Settings {*directory, *mode, *logging};
}

std::optional<Settings> prompt_settings(const Settings& current) {
    const auto directory = prompt_directory(
        L"Taskbar data directory",
        current.directory
    );
    if (!directory) {
        return std::nullopt;
    }
    const auto mode = prompt_mode(current.mode);
    if (!mode) {
        return std::nullopt;
    }
    while (true) {
        std::wcout << L"Enable logging? ["
                   << (current.logging ? L"Y/n" : L"y/N")
                   << L"]: ";
        std::wcout.flush();
        const auto answer = read_console_line();
        if (!answer) {
            return std::nullopt;
        }
        if (answer->empty()) {
            return Settings {*directory, *mode, current.logging};
        }
        if (*answer == "y" || *answer == "Y") {
            return Settings {*directory, *mode, true};
        }
        if (*answer == "n" || *answer == "N") {
            return Settings {*directory, *mode, false};
        }
        std::wcout << L"Enter Y or n.\n";
    }
}

void show_settings(const Settings& settings) {
    std::wcout
        << L"\ntaskbar_config\n"
        << L"directory = " << ac::encoding::to_utf16(settings.directory) << L'\n'
        << L"mode = " << ac::encoding::to_utf16(settings.mode) << L'\n';
}

int launch_taskbar_builder(const std::wstring_view arguments) {
    const auto executable_path =
        ac::paths::bin_directory() / "taskbar_builder.exe";
    std::error_code error;
    const bool present = std::filesystem::exists(executable_path, error);
    if (error || !present) {
        taskbar_config.log_print("Missing {}.", executable_path.string());
        return 1;
    }

    std::wstring command_line = L"\"" + executable_path.wstring() + L"\"";
    if (!arguments.empty()) {
        command_line += L' ';
        command_line += arguments;
    }
    STARTUPINFOW startup_info {};
    startup_info.cb = sizeof(startup_info);
    const HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    const HANDLE error_handle = GetStdHandle(STD_ERROR_HANDLE);
    const bool share_stdio =
        input != nullptr && input != INVALID_HANDLE_VALUE &&
        output != nullptr && output != INVALID_HANDLE_VALUE &&
        error_handle != nullptr && error_handle != INVALID_HANDLE_VALUE;
    if (share_stdio) {
        SetHandleInformation(input, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
        SetHandleInformation(output, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
        SetHandleInformation(
            error_handle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT
        );
        startup_info.dwFlags = STARTF_USESTDHANDLES;
        startup_info.hStdInput = input;
        startup_info.hStdOutput = output;
        startup_info.hStdError = error_handle;
    }
    PROCESS_INFORMATION process_info {};
    if (!::CreateProcessW(
            executable_path.c_str(),
            command_line.data(),
            nullptr,
            nullptr,
            TRUE,
            0,
            nullptr,
            executable_path.parent_path().c_str(),
            &startup_info,
            &process_info
        )) {
        taskbar_config.log_print(
            "Unable to start {}.",
            executable_path.string()
        );
        return 1;
    }

    ::CloseHandle(process_info.hThread);
    ::WaitForSingleObject(process_info.hProcess, INFINITE);
    DWORD exit_code = 1;
    if (!::GetExitCodeProcess(process_info.hProcess, &exit_code)) {
        ::CloseHandle(process_info.hProcess);
        taskbar_config.log_print(
            "Unable to read the exit code from taskbar_builder.exe."
        );
        return 1;
    }
    ::CloseHandle(process_info.hProcess);
    const int code = static_cast<int>(exit_code);
    if (code != 0) {
        taskbar_config.log_print(
            "Taskbar application provisioning failed. taskbar_builder.exe "
            "exited {}.",
            code
        );
    }
    return code;
}

int offer_builder() {
    const auto accepted = prompt_yes("Run Taskbar builder now? [Y/n]:");
    if (!accepted) {
        return 1;
    }
    if (!*accepted) {
        taskbar_config.log_print("Taskbar builder was not requested.");
        return 0;
    }
    return launch_taskbar_builder({});
}

int edit_settings(const std::filesystem::path& path, Settings settings) {
    say_restart();
    while (true) {
        show_settings(settings);
        std::wcout
            << L"  1. Set directory\n"
            << L"  2. Set mode\n"
            << L"  3. Exit\n"
            << L"> ";
        std::wcout.flush();
        const auto selected = read_console_line();
        if (!selected) {
            return 0;
        }
        if (*selected == "3" || *selected == "q" || *selected == "Q") {
            return 0;
        }
        if (*selected == "1") {
            const auto directory = prompt_directory(
                L"Taskbar data directory",
                settings.directory
            );
            if (!directory) {
                return 1;
            }
            settings.directory = *directory;
        }
        else if (*selected == "2") {
            const auto mode = prompt_mode(settings.mode);
            if (!mode) {
                return 1;
            }
            settings.mode = *mode;
        }
        else {
            std::wcout << L"Enter 1, 2, or 3.\n";
            continue;
        }
        if (!write_ini(path, settings)) {
            return 1;
        }
        say_restart();
    }
}

} // namespace

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    taskbar_config.log_main("taskbar_config.exe started");
    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }

    const auto path = ac::paths::config_directory() / "taskbar.ini";
    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    if (error) {
        taskbar_config.log_print("Failed to inspect {}", path.string());
        return 1;
    }

    namespace req = ac::config::components_request;
    req::log_config_request(taskbar_config, *launch);
    if (launch->seed) {
        if (present) {
            req::log_seed_skipped(taskbar_config, "config/taskbar.ini");
        }
        else {
            req::log_writing_defaults(taskbar_config);
            if (!write_ini(path, Settings {})) {
                taskbar_config.log_print(
                    "Taskbar configuration was not initialized."
                );
                return 1;
            }
            req::log_configuration_initialized(taskbar_config);
        }
        return launch_taskbar_builder(L"--seed");
    }

    if (launch->init) {
        if (present) {
            req::log_initialization_skipped(
                taskbar_config,
                "config/taskbar.ini"
            );
        }
        else {
            req::log_configuration_missing(taskbar_config);
            const auto settings = prompt_init_settings();
            if (!settings) {
                taskbar_config.log_print(
                    "Taskbar configuration was not initialized."
                );
                return 1;
            }
            if (!write_ini(path, *settings)) {
                taskbar_config.log_print(
                    "Taskbar configuration was not initialized."
                );
                return 1;
            }
            req::log_configuration_initialized(taskbar_config);
            say_restart();
        }
        return offer_builder();
    }

    if (!present) {
        req::log_configuration_missing(taskbar_config);
        const auto settings = prompt_settings(Settings {});
        if (!settings || !write_ini(path, *settings)) {
            taskbar_config.log_print(
                "Taskbar configuration was not initialized."
            );
            return 1;
        }
        req::log_configuration_initialized(taskbar_config);
        say_restart();
        return 0;
    }

    return edit_settings(path, read_settings(path));
}
