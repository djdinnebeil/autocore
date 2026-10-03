import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.ini;
import auto_core.core.paths;
import journal_auto_select;
import journal_defaults;
import journal_remote_sync;
import components_editor_request;

import <iostream>;
import <Windows.h>;
import auto_core.core.shell;

namespace {

ac::Component journal_config {
    "journal_config",
    ac::logging::config::LoggingScope {"journal"}
};

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
        journal_config.log_print("Missing {}.", executable_path.string());
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
        journal_config.log_print("Unable to start {}.", executable_path.string());
        return 1;
    }

    CloseHandle(process.hThread);
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exit_code = 1;
    if (!GetExitCodeProcess(process.hProcess, &exit_code)) {
        CloseHandle(process.hProcess);
        journal_config.log_print(
            "Unable to read the exit code from {}.",
            executable_path.string()
        );
        return 1;
    }
    CloseHandle(process.hProcess);
    const int code = static_cast<int>(exit_code);
    if (code != 0) {
        journal_config.log_print("{} exited {}.", executable_path.string(), code);
    }
    return code;
}

int launch_owned_stores(const std::wstring_view mode) {
    if (const int builder = launch_owner(L"journal_builder.exe", mode);
        builder != 0) {
        return builder;
    }
    if (const int cloud = launch_owner(L"journal_cloud.exe", mode);
        cloud != 0) {
        return cloud;
    }
    if (const int clock = launch_owner(L"journal_clock.exe", mode);
        clock != 0) {
        return clock;
    }
    if (const int database = launch_owner(L"journal_db.exe", mode);
        database != 0) {
        return database;
    }
    return launch_owner(L"journal_series.exe", mode);
}

std::string_view trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

[[nodiscard]]
std::optional<std::string> prompt_line(
    const std::string_view label,
    const std::string_view current
) {
    std::cout << label << " [" << current << "]: ";
    std::string input;
    if (!std::getline(std::cin, input)) {
        return std::nullopt;
    }
    const auto value = trim(input);
    if (value.empty()) {
        return std::string {current};
    }
    return std::string {value};
}

[[nodiscard]]
bool cancelled(const std::string_view value) {
    if (value != "cancel") {
        return false;
    }
    journal_config.log_print("Cancelled.");
    return true;
}

[[nodiscard]]
std::optional<std::string> prompt_on_off(
    const std::string_view label,
    const std::string_view current
) {
    while (true) {
        std::cout << label << " [" << current << "]: ";
        std::string input;
        if (!std::getline(std::cin, input)) {
            return std::nullopt;
        }
        const auto value = trim(input);
        if (cancelled(value)) {
            return std::nullopt;
        }
        if (value.empty()) {
            return std::string {current};
        }
        if (const auto token = journal::remote_sync::canonical(value)) {
            return *token;
        }
        std::cout << "Enter on or off.\n";
    }
}

[[nodiscard]]
std::optional<std::string> prompt_remote_sync(const std::string_view current) {
    return prompt_on_off("remote_sync", current);
}

bool ensure_journal_ini(const bool prompt) {
    const auto path = ac::paths::config_directory() / "journal.ini";

    std::error_code error;
    if (std::filesystem::exists(path, error)) {
        return true;
    }
    if (error) {
        journal_config.log_print(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        );
        return false;
    }

    std::string directory;
    bool auto_select_new_series = true;
    std::string remote_sync {"off"};
    bool logging = true;
    if (prompt) {
        std::cout
            << "Journal directory ["
            << journal::defaults::directory
            << "]: ";
        std::string input;
        if (!std::getline(std::cin, input)) {
            return false;
        }
        const auto directory_value = trim(input);
        if (cancelled(directory_value)) {
            return false;
        }
        directory = std::string {directory_value};
        const auto select = prompt_on_off("Auto-select new series", "on");
        if (!select) {
            return false;
        }
        auto_select_new_series = *select == "on";
        const auto sync = prompt_on_off("Remote sync", remote_sync);
        if (!sync) {
            return false;
        }
        remote_sync = *sync;
        const auto logging_value = prompt_on_off("Enable logging", "on");
        if (!logging_value) {
            return false;
        }
        logging = *logging_value == "on";
    }

    std::error_code create_error;
    std::filesystem::create_directories(ac::paths::config_directory(), create_error);
    if (create_error) {
        journal_config.log_print(
            "Failed to create config directory: {}",
            create_error.message()
        );
        return false;
    }
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        journal_config.log_print("Failed to create {}", path.string());
        return false;
    }
    const auto contents = journal::defaults::ini_for(
        directory,
        logging,
        auto_select_new_series,
        remote_sync
    );
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    output.close();
    return static_cast<bool>(output);
}

struct JournalSettings {
    std::string directory {journal::defaults::directory};
    bool auto_select_new_series = true;
    std::string remote_sync {"off"};
    bool logging = true;
};

[[nodiscard]]
JournalSettings load_settings() {
    JournalSettings settings;
    const auto document = ac::ini::read(
        ac::paths::config_directory() / "journal.ini"
    );
    if (!document) {
        return settings;
    }
    if (const auto value = document->find("journal", "directory");
        value && !value->empty()) {
        settings.directory = std::string {*value};
    }
    settings.auto_select_new_series = journal::auto_select::enabled(
        document->find("journal", "auto_select_new_series")
    );
    if (const auto value = document->find("journal", "remote_sync")) {
        if (const auto token = journal::remote_sync::canonical(*value)) {
            settings.remote_sync = *token;
        }
    }
    if (const auto value = document->find("journal", "logging")) {
        if (*value == "on" || *value == "true") {
            settings.logging = true;
        }
        else if (*value == "off" || *value == "false") {
            settings.logging = false;
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

[[nodiscard]]
bool save_settings(const JournalSettings& settings) {
    const auto path = ac::paths::config_directory() / "journal.ini";
    const std::string contents =
        "[journal]\n"
        "directory = " + settings.directory + "\n"
        "auto_select_new_series = " +
        std::string {settings.auto_select_new_series ? "on" : "off"} + "\n"
        "remote_sync = " + settings.remote_sync + "\n"
        "logging = " + std::string {settings.logging ? "on" : "off"} + "\n";
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        journal_config.log_print("Failed to write {}", path.string());
        return false;
    }
    output.write(
        contents.data(),
        static_cast<std::streamsize>(contents.size())
    );
    output.close();
    if (!output) {
        journal_config.log_print("Failed to write {}", path.string());
        return false;
    }
    journal_config.log_print("Wrote {}", path.string());
    return true;
}

[[nodiscard]]
bool edit_settings() {
    auto settings = load_settings();
    const auto directory = prompt_line("directory", settings.directory);
    if (!directory) {
        return false;
    }
    const auto remote_sync = prompt_remote_sync(settings.remote_sync);
    if (!remote_sync) {
        return false;
    }
    settings.directory = *directory;
    settings.remote_sync = *remote_sync;
    while (true) {
        std::cout << "Select a new series automatically? ["
                  << (settings.auto_select_new_series ? "Y/n" : "y/N")
                  << "]: ";
        std::string answer;
        if (!std::getline(std::cin, answer)) {
            return false;
        }
        const auto value = trim(answer);
        if (value.empty()) {
            break;
        }
        if (value == "y" || value == "Y") {
            settings.auto_select_new_series = true;
            break;
        }
        if (value == "n" || value == "N") {
            settings.auto_select_new_series = false;
            break;
        }
        std::cout << "Enter Y or n.\n";
    }
    while (true) {
        std::cout << "Enable logging? ["
                  << (settings.logging ? "Y/n" : "y/N")
                  << "]: ";
        std::string answer;
        if (!std::getline(std::cin, answer)) {
            return false;
        }
        const auto value = trim(answer);
        if (value.empty()) {
            break;
        }
        if (value == "y" || value == "Y") {
            settings.logging = true;
            break;
        }
        if (value == "n" || value == "N") {
            settings.logging = false;
            break;
        }
        std::cout << "Enter Y or n.\n";
    }
    return save_settings(settings);
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
    journal_config.log_main("journal_config.exe started");

    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }

    const auto path = ac::paths::config_directory() / "journal.ini";
    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    if (error) {
        journal_config.log_print("Failed to inspect {}", path.string());
        return 1;
    }

    namespace req = ac::config::components_request;
    req::log_config_request(journal_config, *launch);
    if (launch->seed) {
        if (present) {
            req::log_seed_skipped(journal_config, "config/journal.ini");
        }
        else {
            req::log_writing_defaults(journal_config);
            if (!ensure_journal_ini(false)) {
                journal_config.log_print(
                    "Journal configuration was not initialized."
                );
                return 1;
            }
            req::log_configuration_initialized(journal_config);
        }
        return launch_owned_stores(L"--seed");
    }

    if (launch->init) {
        if (present) {
            req::log_initialization_skipped(journal_config, "config/journal.ini");
        }
        else {
            req::log_configuration_missing(journal_config);
            if (!ensure_journal_ini(true)) {
                journal_config.log_print(
                    "Journal configuration was not initialized."
                );
                return 1;
            }
        }
        return launch_owned_stores(L"--init");
    }

    if (!ensure_journal_ini(true)) {
        journal_config.log_print("Journal configuration was not initialized.");
        return 1;
    }

    if (launch_owner(L"journal_builder.exe", L"--seed") != 0) {
        return 1;
    }

    activate_own_console();

    while (true) {
        std::cout
            << "\njournal_config\n"
            << "  1. Journal settings\n"
            << "  2. Exit\n"
            << "> ";
        std::string choice;
        if (!std::getline(std::cin, choice)) {
            return 0;
        }
        if (choice == "1") {
            if (!edit_settings()) {
                return 1;
            }
        }
        else if (choice == "2" || choice == "q" || choice == "Q") {
            return 0;
        }
        else {
            std::cout << "Unknown option.\n";
        }
    }
}
