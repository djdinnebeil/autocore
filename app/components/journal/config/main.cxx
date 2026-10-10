import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.ini;
import auto_core.core.paths;
import journal_auto_select;
import journal_defaults;
import journal_remote_sync;
import components_editor_request;
import config_menu;

import <iostream>;
import <Windows.h>;
import auto_core.core.shell;

namespace menu = ac::config_menu;

namespace {

ac::Component journal_config {
    "journal_config",
    ac::logging::config::LoggingScope {"journal"}
};

constexpr std::string_view on_off[] {"on", "off"};

const menu::Setting menu_settings[] {
    {
        .key = "directory",
        .display_name = "Directory",
        .summary = "Folder that stores Journal data.",
        .default_value = journal::defaults::directory,
    },
    {
        .key = "auto_select_new_series",
        .display_name = "Auto-select new series",
        .summary = "On makes a newly created series the active series.",
        .default_value = "on",
        .choices = on_off,
    },
    {
        .key = "remote_sync",
        .display_name = "Remote sync",
        .summary = "On starts the Journal cloud runtime.",
        .default_value = "off",
        .choices = on_off,
    },
    {
        .key = "logging",
        .display_name = "Logging",
        .summary = "Controls logging for the Journal component.",
        .default_value = "on",
        .choices = on_off,
    },
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

std::filesystem::path ini_path() {
    return ac::paths::config_directory() / "journal.ini";
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
    const auto document = ac::ini::read(ini_path());
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
        if (*value == "on") {
            settings.logging = true;
        }
        else if (*value == "off") {
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
bool write_settings(const JournalSettings& settings, const bool announce) {
    const auto path = ini_path();
    std::error_code create_error;
    std::filesystem::create_directories(path.parent_path(), create_error);
    if (create_error) {
        journal_config.log_print(
            "Failed to create config directory: {}",
            create_error.message()
        );
        return false;
    }
    const auto contents = menu::with_header(
        menu_settings,
        journal::defaults::ini_for(
            settings.directory,
            settings.logging,
            settings.auto_select_new_series,
            settings.remote_sync
        )
    );
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
    if (announce) {
        journal_config.log_print("Wrote {}", path.string());
    }
    return true;
}

[[nodiscard]]
std::string current_text(const menu::Setting& setting) {
    const auto settings = load_settings();
    if (setting.key == "directory") {
        return settings.directory;
    }
    if (setting.key == "auto_select_new_series") {
        return settings.auto_select_new_series ? "on" : "off";
    }
    if (setting.key == "remote_sync") {
        return settings.remote_sync;
    }
    return settings.logging ? "on" : "off";
}

[[nodiscard]]
menu::ApplyResult apply_setting(
    const menu::Setting& setting,
    const std::string_view value
) {
    auto settings = load_settings();
    if (setting.key == "directory") {
        settings.directory = std::string {value};
    }
    else if (setting.key == "auto_select_new_series") {
        settings.auto_select_new_series = value == "on";
    }
    else if (setting.key == "remote_sync") {
        settings.remote_sync = std::string {value};
    }
    else {
        settings.logging = value == "on";
    }
    if (!write_settings(settings, true)) {
        return menu::ApplyResult::failed;
    }
    return menu::ApplyResult::stored;
}

[[nodiscard]]
int run_configuration_menu() {
    const auto result = menu::run_menu(
        "Journal Configuration",
        menu_settings,
        current_text,
        apply_setting,
        std::cin,
        std::cout
    );
    return result.ok ? 0 : 1;
}

[[nodiscard]]
int offer_configuration() {
    const auto offer = menu::offer_configuration(
        "Journal Configuration",
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

    const auto path = ini_path();
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
            if (!write_settings({}, false)) {
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
            if (!write_settings({}, false)) {
                journal_config.log_print(
                    "Journal configuration was not initialized."
                );
                return 1;
            }
            if (offer_configuration() != 0) {
                journal_config.log_print(
                    "Journal configuration was not initialized."
                );
                return 1;
            }
        }
        return launch_owned_stores(L"--init");
    }

    if (!present) {
        if (!write_settings({}, false)) {
            journal_config.log_print(
                "Journal configuration was not initialized."
            );
            return 1;
        }
    }

    if (launch_owner(L"journal_builder.exe", L"--seed") != 0) {
        return 1;
    }

    activate_own_console();
    return run_configuration_menu();
}
