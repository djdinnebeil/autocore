#include <Windows.h>

import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.ini;
import auto_core.core.paths;
import server_defaults;
import components_editor_request;
import config_menu;

import <iostream>;
import auto_core.core.shell;

namespace menu = ac::config_menu;

namespace {

ac::Component server_config {
    "server_config",
    ac::logging::config::LoggingScope {"server"}
};

constexpr std::string_view on_off[] {"on", "off"};

const menu::Setting menu_settings[] {
    {
        .key = "directory",
        .display_name = "Directory",
        .summary = "Folder that stores Server data.",
        .default_value = server::defaults::directory,
    },
    {
        .key = "logging",
        .display_name = "Logging",
        .summary = "Controls logging for the Server component.",
        .default_value = "on",
        .choices = on_off,
    },
};

std::string_view trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

struct Settings {
    std::string directory {server::defaults::directory};
    bool logging = true;
};

std::filesystem::path ini_path() {
    return ac::paths::config_directory() / "server.ini";
}

Settings load_settings() {
    Settings settings;
    const auto document = ac::ini::read(ini_path());
    if (!document) {
        return settings;
    }
    if (const auto value = document->find("server", "directory");
        value && !trim(*value).empty()) {
        settings.directory = std::string {trim(*value)};
    }
    if (const auto value = document->find("server", "logging")) {
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
    return settings;
}

bool write_server_ini(const Settings& settings) {
    std::error_code error;
    std::filesystem::create_directories(ac::paths::config_directory(), error);
    if (error) {
        server_config.log_print(
            "Failed to create config directory: {}",
            error.message()
        );
        return false;
    }

    const auto path = ini_path();
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        server_config.log_print("Failed to create {}", path.string());
        return false;
    }
    const auto contents = menu::with_header(
        menu_settings,
        server::defaults::ini_for(settings.directory, settings.logging)
    );
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    output.close();
    if (!output) {
        server_config.log_print("Failed to write {}", path.string());
        return false;
    }
    return true;
}

bool cancelled(const std::string_view value) {
    if (value != "cancel") {
        return false;
    }
    server_config.log_print("Cancelled.");
    return true;
}

enum class SiteChoice {
    yes,
    no,
    cancelled
};

SiteChoice prompt_default_site() {
    while (true) {
        std::cout << "Create default site files in the document root? [Y/n]: ";
        std::string input;
        if (!std::getline(std::cin, input)) {
            return SiteChoice::cancelled;
        }
        const auto value = trim(input);
        if (cancelled(value)) {
            return SiteChoice::cancelled;
        }
        if (value.empty() || value == "y" || value == "Y") {
            return SiteChoice::yes;
        }
        if (value == "n" || value == "N") {
            return SiteChoice::no;
        }
        std::cout << "Enter Y or n.\n";
    }
}

std::string current_text(const menu::Setting& setting) {
    const auto settings = load_settings();
    if (setting.key == "directory") {
        return settings.directory;
    }
    return settings.logging ? "on" : "off";
}

menu::ApplyResult apply_setting(
    const menu::Setting& setting,
    const std::string_view value
) {
    auto settings = load_settings();
    if (setting.key == "directory") {
        settings.directory = std::string {value};
    }
    else {
        settings.logging = value == "on";
    }
    if (!write_server_ini(settings)) {
        server_config.log_print("Failed to write config/server.ini.");
        return menu::ApplyResult::failed;
    }
    if (setting.key == "directory") {
        server_config.log_print("directory stored as {}.", settings.directory);
    }
    return menu::ApplyResult::stored;
}

int run_configuration_menu() {
    const auto result = menu::run_menu(
        "Server Configuration",
        menu_settings,
        current_text,
        apply_setting,
        std::cin,
        std::cout
    );
    return result.ok ? 0 : 1;
}

int offer_configuration() {
    const auto offer = menu::offer_configuration(
        "Server Configuration",
        menu_settings,
        current_text,
        std::cin,
        std::cout
    );
    if (offer == menu::OfferResult::failed) {
        return 1;
    }
    if (offer == menu::OfferResult::configure) {
        return run_configuration_menu();
    }
    return 0;
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
    const std::wstring_view executable_name,
    const std::wstring_view arguments
) {
    const auto executable_path = ac::paths::bin_directory() / executable_name;
    std::error_code error;
    const bool present = std::filesystem::exists(executable_path, error);
    if (error || !present) {
        server_config.log_print("Missing {}.", executable_path.string());
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
        server_config.log_print("Unable to start {}.", executable_path.string());
        return 1;
    }

    CloseHandle(process.hThread);
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exit_code = 1;
    if (!GetExitCodeProcess(process.hProcess, &exit_code)) {
        CloseHandle(process.hProcess);
        server_config.log_print(
            "Unable to read the exit code from {}.",
            executable_path.string()
        );
        return 1;
    }
    CloseHandle(process.hProcess);
    const int code = static_cast<int>(exit_code);
    if (code != 0) {
        server_config.log_print("{} exited {}.", executable_path.string(), code);
    }
    return code;
}

int launch_owned_seed() {
    if (const int editor = launch_owner(L"server_editor.exe", L"--seed");
        editor != 0) {
        return editor;
    }
    return launch_owner(L"server_builder.exe", L"--seed");
}

int launch_owned_init() {
    if (const int editor = launch_owner(L"server_editor.exe", L"--init");
        editor != 0) {
        return editor;
    }
    switch (prompt_default_site()) {
    case SiteChoice::yes:
        return launch_owner(L"server_builder.exe", L"--seed");
    case SiteChoice::no:
        server_config.log_print("Default site files were not requested.");
        return 0;
    case SiteChoice::cancelled:
        return 1;
    }
    return 1;
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
    server_config.log_main("server_config.exe started");

    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }

    const auto path = ini_path();
    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    if (error) {
        server_config.log_print("Failed to inspect {}", path.string());
        return 1;
    }

    namespace req = ac::config::components_request;
    req::log_config_request(server_config, *launch);
    if (launch->seed) {
        if (present) {
            req::log_seed_skipped(server_config, "config/server.ini");
        }
        else {
            req::log_writing_defaults(server_config);
            if (!write_server_ini({})) {
                server_config.log_print(
                    "Server configuration was not initialized."
                );
                return 1;
            }
            req::log_configuration_initialized(server_config);
        }
        return launch_owned_seed();
    }

    if (launch->init) {
        if (present) {
            req::log_initialization_skipped(server_config, "config/server.ini");
        }
        else {
            req::log_configuration_missing(server_config);
            if (!write_server_ini({})) {
                server_config.log_print(
                    "Server configuration was not initialized."
                );
                return 1;
            }
            if (offer_configuration() != 0) {
                server_config.log_print(
                    "Server configuration was not initialized."
                );
                return 1;
            }
            req::log_configuration_initialized(server_config);
        }
        return launch_owned_init();
    }

    if (!present) {
        if (!write_server_ini({})) {
            server_config.log_print(
                "Server configuration was not initialized."
            );
            return 1;
        }
    }

    activate_own_console();
    return run_configuration_menu();
}
