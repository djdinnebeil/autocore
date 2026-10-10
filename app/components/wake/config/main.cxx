#include <Windows.h>

#include "../shared/wake_history_detail.hpp"

import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.ini;
import auto_core.core.paths;
import wake_data_directory;
import components_editor_request;
import config_menu;
import wake_defaults;

import <iostream>;
import auto_core.core.shell;

namespace menu = ac::config_menu;

namespace {

constexpr std::string_view on_off[] {"on", "off"};

const menu::Setting menu_settings[] {
    {
        .key = "directory",
        .display_name = "Directory",
        .summary =
            "Wake data directory. A relative path resolves against the "
            "installation root.",
        .default_value = wake::defaults::directory,
        .constraint = "Enter a relative or absolute directory.",
    },
    {
        .key = "logging",
        .display_name = "Logging",
        .summary = "Controls logging for the Wake component. History files stay independent of this switch.",
        .default_value = "on",
        .choices = on_off,
    },
};

ac::Component wake_config {
    "wake_config",
    ac::logging::config::LoggingScope {"wake"}
};

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
    if (*value == "on") {
        return true;
    }
    if (*value == "off") {
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

    const auto contents = menu::with_header(
        menu_settings,
        wake::defaults::ini_for(directory, logging)
    );
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
    const auto directory = wake::data_directory();
    if (wake::history::all_history_files_present(directory)) {
        wake_config.log_print("Wake history is already present.");
        return 0;
    }
    wake_config.log_print("Launching wake_ac.exe --snapshot.");
    return launch_snapshot();
}

[[nodiscard]]
std::string current_text(const menu::Setting& setting) {
    if (setting.key == "directory") {
        return current_directory();
    }
    return current_logging() ? "on" : "off";
}

[[nodiscard]]
menu::ApplyResult apply_setting(
    const menu::Setting& setting,
    const std::string_view value
) {
    const auto directory = setting.key == "directory"
        ? std::string {value}
        : current_directory();
    const bool logging = setting.key == "logging"
        ? value == "on"
        : current_logging();
    if (!write_directory(directory, logging)) {
        return menu::ApplyResult::failed;
    }
    return menu::ApplyResult::stored;
}

[[nodiscard]]
int run_configuration_menu() {
    const auto result = menu::run_menu(
        "Wake Configuration",
        menu_settings,
        current_text,
        apply_setting,
        std::cin,
        std::cout
    );
    return result.ok ? 0 : 1;
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
            req::log_writing_defaults(wake_config);
            if (!write_directory(wake::defaults::directory, true)) {
                return 1;
            }
            const auto offer = menu::offer_configuration(
                "Wake Configuration",
                menu_settings,
                current_text,
                std::cin,
                std::cout
            );
            if (offer == menu::OfferResult::failed) {
                return 1;
            }
            if (offer == menu::OfferResult::configure &&
                run_configuration_menu() != 0) {
                return 1;
            }
            req::log_configuration_initialized(wake_config);
        }
        return provision_history();
    }

    if (!exists) {
        req::log_configuration_missing(wake_config);
        req::log_writing_defaults(wake_config);
        if (!write_directory(wake::defaults::directory, true)) {
            return 1;
        }
    }
    return run_configuration_menu();
}
