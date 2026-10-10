import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.ini;
import auto_core.core.paths;
import spotify_application_data;
import spotify_data_directory;
import spotify_defaults;
import components_editor_request;
import config_menu;

import <Windows.h>;
import <iostream>;
import auto_core.core.shell;

namespace menu = ac::config_menu;

namespace {

ac::Component spotify_config {
    "spotify_config",
    ac::logging::config::LoggingScope {"spotify"}
};

constexpr std::string_view on_off[] {"on", "off"};

const menu::Setting menu_settings[] {
    {
        .key = "directory",
        .display_name = "Directory",
        .summary = "Folder that stores Spotify data.",
        .default_value = spotify::defaults::directory,
    },
    {
        .key = "auto_launch_oauth",
        .display_name = "Auto-launch OAuth",
        .summary =
            "Launch Spotify OAuth when interactive reauthorization is required.",
        .default_value = "off",
        .choices = on_off,
    },
    {
        .key = "logging",
        .display_name = "Logging",
        .summary = "Controls logging for the Spotify component.",
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

bool write_bytes(
    const std::filesystem::path& path,
    std::string_view contents
) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) {
        spotify_config.log_print(
            "Failed to create config directory: {}",
            error.message()
        );
        return false;
    }
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        spotify_config.log_print("Failed to create {}", path.string());
        return false;
    }
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
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

std::optional<bool> prompt_yes_no(std::string_view question) {
    while (true) {
        std::cout << question;
        std::string line;
        if (!std::getline(std::cin, line)) {
            spotify_config.log_print("Failed to read Spotify configuration.");
            return std::nullopt;
        }
        if (const auto answer = spotify::defaults::accepted_yes_no(trim(line))) {
            return *answer;
        }
    }
}

std::optional<int> launch_process(
    std::wstring_view executable_name,
    std::wstring_view arguments,
    const DWORD creation_flags
) {
    const auto executable_path = ac::paths::bin_directory() / executable_name;
    std::error_code error;
    const bool present = std::filesystem::exists(executable_path, error);
    if (error || !present) {
        spotify_config.log_print("Missing {}.", executable_path.string());
        return std::nullopt;
    }

    std::wstring command_line = quote_argument(executable_path.wstring());
    if (!arguments.empty()) {
        command_line += L' ';
        command_line += quote_argument(arguments);
    }
    STARTUPINFOW startup_info {};
    startup_info.cb = sizeof(startup_info);
    PROCESS_INFORMATION process_info {};
    if (!::CreateProcessW(
            executable_path.c_str(),
            command_line.data(),
            nullptr,
            nullptr,
            FALSE,
            creation_flags,
            nullptr,
            executable_path.parent_path().c_str(),
            &startup_info,
            &process_info
        )) {
        spotify_config.log_print("Unable to start {}.", executable_path.string());
        return std::nullopt;
    }

    ::CloseHandle(process_info.hThread);
    ::WaitForSingleObject(process_info.hProcess, INFINITE);
    DWORD exit_code = 1;
    if (!::GetExitCodeProcess(process_info.hProcess, &exit_code)) {
        ::CloseHandle(process_info.hProcess);
        spotify_config.log_print(
            "Unable to read the exit code from {}.",
            executable_path.string()
        );
        return std::nullopt;
    }
    ::CloseHandle(process_info.hProcess);
    const int code = static_cast<int>(exit_code);
    if (code != 0) {
        spotify_config.log_print("{} exited {}.", executable_path.string(), code);
    }
    return code;
}

int launch_owner(
    std::wstring_view executable_name,
    std::wstring_view arguments
) {
    const auto code = launch_process(executable_name, arguments, 0);
    if (!code) {
        return 1;
    }
    return *code;
}

struct Settings {
    std::string directory {std::string {spotify::defaults::directory}};
    bool auto_launch = false;
    bool logging = true;
};

std::filesystem::path ini_path() {
    return ac::paths::config_directory() / "spotify.ini";
}

std::optional<Settings> load_settings(const bool report_failure) {
    const auto document = ac::ini::read(ini_path());
    if (!document) {
        if (report_failure) {
            spotify_config.log_print("Failed to read {}.", ini_path().string());
        }
        return std::nullopt;
    }
    Settings settings;
    if (const auto value = document->find("spotify", "directory");
        value && !value->empty()) {
        settings.directory = std::string {*value};
    }
    if (const auto value = document->find("spotify", "auto_launch_oauth")) {
        settings.auto_launch = spotify::defaults::auto_launch_enabled(*value);
    }
    const auto logging = document->find("spotify", "logging");
    if (!logging) {
        settings.logging = ac::logging::config::component_logging_default();
    }
    else if (*logging == "off") {
        settings.logging = false;
    }
    else if (*logging == "on") {
        settings.logging = true;
    }
    else {
        settings.logging = ac::logging::config::component_logging_default();
    }
    return settings;
}

bool write_settings(const Settings& settings) {
    return write_bytes(
        ini_path(),
        menu::with_header(
            menu_settings,
            spotify::defaults::ini_for(
                settings.directory,
                settings.auto_launch,
                settings.logging
            )
        )
    );
}

std::string current_text(const menu::Setting& setting) {
    const auto settings = load_settings(false);
    if (!settings) {
        if (setting.key == "directory") {
            return std::string {spotify::defaults::directory};
        }
        if (setting.key == "auto_launch_oauth") {
            return "off";
        }
        return "on";
    }
    if (setting.key == "directory") {
        return settings->directory;
    }
    if (setting.key == "auto_launch_oauth") {
        return settings->auto_launch ? "on" : "off";
    }
    return settings->logging ? "on" : "off";
}

menu::ApplyResult apply_setting(
    const menu::Setting& setting,
    const std::string_view value
) {
    auto settings = load_settings(true);
    if (!settings) {
        return menu::ApplyResult::failed;
    }
    if (setting.key == "directory") {
        settings->directory = std::string {value};
    }
    else if (setting.key == "auto_launch_oauth") {
        settings->auto_launch = value == "on";
    }
    else {
        settings->logging = value == "on";
    }
    if (!write_settings(*settings)) {
        return menu::ApplyResult::failed;
    }
    spotify_config.log_print("Wrote {}.", ini_path().string());
    return menu::ApplyResult::stored;
}

int run_configuration_menu() {
    const auto result = menu::run_menu(
        "Spotify Configuration",
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
        "Spotify Configuration",
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

std::optional<bool> file_present(const std::filesystem::path& path) {
    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    if (error) {
        spotify_config.log_print(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        );
        return std::nullopt;
    }
    return present;
}

void report_complete() {
    spotify_config.log_print("Spotify initialization complete.");
}

void report_incomplete() {
    spotify_config.log_print(
        "Spotify initialization incomplete: User Authorization has not been completed."
    );
}

int seed_owned_files() {
    if (const int database = launch_owner(L"spotify_db.exe", L"--seed");
        database != 0) {
        return database;
    }
    if (const int format = launch_owner(L"spotify_formatter.exe", L"--seed");
        format != 0) {
        return format;
    }
    return launch_owner(L"spotify_editor.exe", L"--seed");
}

int prompt_missing_stores() {
    const auto directory = spotify::data_directory();
    const auto history = file_present(directory / "history.db");
    if (!history) {
        return 1;
    }
    if (!*history) {
        const auto create = prompt_yes_no(
            "Create the listening database? [Y/n]: "
        );
        if (!create) {
            return 1;
        }
        if (*create) {
            if (const int database = launch_owner(L"spotify_db.exe", L"--seed");
                database != 0) {
                return database;
            }
        }
    }

    const auto format = file_present(directory / "song.format");
    if (!format) {
        return 1;
    }
    if (!*format) {
        return launch_owner(L"spotify_formatter.exe", L"--init");
    }
    return 0;
}

int finish_authorization() {
    if (!spotify::data::load_client_id()) {
        std::cout
            << "The Spotify Component requires Spotify User Authorization "
            << "to provide its functionality.\n";
        const auto configure = prompt_yes_no(
            "Configure the Spotify client ID now? [Y/n]: "
        );
        if (!configure) {
            return 1;
        }
        if (*configure) {
            if (const int editor = launch_owner(
                    L"spotify_editor.exe",
                    L"--client-id"
                );
                editor != 0) {
                return editor;
            }
        }
        if (!spotify::data::load_client_id()) {
            report_incomplete();
            return 0;
        }
    }

    const auto tokens = file_present(
        spotify::data_directory() / "tokens.map"
    );
    if (!tokens) {
        return 1;
    }
    if (*tokens) {
        report_complete();
        return 0;
    }

    const auto oauth = launch_process(
        L"spotify_oauth.exe",
        {},
        CREATE_NEW_CONSOLE
    );
    if (!oauth) {
        return 1;
    }
    if (*oauth == 0) {
        report_complete();
    }
    else {
        report_incomplete();
    }
    return 0;
}

int report_seeded_authorization() {
    const auto tokens = file_present(
        spotify::data_directory() / "tokens.map"
    );
    if (!tokens) {
        return 1;
    }
    if (!spotify::data::load_client_id() || !*tokens) {
        spotify_config.log_print("Spotify files have been seeded.");
        report_incomplete();
    }
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    spotify_config.log_main("spotify_config.exe started");

    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }

    const auto path = ini_path();
    std::error_code error;
    const bool exists = std::filesystem::exists(path, error);
    if (error) {
        spotify_config.log_print("Failed to inspect {}", path.string());
        return 1;
    }

    namespace req = ac::config::components_request;
    req::log_config_request(spotify_config, *launch);

    if (!launch->init && !launch->seed) {
        if (!exists) {
            if (!write_settings({})) {
                return 1;
            }
            spotify_config.log_print("Wrote {}.", path.string());
        }
        else if (!load_settings(true)) {
            return 1;
        }
        return run_configuration_menu();
    }

    if (launch->seed) {
        if (exists) {
            req::log_seed_skipped(spotify_config, "config/spotify.ini");
        }
        else {
            req::log_writing_defaults(spotify_config);
            if (!write_settings({})) {
                return 1;
            }
            req::log_configuration_initialized(spotify_config);
        }
        if (const int seeded = seed_owned_files(); seeded != 0) {
            return seeded;
        }
    }
    else if (exists) {
        req::log_initialization_skipped(spotify_config, "config/spotify.ini");
    }
    else {
        req::log_configuration_missing(spotify_config);
        if (!write_settings({})) {
            return 1;
        }
        spotify_config.log_print("Wrote {}.", path.string());
        if (offer_configuration() != 0) {
            return 1;
        }
    }

    if (!launch->seed) {
        if (const int stores = prompt_missing_stores(); stores != 0) {
            return stores;
        }
    }

    if (!launch->init) {
        return report_seeded_authorization();
    }
    return finish_authorization();
}
