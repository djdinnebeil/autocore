import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.ini;
import auto_core.core.paths;
import itunes_defaults;
import components_editor_request;

import <iostream>;
import <Windows.h>;
import auto_core.core.shell;

namespace {

std::string_view trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

bool write_bytes(
    ac::Component& log,
    const std::filesystem::path& path,
    std::string_view contents
) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) {
        log.log_print(
            "Failed to create config directory: {}",
            error.message()
        );
        return false;
    }
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        log.log_print("Failed to create {}", path.string());
        return false;
    }
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    output.close();
    if (!output) {
        log.log_print("Failed to write {}", path.string());
        return false;
    }
    return true;
}

struct PromptValues {
    std::string directory {itunes::defaults::directory};
    bool auto_start = itunes::defaults::auto_start;
    bool logging = true;
};

PromptValues values_in_file(
    ac::Component& log,
    const std::filesystem::path& path,
    bool& readable
) {
    readable = false;
    const auto document = ac::ini::read(path);
    if (!document) {
        log.log_print("Failed to read {}", path.string());
        return {};
    }
    readable = true;

    PromptValues values;
    if (const auto directory = document->find("itunes", "directory")) {
        const auto text = trim(*directory);
        if (!text.empty()) {
            values.directory = std::string {text};
        }
    }

    if (const auto auto_start = document->find("itunes", "auto_start")) {
        if (*auto_start == "on") {
            values.auto_start = true;
        }
        else if (*auto_start == "off") {
            values.auto_start = false;
        }
    }

    if (const auto logging = document->find("itunes", "logging")) {
        if (*logging == "off" || *logging == "false") {
            values.logging = false;
        }
        else if (*logging == "on" || *logging == "true") {
            values.logging = true;
        }
        else {
            values.logging = ac::logging::config::component_logging_default();
        }
    }
    else {
        values.logging = ac::logging::config::component_logging_default();
    }
    return values;
}

bool cancelled(ac::Component& log, std::string_view value) {
    if (value != "cancel") {
        return false;
    }
    log.log_print("Cancelled.");
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

std::optional<PromptValues> prompt_values(
    ac::Component& log,
    const PromptValues& current
) {
    std::cout << "iTunes directory [" << current.directory << "]: ";
    std::string directory_input;
    if (!std::getline(std::cin, directory_input)) {
        log.log_print("Failed to read directory.");
        return std::nullopt;
    }
    const auto directory = trim(directory_input);
    if (cancelled(log, directory)) {
        return std::nullopt;
    }
    PromptValues values = current;
    if (!directory.empty()) {
        values.directory = std::string {directory};
    }

    while (true) {
        std::cout
            << "Auto-start iTunes ["
            << (current.auto_start ? "on" : "off")
            << "]: ";
        std::string auto_start_input;
        if (!std::getline(std::cin, auto_start_input)) {
            log.log_print("Failed to read auto_start.");
            return std::nullopt;
        }
        const auto auto_start_value = trim(auto_start_input);
        if (cancelled(log, auto_start_value)) {
            return std::nullopt;
        }
        if (auto_start_value.empty()) {
            values.auto_start = current.auto_start;
            break;
        }
        const auto auto_start_token = lower_token(auto_start_value);
        if (auto_start_token == "on") {
            values.auto_start = true;
            break;
        }
        if (auto_start_token == "off") {
            values.auto_start = false;
            break;
        }
        std::cout << "Enter on or off.\n";
    }

    while (true) {
        std::cout << "Enable logging ["
                  << (current.logging ? "on" : "off")
                  << "]: ";
        std::string logging_input;
        if (!std::getline(std::cin, logging_input)) {
            log.log_print("Failed to read logging.");
            return std::nullopt;
        }
        const auto logging_value = trim(logging_input);
        if (cancelled(log, logging_value)) {
            return std::nullopt;
        }
        if (logging_value.empty()) {
            values.logging = current.logging;
            break;
        }
        const auto logging_token = lower_token(logging_value);
        if (logging_token == "on") {
            values.logging = true;
            break;
        }
        if (logging_token == "off") {
            values.logging = false;
            break;
        }
        std::cout << "Enter on or off.\n";
    }
    return values;
}

bool write_prompted(
    ac::Component& log,
    const std::filesystem::path& path,
    const PromptValues& values
) {
    const auto contents = itunes::defaults::ini_for(
        values.directory,
        values.auto_start,
        values.logging
    );
    if (!write_bytes(log, path, contents)) {
        return false;
    }
    log.log_print("Wrote {}", path.string());
    return true;
}

std::wstring quote_argument(std::wstring_view value) {
    std::wstring quoted;
    quoted.reserve(value.size() + 2);
    quoted.push_back(L'"');
    quoted.append(value);
    quoted.push_back(L'"');
    return quoted;
}

int launch_owner(
    ac::Component& log,
    std::wstring_view executable_name,
    std::wstring_view arguments
) {
    const auto executable_path = ac::paths::bin_directory() / executable_name;
    std::error_code error;
    const bool present = std::filesystem::exists(executable_path, error);
    if (error || !present) {
        log.log_print("Missing {}.", executable_path.string());
        return 1;
    }

    std::wstring command_line =
        quote_argument(executable_path.wstring()) + L" " +
        quote_argument(arguments);
    STARTUPINFOW startup_info {};
    startup_info.cb = sizeof(startup_info);
    PROCESS_INFORMATION process_info {};
    if (!::CreateProcessW(
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
        log.log_print("Unable to start {}.", executable_path.string());
        return 1;
    }

    ::CloseHandle(process_info.hThread);
    ::WaitForSingleObject(process_info.hProcess, INFINITE);
    DWORD exit_code = 1;
    if (!::GetExitCodeProcess(process_info.hProcess, &exit_code)) {
        ::CloseHandle(process_info.hProcess);
        log.log_print(
            "Unable to read the exit code from {}.",
            executable_path.string()
        );
        return 1;
    }
    ::CloseHandle(process_info.hProcess);
    const int code = static_cast<int>(exit_code);
    if (code != 0) {
        log.log_print("{} exited {}.", executable_path.string(), code);
    }
    return code;
}

int launch_owned_stores(ac::Component& log, std::wstring_view formatter_mode) {
    if (const int database = launch_owner(log, L"itunes_db.exe", L"--seed");
        database != 0) {
        return database;
    }
    return launch_owner(log, L"itunes_formatter.exe", formatter_mode);
}

} // namespace

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    ac::Component itunes_config {
        "itunes_config",
        ac::logging::config::LoggingScope {"itunes"}
    };
    itunes_config.log_main("itunes_config.exe started");

    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }

    const auto path = ac::paths::config_directory() / "itunes.ini";
    std::error_code error;
    const bool exists = std::filesystem::exists(path, error);
    if (error) {
        itunes_config.log_print("Failed to inspect {}", path.string());
        return 1;
    }

    namespace req = ac::config::components_request;
    req::log_config_request(itunes_config, *launch);
    if (launch->seed) {
        if (exists) {
            req::log_seed_skipped(itunes_config, "config/itunes.ini");
        }
        else {
            req::log_writing_defaults(itunes_config);
            if (!write_bytes(itunes_config, path, itunes::defaults::ini_text)) {
                return 1;
            }
            req::log_configuration_initialized(itunes_config);
        }
        return launch_owned_stores(itunes_config, L"--seed");
    }

    if (launch->init) {
        if (exists) {
            req::log_initialization_skipped(itunes_config, "config/itunes.ini");
        }
        else {
            req::log_configuration_missing(itunes_config);
            const auto values = prompt_values(itunes_config, {});
            if (!values) {
                return 1;
            }
            if (!write_prompted(itunes_config, path, *values)) {
                return 1;
            }
        }
        return launch_owned_stores(itunes_config, L"--init");
    }

    if (exists) {
        bool readable = false;
        const auto current = values_in_file(itunes_config, path, readable);
        if (!readable) {
            return 1;
        }
        const auto values = prompt_values(itunes_config, current);
        if (!values) {
            return 1;
        }
        if (!write_prompted(itunes_config, path, *values)) {
            return 1;
        }
        return 0;
    }

    const auto values = prompt_values(itunes_config, {});
    if (!values) {
        return 1;
    }
    if (!write_prompted(itunes_config, path, *values)) {
        return 1;
    }
    return 0;
}
