import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.ini;
import auto_core.core.paths;
import taskbar_defaults;
import components_editor_request;
import config_menu;

import <Windows.h>;
import <iostream>;
import auto_core.core.shell;

namespace menu = ac::config_menu;

namespace {

ac::Component taskbar_config {
    "taskbar_config",
    ac::logging::config::LoggingScope {"taskbar"}
};

constexpr std::string_view on_off[] {"on", "off"};
constexpr std::string_view modes[] {"live", "cache"};

constexpr std::string_view menu_title =
    "Restart taskbar_ac.exe before directory and mode changes apply.\n"
    "Taskbar Configuration";

const menu::Setting menu_settings[] {
    {
        .key = "directory",
        .display_name = "Directory",
        .summary = "Folder that stores Taskbar data.",
        .default_value = taskbar::defaults::directory,
    },
    {
        .key = "mode",
        .display_name = "Mode",
        .summary =
            "live discovers taskbar positions, and cache reuses a usable map.",
        .default_value = taskbar::defaults::mode,
        .choices = modes,
    },
    {
        .key = "logging",
        .display_name = "Logging",
        .summary = "Controls logging for the Taskbar component.",
        .default_value = "on",
        .choices = on_off,
    },
};

struct Settings {
    std::string directory {std::string {taskbar::defaults::directory}};
    std::string mode {std::string {taskbar::defaults::mode}};
    bool logging = true;
};

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
    const auto contents = menu::with_header(
        menu_settings,
        taskbar::defaults::ini_for(
            settings.directory,
            settings.mode,
            settings.logging
        )
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

Settings read_settings(
    const std::filesystem::path& path,
    const bool report
) {
    Settings settings;
    const auto document = ac::ini::read(path);
    if (!document) {
        if (report) {
            taskbar_config.log_print(
                "config/taskbar.ini is malformed. Using compiled defaults until "
                "you save new settings."
            );
        }
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
        else if (report) {
            taskbar_config.log_print(
                "config/taskbar.ini mode is invalid. Using live."
            );
        }
    }
    if (const auto logging = document->find("taskbar", "logging")) {
        if (*logging == "on") {
            settings.logging = true;
        }
        else if (*logging == "off") {
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
    std::cout
        << "Restart taskbar_ac.exe before directory and mode changes apply.\n";
}

std::optional<std::string> read_console_line() {
    std::string input;
    if (!std::getline(std::cin, input)) {
        return std::nullopt;
    }
    const auto first = input.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return std::string {};
    }
    const auto last = input.find_last_not_of(" \t\r\n");
    return input.substr(first, last - first + 1);
}

std::optional<bool> prompt_yes(std::string_view question) {
    while (true) {
        std::cout << question << ' ';
        std::cout.flush();
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
        std::cout << "Enter Y or N.\n";
    }
}

std::filesystem::path ini_path() {
    return ac::paths::config_directory() / "taskbar.ini";
}

std::string current_text(const menu::Setting& setting) {
    const auto settings = read_settings(ini_path(), false);
    if (setting.key == "directory") {
        return settings.directory;
    }
    if (setting.key == "mode") {
        return settings.mode;
    }
    return settings.logging ? "on" : "off";
}

menu::ApplyResult apply_setting(
    const menu::Setting& setting,
    const std::string_view value
) {
    auto settings = read_settings(ini_path(), false);
    if (setting.key == "directory") {
        settings.directory = std::string {value};
    }
    else if (setting.key == "mode") {
        settings.mode = std::string {value};
    }
    else {
        settings.logging = value == "on";
    }
    if (!write_ini(ini_path(), settings)) {
        return menu::ApplyResult::failed;
    }
    return menu::ApplyResult::stored;
}

int run_configuration_menu() {
    (void)read_settings(ini_path(), true);
    const auto result = menu::run_menu(
        menu_title,
        menu_settings,
        current_text,
        apply_setting,
        std::cin,
        std::cout
    );
    return result.ok ? 0 : 1;
}

int offer_configuration() {
    (void)read_settings(ini_path(), true);
    const auto offer = menu::offer_configuration(
        "Taskbar Configuration",
        menu_settings,
        current_text,
        std::cin,
        std::cout
    );
    if (offer == menu::OfferResult::failed) {
        return 1;
    }
    if (offer == menu::OfferResult::configure) {
        const auto result = menu::run_menu(
            menu_title,
            menu_settings,
            current_text,
            apply_setting,
            std::cin,
            std::cout
        );
        return result.ok ? 0 : 1;
    }
    return 0;
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

} // namespace

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    taskbar_config.log_main("taskbar_config.exe started");
    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }

    const auto path = ini_path();
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
            if (!write_ini(path, Settings {})) {
                taskbar_config.log_print(
                    "Taskbar configuration was not initialized."
                );
                return 1;
            }
            if (offer_configuration() != 0) {
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
        if (!write_ini(path, Settings {})) {
            taskbar_config.log_print(
                "Taskbar configuration was not initialized."
            );
            return 1;
        }
        req::log_configuration_initialized(taskbar_config);
    }

    return run_configuration_menu();
}
