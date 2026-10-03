#include <Windows.h>

#include "../shared/wake_history_detail.hpp"

import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.ini;
import auto_core.core.paths;
import wake_defaults;
import components_editor_request;

import <iostream>;
import auto_core.core.shell;

namespace {

ac::Component wake_config {
    "wake_config",
    ac::logging::config::LoggingScope {"wake"}
};

[[nodiscard]]
std::string trim(const std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t");
    return std::string {value.substr(first, last - first + 1)};
}

[[nodiscard]]
std::filesystem::path ini_path() {
    return ac::paths::config_directory() / "wake.ini";
}

[[nodiscard]]
std::string current_directory() {
    const auto document = ac::ini::read(ini_path());
    if (!document) {
        return std::string {wake::defaults::directory};
    }
    const auto value = document->find("wake", "directory");
    if (!value || value->empty()) {
        return std::string {wake::defaults::directory};
    }
    return std::string {*value};
}

[[nodiscard]]
bool current_logging() {
    const auto document = ac::ini::read(ini_path());
    if (!document) {
        return ac::logging::config::component_logging_default();
    }
    const auto value = document->find("wake", "logging");
    if (!value) {
        return ac::logging::config::component_logging_default();
    }
    if (*value == "on" || *value == "true") {
        return true;
    }
    if (*value == "off" || *value == "false") {
        return false;
    }
    return ac::logging::config::component_logging_default();
}

[[nodiscard]]
bool write_directory(const std::string_view directory, const bool logging) {
    const auto path = ini_path();
    std::error_code create_error;
    std::filesystem::create_directories(path.parent_path(), create_error);
    if (create_error) {
        wake_config.log_print(
            "Failed to create config directory: {}",
            create_error.message()
        );
        return false;
    }

    const auto contents = wake::defaults::ini_for(directory, logging);
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        wake_config.log_print("Failed to create {}", path.string());
        return false;
    }
    output.write(
        contents.data(),
        static_cast<std::streamsize>(contents.size())
    );
    output.close();
    if (!output) {
        wake_config.log_print("Failed to write {}", path.string());
        return false;
    }
    wake_config.log_print("Wrote {}", path.string());
    return true;
}

[[nodiscard]]
std::optional<std::string> prompt_directory(const std::string_view current) {
    std::cout << "Wake data directory [" << current << "]: ";
    std::string input;
    if (!std::getline(std::cin, input)) {
        wake_config.log_print("Failed to read Wake directory.");
        return std::nullopt;
    }
    const auto value = trim(input);
    if (value.empty()) {
        return std::string {current};
    }
    return value;
}

[[nodiscard]]
std::optional<bool> prompt_logging(const bool current) {
    while (true) {
        std::cout << "Enable logging? [" << (current ? "Y/n" : "y/N") << "]: ";
        std::string input;
        if (!std::getline(std::cin, input)) {
            return std::nullopt;
        }
        const auto value = trim(input);
        if (value.empty()) {
            return current;
        }
        if (value == "y" || value == "Y") {
            return true;
        }
        if (value == "n" || value == "N") {
            return false;
        }
        std::cout << "Enter Y or n.\n";
    }
}

std::wstring quote_argument(const std::wstring_view value) {
    std::wstring quoted;
    quoted.reserve(value.size() + 2);
    quoted.push_back(L'"');
    quoted.append(value);
    quoted.push_back(L'"');
    return quoted;
}

[[nodiscard]]
int launch_snapshot() {
    const auto executable_path =
        ac::paths::bin_directory() / L"wake_ac.exe";
    std::error_code error;
    const bool present = std::filesystem::exists(executable_path, error);
    if (error || !present) {
        wake_config.log_print("Missing {}.", executable_path.string());
        return 1;
    }

    std::wstring command =
        quote_argument(executable_path.wstring()) + L" " +
        quote_argument(L"--snapshot");
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
        SetHandleInformation(error_handle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
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
        wake_config.log_print("Unable to start {}.", executable_path.string());
        return 1;
    }

    CloseHandle(process.hThread);
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exit_code = 1;
    if (!GetExitCodeProcess(process.hProcess, &exit_code)) {
        CloseHandle(process.hProcess);
        wake_config.log_print(
            "Unable to read the exit code from {}.",
            executable_path.string()
        );
        return 1;
    }
    CloseHandle(process.hProcess);
    const int code = static_cast<int>(exit_code);
    if (code != 0) {
        wake_config.log_print("{} exited {}.", executable_path.string(), code);
    }
    return code;
}

[[nodiscard]]
int provision_history() {
    const auto directory = ac::paths::wake_directory();
    if (wake::history::all_history_files_present(directory)) {
        wake_config.log_print("Wake history is already present.");
        return 0;
    }
    wake_config.log_print("Launching wake_ac.exe --snapshot.");
    return launch_snapshot();
}

[[nodiscard]]
int edit_directory() {
    const auto directory = prompt_directory(current_directory());
    if (!directory) {
        return 1;
    }
    const auto logging = prompt_logging(current_logging());
    if (!logging) {
        return 1;
    }
    if (!write_directory(*directory, *logging)) {
        return 1;
    }
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    wake_config.log_main("wake_config.exe started");

    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }

    std::error_code error;
    const bool exists = std::filesystem::exists(ini_path(), error);
    if (error) {
        wake_config.log_print("Failed to inspect {}", ini_path().string());
        return 1;
    }

    namespace req = ac::config::components_request;
    req::log_config_request(wake_config, *launch);
    if (launch->seed) {
        if (exists) {
            req::log_seed_skipped(wake_config, "config/wake.ini");
        }
        else {
            req::log_writing_defaults(wake_config);
            if (!write_directory(wake::defaults::directory, true)) {
                return 1;
            }
            req::log_configuration_initialized(wake_config);
        }
        return provision_history();
    }

    if (launch->init) {
        if (exists) {
            req::log_initialization_skipped(wake_config, "config/wake.ini");
        }
        else {
            req::log_configuration_missing(wake_config);
            const auto directory = prompt_directory(wake::defaults::directory);
            if (!directory) {
                return 1;
            }
            const auto logging = prompt_logging(true);
            if (!logging) {
                return 1;
            }
            if (!write_directory(*directory, *logging)) {
                return 1;
            }
            req::log_configuration_initialized(wake_config);
        }
        return provision_history();
    }

    if (!exists) {
        const auto directory = prompt_directory(wake::defaults::directory);
        const auto logging = prompt_logging(true);
        if (!directory || !logging || !write_directory(*directory, *logging)) {
            return 1;
        }
    }

    while (true) {
        std::cout << "\nwake_config\n";
        std::cout << "directory = " << current_directory() << "\n";
        std::cout << "logging = " << (current_logging() ? "on" : "off") << "\n";
        std::cout << "1. Set directory\n";
        std::cout << "2. Exit\n";
        std::cout << "Choice: ";
        std::string choice;
        if (!std::getline(std::cin, choice)) {
            return 0;
        }
        if (choice == "1") {
            if (edit_directory() != 0) {
                return 1;
            }
        }
        else if (choice == "2" || choice == "q" || choice == "Q") {
            return 0;
        }
        else {
            std::cout << "Enter 1 or 2.\n";
        }
    }
}
