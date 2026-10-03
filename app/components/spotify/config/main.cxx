import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.ini;
import auto_core.core.paths;
import spotify_application_data;
import spotify_defaults;
import components_editor_request;

import <Windows.h>;
import <iostream>;
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

std::optional<bool> prompt_yes_no(
    ac::Component& log,
    std::string_view question
) {
    while (true) {
        std::cout << question;
        std::string line;
        if (!std::getline(std::cin, line)) {
            log.log_print("Failed to read Spotify configuration.");
            return std::nullopt;
        }
        if (const auto answer = spotify::defaults::accepted_yes_no(trim(line))) {
            return *answer;
        }
    }
}

std::optional<bool> prompt_auto_launch(ac::Component& log) {
    while (true) {
        std::cout
            << "Automatically launch Spotify OAuth when authorization is required? [y/N]: ";
        std::string line;
        if (!std::getline(std::cin, line)) {
            log.log_print("Failed to read Spotify configuration.");
            return std::nullopt;
        }
        const auto value = trim(line);
        if (value.empty() || value == "n" || value == "N") {
            return false;
        }
        if (value == "y" || value == "Y") {
            return true;
        }
    }
}

std::optional<int> launch_process(
    ac::Component& log,
    std::wstring_view executable_name,
    std::wstring_view arguments,
    const DWORD creation_flags
) {
    const auto executable_path = ac::paths::bin_directory() / executable_name;
    std::error_code error;
    const bool present = std::filesystem::exists(executable_path, error);
    if (error || !present) {
        log.log_print("Missing {}.", executable_path.string());
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
        log.log_print("Unable to start {}.", executable_path.string());
        return std::nullopt;
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
        return std::nullopt;
    }
    ::CloseHandle(process_info.hProcess);
    const int code = static_cast<int>(exit_code);
    if (code != 0) {
        log.log_print("{} exited {}.", executable_path.string(), code);
    }
    return code;
}

int launch_owner(
    ac::Component& log,
    std::wstring_view executable_name,
    std::wstring_view arguments
) {
    const auto code = launch_process(log, executable_name, arguments, 0);
    if (!code) {
        return 1;
    }
    return *code;
}

std::optional<std::string> stored_directory(
    ac::Component& log,
    const std::filesystem::path& path
) {
    const auto document = ac::ini::read(path);
    if (!document) {
        log.log_print("Failed to read {}.", path.string());
        return std::nullopt;
    }
    const auto value = document->find("spotify", "directory");
    if (!value || value->empty()) {
        return std::string {spotify::defaults::directory};
    }
    return std::string {*value};
}

bool stored_logging(const std::filesystem::path& path) {
    const auto document = ac::ini::read(path);
    if (!document) {
        return ac::logging::config::component_logging_default();
    }
    const auto value = document->find("spotify", "logging");
    if (!value) {
        return ac::logging::config::component_logging_default();
    }
    if (*value == "off" || *value == "false") {
        return false;
    }
    if (*value == "on" || *value == "true") {
        return true;
    }
    return ac::logging::config::component_logging_default();
}

std::optional<bool> prompt_logging(ac::Component& log, const bool current) {
    while (true) {
        std::cout << "Enable logging? ["
                  << (current ? "Y/n" : "y/N")
                  << "]: ";
        std::string input;
        if (!std::getline(std::cin, input)) {
            log.log_print("Failed to read logging.");
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

std::optional<bool> file_present(
    ac::Component& log,
    const std::filesystem::path& path
) {
    std::error_code error;
    const bool present = std::filesystem::exists(path, error);
    if (error) {
        log.log_print(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        );
        return std::nullopt;
    }
    return present;
}

void report_complete(ac::Component& log) {
    log.log_print("Spotify initialization complete.");
}

void report_incomplete(ac::Component& log) {
    log.log_print(
        "Spotify initialization incomplete: User Authorization has not been completed."
    );
}

int seed_owned_files(ac::Component& log) {
    if (const int database = launch_owner(log, L"spotify_db.exe", L"--seed");
        database != 0) {
        return database;
    }
    if (const int format = launch_owner(log, L"spotify_formatter.exe", L"--seed");
        format != 0) {
        return format;
    }
    return launch_owner(log, L"spotify_editor.exe", L"--seed");
}

int prompt_missing_stores(ac::Component& log) {
    const auto directory = ac::paths::spotify_directory();
    const auto history = file_present(log, directory / "history.db");
    if (!history) {
        return 1;
    }
    if (!*history) {
        const auto create = prompt_yes_no(
            log,
            "Create the listening database? [Y/n]: "
        );
        if (!create) {
            return 1;
        }
        if (*create) {
            if (const int database = launch_owner(log, L"spotify_db.exe", L"--seed");
                database != 0) {
                return database;
            }
        }
    }

    const auto format = file_present(log, directory / "song.format");
    if (!format) {
        return 1;
    }
    if (!*format) {
        return launch_owner(log, L"spotify_formatter.exe", L"--init");
    }
    return 0;
}

int finish_authorization(ac::Component& log) {
    if (!spotify::data::load_client_id()) {
        std::cout
            << "The Spotify Component requires Spotify User Authorization "
            << "to provide its functionality.\n";
        const auto configure = prompt_yes_no(
            log,
            "Configure the Spotify client ID now? [Y/n]: "
        );
        if (!configure) {
            return 1;
        }
        if (*configure) {
            if (const int editor = launch_owner(
                    log,
                    L"spotify_editor.exe",
                    L"--client-id"
                );
                editor != 0) {
                return editor;
            }
        }
        if (!spotify::data::load_client_id()) {
            report_incomplete(log);
            return 0;
        }
    }

    const auto tokens = file_present(
        log,
        ac::paths::spotify_directory() / "tokens.map"
    );
    if (!tokens) {
        return 1;
    }
    if (*tokens) {
        report_complete(log);
        return 0;
    }

    const auto oauth = launch_process(
        log,
        L"spotify_oauth.exe",
        {},
        CREATE_NEW_CONSOLE
    );
    if (!oauth) {
        return 1;
    }
    if (*oauth == 0) {
        report_complete(log);
    }
    else {
        report_incomplete(log);
    }
    return 0;
}

int report_seeded_authorization(ac::Component& log) {
    const auto tokens = file_present(
        log,
        ac::paths::spotify_directory() / "tokens.map"
    );
    if (!tokens) {
        return 1;
    }
    if (!spotify::data::load_client_id() || !*tokens) {
        log.log_print("Spotify files have been seeded.");
        report_incomplete(log);
    }
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    ac::Component spotify_config {
        "spotify_config",
        ac::logging::config::LoggingScope {"spotify"}
    };
    spotify_config.log_main("spotify_config.exe started");

    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }

    const auto path = ac::paths::config_directory() / "spotify.ini";
    std::error_code error;
    const bool exists = std::filesystem::exists(path, error);
    if (error) {
        spotify_config.log_print("Failed to inspect {}", path.string());
        return 1;
    }

    namespace req = ac::config::components_request;
    req::log_config_request(spotify_config, *launch);

    if (!launch->init && !launch->seed) {
        std::string directory {spotify::defaults::directory};
        if (exists) {
            const auto current = stored_directory(spotify_config, path);
            if (!current) {
                return 1;
            }
            directory = *current;
        }
        else {
            std::cout << "Spotify directory ["
                      << spotify::defaults::directory << "]: ";
            std::string input;
            if (!std::getline(std::cin, input)) {
                spotify_config.log_print("Failed to read Spotify directory.");
                return 1;
            }
            const auto chosen = trim(input);
            if (!chosen.empty()) {
                directory = std::string {chosen};
            }
        }

        const auto auto_launch = prompt_auto_launch(spotify_config);
        if (!auto_launch) {
            return 1;
        }
        const auto logging = prompt_logging(
            spotify_config,
            exists ? stored_logging(path) : true
        );
        if (!logging) {
            return 1;
        }
        if (!write_bytes(
                spotify_config,
                path,
                spotify::defaults::ini_for(directory, *auto_launch, *logging)
            )) {
            return 1;
        }
        spotify_config.log_print("Wrote {}.", path.string());
        return 0;
    }

    if (launch->seed) {
        if (exists) {
            req::log_seed_skipped(spotify_config, "config/spotify.ini");
        }
        else {
            req::log_writing_defaults(spotify_config);
            if (!write_bytes(spotify_config, path, spotify::defaults::ini_text)) {
                return 1;
            }
            req::log_configuration_initialized(spotify_config);
        }
        if (const int seeded = seed_owned_files(spotify_config); seeded != 0) {
            return seeded;
        }
    }
    else if (exists) {
        req::log_initialization_skipped(spotify_config, "config/spotify.ini");
    }
    else {
        req::log_configuration_missing(spotify_config);
        std::cout << "Spotify directory ["
                  << spotify::defaults::directory << "]: ";
        std::string input;
        if (!std::getline(std::cin, input)) {
            spotify_config.log_print("Failed to read Spotify directory.");
            return 1;
        }
        std::string directory {spotify::defaults::directory};
        const auto chosen = trim(input);
        if (!chosen.empty()) {
            directory = std::string {chosen};
        }
        const auto auto_launch = prompt_auto_launch(spotify_config);
        if (!auto_launch) {
            return 1;
        }
        const auto logging = prompt_logging(spotify_config, true);
        if (!logging) {
            return 1;
        }
        if (!write_bytes(
                spotify_config,
                path,
                spotify::defaults::ini_for(directory, *auto_launch, *logging)
            )) {
            return 1;
        }
        spotify_config.log_print("Wrote {}.", path.string());
    }

    if (!launch->seed) {
        if (const int stores = prompt_missing_stores(spotify_config); stores != 0) {
            return stores;
        }
    }

    if (!launch->init) {
        return report_seeded_authorization(spotify_config);
    }
    return finish_authorization(spotify_config);
}
