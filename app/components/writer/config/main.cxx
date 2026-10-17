#include <Windows.h>

import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.encoding;
import auto_core.core.ini;
import auto_core.core.paths;
import writer_defaults;
import components_editor_request;
import config_menu;

import <iostream>;
import auto_core.core.shell;

namespace menu = ac::config_menu;

namespace {

ac::Component writer_config {
    "writer_config",
    ac::logging::config::LoggingScope {"writer"}
};

constexpr std::string_view on_off[] {"on", "off"};

const menu::Setting menu_settings[] {
    {
        .key = "directory",
        .display_name = "Directory",
        .summary = "Folder that stores Writer data.",
        .default_value = writer::defaults::directory,
        .directory_path = true,
    },
    {
        .key = "notes_subdirectory",
        .display_name = "Notes subdirectory",
        .summary = "Relative notes folder inside the Writer directory, or an absolute path used directly.",
        .default_value = writer::defaults::notes_subdirectory,
        .constraint = "Enter a relative or absolute notes path.",
        .directory_path = true,
    },
    {
        .key = "logging",
        .display_name = "Logging",
        .summary = "Controls logging for the Writer component.",
        .default_value = "on",
        .choices = on_off,
    },
};

bool is_accepted_notes_subdirectory(std::string_view value) {
    if (value.empty()) {
        return false;
    }
    try {
        const std::filesystem::path path {
            ac::encoding::to_utf16(std::string {value})
        };
        if (path.empty()) {
            return false;
        }
        if (path.is_absolute()) {
            return true;
        }
        return !path.has_root_name() && !path.has_root_directory();
    }
    catch (...) {
        return false;
    }
}

std::filesystem::path writer_ini_path() {
    return ac::paths::component_config_file("writer");
}

struct Settings {
    std::string directory {std::string {writer::defaults::directory}};
    std::string notes {std::string {writer::defaults::notes_subdirectory}};
    bool logging = true;
};

Settings read_settings() {
    Settings settings;
    const auto document = ac::ini::read(writer_ini_path());
    if (!document) {
        return settings;
    }
    if (const auto value = document->find("writer", "directory");
        value && !value->empty()) {
        settings.directory = std::string {*value};
    }
    if (const auto value = document->find("writer", "notes_subdirectory");
        value && !value->empty()) {
        settings.notes = std::string {*value};
    }
    if (const auto value = document->find("writer", "logging")) {
        if (*value == "off") {
            settings.logging = false;
        }
        else if (*value == "on") {
            settings.logging = true;
        }
        else {
            settings.logging = ac::logging::config::component_logging_default();
        }
    }
    else if (document) {
        settings.logging = ac::logging::config::component_logging_default();
    }
    return settings;
}

bool write_writer_ini(const Settings& settings) {
    std::error_code create_error;
    std::filesystem::create_directories(
        ac::paths::config_directory(),
        create_error
    );
    if (create_error) {
        writer_config.print(
            "Failed to create config directory: {}",
            create_error.message()
        );
        return false;
    }

    const auto path = writer_ini_path();
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        writer_config.print("Failed to create {}", path.string());
        return false;
    }
    const auto contents =
        "# directory = relative or absolute path\n"
        "# notes_subdirectory = relative or absolute path\n" +
        menu::with_header(
            menu_settings,
            writer::defaults::ini_for(
                settings.directory,
                settings.notes,
                settings.logging
            )
        );
    output.write(
        contents.data(),
        static_cast<std::streamsize>(contents.size())
    );
    output.close();
    return static_cast<bool>(output);
}

std::wstring quote_argument(std::wstring_view value) {
    std::wstring quoted;
    quoted.reserve(value.size() + 2);
    quoted.push_back(L'"');
    quoted.append(value);
    quoted.push_back(L'"');
    return quoted;
}

int launch_writer_editor(const std::wstring_view arguments) {
    const auto executable_path =
        ac::paths::bin_directory() / "writer_editor.exe";
    std::error_code error;
    const bool present = std::filesystem::exists(executable_path, error);
    if (error || !present) {
        writer_config.print("Missing {}.", executable_path.string());
        return 1;
    }

    std::wstring command =
        quote_argument(executable_path.wstring()) + L" " +
        quote_argument(arguments);
    STARTUPINFOW startup {};
    startup.cb = sizeof(startup);
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
            error_handle,
            HANDLE_FLAG_INHERIT,
            HANDLE_FLAG_INHERIT
        );
        startup.dwFlags = STARTF_USESTDHANDLES;
        startup.hStdInput = input;
        startup.hStdOutput = output;
        startup.hStdError = error_handle;
    }
    PROCESS_INFORMATION process {};
    if (!CreateProcessW(
            executable_path.c_str(),
            command.data(),
            nullptr,
            nullptr,
            TRUE,
            0,
            nullptr,
            executable_path.parent_path().c_str(),
            &startup,
            &process
        )) {
        writer_config.print(
            "Unable to start {}.",
            executable_path.string()
        );
        return 1;
    }

    CloseHandle(process.hThread);
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exit_code = 1;
    if (!GetExitCodeProcess(process.hProcess, &exit_code)) {
        CloseHandle(process.hProcess);
        writer_config.print(
            "Unable to read the exit code from {}.",
            executable_path.string()
        );
        return 1;
    }
    CloseHandle(process.hProcess);
    const int code = static_cast<int>(exit_code);
    if (code != 0) {
        writer_config.print(
            "{} exited {}.",
            executable_path.string(),
            code
        );
    }
    return code;
}

std::string current_text(const menu::Setting& setting) {
    const auto settings = read_settings();
    if (setting.key == "directory") {
        return settings.directory;
    }
    if (setting.key == "notes_subdirectory") {
        return settings.notes;
    }
    return settings.logging ? "on" : "off";
}

menu::ApplyResult apply_setting(
    const menu::Setting& setting,
    const std::string_view value
) {
    auto settings = read_settings();
    if (setting.key == "directory") {
        settings.directory = std::string {value};
    }
    else if (setting.key == "notes_subdirectory") {
        if (!is_accepted_notes_subdirectory(value)) {
            writer_config.log_debug(
                "notes_subdirectory must be a relative or absolute path."
            );
            return menu::ApplyResult::invalid;
        }
        settings.notes = std::string {value};
    }
    else {
        settings.logging = value == "on";
    }
    if (!write_writer_ini(settings)) {
        return menu::ApplyResult::failed;
    }
    return menu::ApplyResult::stored;
}

int run_configuration_menu() {
    const auto result = menu::run_menu(
        "Writer Configuration",
        menu_settings,
        current_text,
        apply_setting,
        std::cin,
        std::cout
    );
    return result.ok ? 0 : 1;
}

int offer_configuration() {
    const auto result = menu::prompt_settings(
        menu_settings,
        current_text,
        apply_setting,
        std::cin,
        std::cout
    );
    return result.ok ? 0 : 1;
}

void activate_own_console() {
    const HWND console = GetConsoleWindow();
    if (console == nullptr) {
        return;
    }
    if (IsIconic(console)) {
        (void)ShowWindow(console, SW_RESTORE);
    }
    (void)BringWindowToTop(console);
    (void)SetForegroundWindow(console);
    (void)SetFocus(console);
}

} // namespace

int main(int argc, char* argv[]) {
    std::setvbuf(stdin, nullptr, _IONBF, 0);
    ac::shell::set_process_app_user_model_id();
    writer_config.log_info("writer_config.exe started");

    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }

    const auto path = writer_ini_path();
    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    if (error) {
        writer_config.print("Failed to inspect {}", path.string());
        return 1;
    }

    namespace req = ac::config::components_request;
    req::log_config_request(writer_config, *launch);
    if (launch->seed) {
        if (present) {
            req::log_seed_skipped(writer_config, "config/components/writer.ini");
        }
        else {
            req::log_writing_defaults(writer_config);
            if (!write_writer_ini({})) {
                writer_config.log_debug(
                    "Writer configuration was not initialized."
                );
                return 1;
            }
            req::log_configuration_initialized(writer_config);
        }
        return req::finish_component_initialization(
            writer_config,
            launch_writer_editor(L"--seed"),
            "Writer"
        );
    }

    if (launch->init) {
        if (present) {
            req::log_initialization_skipped(writer_config, "config/components/writer.ini");
        }
        else {
            req::log_configuration_missing(writer_config);
            if (!write_writer_ini({})) {
                writer_config.log_debug(
                    "Writer configuration was not initialized."
                );
                return 1;
            }
            if (offer_configuration() != 0) {
                writer_config.log_debug(
                    "Writer configuration was not initialized."
                );
                return 1;
            }
            req::log_configuration_initialized(writer_config);
        }
        return req::finish_component_initialization(
            writer_config,
            launch_writer_editor(L"--init"),
            "Writer"
        );
    }

    if (!present) {
        if (!write_writer_ini({})) {
            writer_config.log_debug(
                "Writer configuration was not initialized."
            );
            return 1;
        }
    }

    activate_own_console();
    return run_configuration_menu();
}
