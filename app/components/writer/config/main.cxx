#include <Windows.h>

import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.encoding;
import auto_core.core.ini;
import auto_core.core.paths;
import writer_defaults;
import components_editor_request;

import <iostream>;
import auto_core.core.shell;

namespace {

ac::Component writer_config {
    "writer_config",
    ac::logging::config::LoggingScope {"writer"}
};

std::string_view trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

bool is_relative_subdirectory(std::string_view value) {
    if (value.empty()) {
        return false;
    }
    try {
        const std::filesystem::path path {
            ac::encoding::to_utf16(std::string {value})
        };
        return !path.empty() &&
            !path.has_root_name() &&
            !path.has_root_directory();
    }
    catch (...) {
        return false;
    }
}

std::filesystem::path writer_ini_path() {
    return ac::paths::config_directory() / "writer.ini";
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

bool write_writer_ini(
    std::string_view directory,
    std::string_view notes,
    bool logging
) {
    std::error_code create_error;
    std::filesystem::create_directories(
        ac::paths::config_directory(),
        create_error
    );
    if (create_error) {
        writer_config.log_print(
            "Failed to create config directory: {}",
            create_error.message()
        );
        return false;
    }

    const auto path = writer_ini_path();
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        writer_config.log_print("Failed to create {}", path.string());
        return false;
    }
    const auto contents = writer::defaults::ini_for(directory, notes, logging);
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
        writer_config.log_print("Missing {}.", executable_path.string());
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
        writer_config.log_print(
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
        writer_config.log_print(
            "Unable to read the exit code from {}.",
            executable_path.string()
        );
        return 1;
    }
    CloseHandle(process.hProcess);
    const int code = static_cast<int>(exit_code);
    if (code != 0) {
        writer_config.log_print(
            "{} exited {}.",
            executable_path.string(),
            code
        );
    }
    return code;
}

std::optional<std::string> read_input_line() {
    const HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    if (input == nullptr || input == INVALID_HANDLE_VALUE) {
        return std::nullopt;
    }

    std::string line;
    while (true) {
        char byte = 0;
        DWORD read = 0;
        if (!ReadFile(input, &byte, 1, &read, nullptr) || read == 0) {
            if (line.empty()) {
                return std::nullopt;
            }
            return line;
        }
        if (byte == '\n') {
            return line;
        }
        if (byte != '\r') {
            line.push_back(byte);
        }
    }
}

std::optional<std::string> prompt_line(std::string_view label) {
    std::cout << label;
    std::string input;
    if (!std::getline(std::cin, input)) {
        return std::nullopt;
    }
    return std::string {trim(input)};
}

std::optional<std::string> prompt_directory(std::string_view current) {
    const std::string label =
        "Writer directory [" + std::string {current} + "]: ";
    auto input = prompt_line(label);
    if (!input) {
        return std::nullopt;
    }
    if (input->empty()) {
        return std::string {current};
    }
    return input;
}

std::optional<std::string> prompt_notes(std::string_view keep) {
    while (true) {
        const std::string label =
            "Notes subdirectory [" + std::string {keep} + "]: ";
        auto input = prompt_line(label);
        if (!input) {
            return std::nullopt;
        }
        if (input->empty()) {
            return std::string {keep};
        }
        if (!is_relative_subdirectory(*input)) {
            writer_config.log_print(
                "notes_subdirectory must be a relative path."
            );
            std::cout << "Enter a relative notes subdirectory.\n";
            continue;
        }
        return input;
    }
}

bool cancelled(const std::string_view value) {
    if (value != "cancel") {
        return false;
    }
    writer_config.log_print("Cancelled.");
    return true;
}

std::string lower_token(std::string_view value) {
    std::string token {value};
    for (char& character : token) {
        if (character >= 'A' && character <= 'Z') {
            character = static_cast<char>(character - 'A' + 'a');
        }
    }
    return token;
}

std::optional<std::string> prompt_init_directory() {
    std::cout
        << "Writer directory ["
        << writer::defaults::directory
        << "]: ";
    std::cout.flush();
    const auto input = read_input_line();
    if (!input) {
        return std::nullopt;
    }
    const auto directory = trim(*input);
    if (cancelled(directory)) {
        return std::nullopt;
    }
    if (directory.empty()) {
        return std::string {writer::defaults::directory};
    }
    return std::string {directory};
}

std::optional<std::string> prompt_init_notes() {
    while (true) {
        std::cout
            << "Notes subdirectory ["
            << writer::defaults::notes_subdirectory
            << "]: ";
        std::cout.flush();
        const auto input = read_input_line();
        if (!input) {
            return std::nullopt;
        }
        const auto notes = trim(*input);
        if (cancelled(notes)) {
            return std::nullopt;
        }
        if (notes.empty()) {
            return std::string {writer::defaults::notes_subdirectory};
        }
        if (!is_relative_subdirectory(notes)) {
            writer_config.log_print(
                "notes_subdirectory must be a relative path."
            );
            std::cout << "Enter a relative notes subdirectory.\n";
            continue;
        }
        return std::string {notes};
    }
}

std::optional<bool> prompt_init_logging() {
    while (true) {
        std::cout << "Enable logging [on]: ";
        std::cout.flush();
        const auto input = read_input_line();
        if (!input) {
            return std::nullopt;
        }
        const auto value = trim(*input);
        if (cancelled(value)) {
            return std::nullopt;
        }
        if (value.empty()) {
            return true;
        }
        const auto token = lower_token(value);
        if (token == "on") {
            return true;
        }
        if (token == "off") {
            return false;
        }
        std::cout << "Enter on or off.\n";
    }
}

std::optional<Settings> prompt_new_settings() {
    const auto directory = prompt_directory(writer::defaults::directory);
    if (!directory) {
        return std::nullopt;
    }
    const auto notes = prompt_notes(writer::defaults::notes_subdirectory);
    if (!notes) {
        return std::nullopt;
    }
    while (true) {
        std::cout << "Enable logging? [Y/n]: ";
        std::string answer;
        if (!std::getline(std::cin, answer)) {
            return std::nullopt;
        }
        const auto value = trim(answer);
        if (value.empty() || value == "y" || value == "Y") {
            return Settings {*directory, *notes, true};
        }
        if (value == "n" || value == "N") {
            return Settings {*directory, *notes, false};
        }
        std::cout << "Enter Y or n.\n";
    }
}

void show_settings(const Settings& settings) {
    std::cout
        << "\nwriter_config\n"
        << "directory = " << settings.directory << '\n'
        << "notes_subdirectory = " << settings.notes << '\n';
}

std::string notes_keep_value(const Settings& settings) {
    if (is_relative_subdirectory(settings.notes)) {
        return settings.notes;
    }
    writer_config.log_print(
        "notes_subdirectory must be a relative path. Blank stores {}.",
        writer::defaults::notes_subdirectory
    );
    return std::string {writer::defaults::notes_subdirectory};
}

void print_menu() {
    std::cout
        << "  1. Change directory\n"
        << "  2. Change notes subdirectory\n"
        << "  3. Exit\n"
        << "> ";
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

int edit_settings() {
    activate_own_console();
    while (true) {
        const Settings settings = read_settings();
        show_settings(settings);
        print_menu();
        std::string choice;
        if (!std::getline(std::cin, choice)) {
            return 0;
        }
        if (choice == "1") {
            const auto directory = prompt_directory(settings.directory);
            if (!directory) {
                return 1;
            }
            const auto notes = notes_keep_value(settings);
            if (!write_writer_ini(*directory, notes, settings.logging)) {
                return 1;
            }
        }
        else if (choice == "2") {
            const auto notes = prompt_notes(notes_keep_value(settings));
            if (!notes) {
                return 1;
            }
            if (!write_writer_ini(settings.directory, *notes, settings.logging)) {
                return 1;
            }
        }
        else if (choice == "3" || choice == "q" || choice == "Q") {
            return 0;
        }
        else {
            std::cout << "Unknown option.\n";
        }
    }
}

} // namespace

int main(int argc, char* argv[]) {
    std::setvbuf(stdin, nullptr, _IONBF, 0);
    ac::shell::set_process_app_user_model_id();
    writer_config.log_main("writer_config.exe started");

    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }

    const auto path = writer_ini_path();
    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    if (error) {
        writer_config.log_print("Failed to inspect {}", path.string());
        return 1;
    }

    namespace req = ac::config::components_request;
    req::log_config_request(writer_config, *launch);
    if (launch->seed) {
        if (present) {
            req::log_seed_skipped(writer_config, "config/writer.ini");
        }
        else {
            req::log_writing_defaults(writer_config);
            if (!write_writer_ini(
                    writer::defaults::directory,
                    writer::defaults::notes_subdirectory,
                    true
                )) {
                writer_config.log_print(
                    "Writer configuration was not initialized."
                );
                return 1;
            }
            req::log_configuration_initialized(writer_config);
        }
        return launch_writer_editor(L"--seed");
    }

    if (launch->init) {
        if (present) {
            req::log_initialization_skipped(writer_config, "config/writer.ini");
        }
        else {
            req::log_configuration_missing(writer_config);
            const auto directory = prompt_init_directory();
            if (!directory) {
                writer_config.log_print(
                    "Writer configuration was not initialized."
                );
                return 1;
            }
            const auto notes = prompt_init_notes();
            if (!notes) {
                writer_config.log_print(
                    "Writer configuration was not initialized."
                );
                return 1;
            }
            const auto logging = prompt_init_logging();
            if (!logging) {
                writer_config.log_print(
                    "Writer configuration was not initialized."
                );
                return 1;
            }
            if (!write_writer_ini(*directory, *notes, *logging)) {
                writer_config.log_print(
                    "Writer configuration was not initialized."
                );
                return 1;
            }
            req::log_configuration_initialized(writer_config);
        }
        return launch_writer_editor(L"--init");
    }

    if (!present) {
        const auto settings = prompt_new_settings();
        if (!settings ||
            !write_writer_ini(
                settings->directory,
                settings->notes,
                settings->logging
            )) {
            writer_config.log_print(
                "Writer configuration was not initialized."
            );
            return 1;
        }
    }

    return edit_settings();
}
