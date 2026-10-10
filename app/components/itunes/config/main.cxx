import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.ini;
import auto_core.core.paths;
import components_editor_request;
import config_menu;
import itunes_defaults;

import <iostream>;
import <Windows.h>;
import auto_core.core.shell;

namespace menu = ac::config_menu;

namespace {

constexpr std::string_view on_off[] {"on", "off"};

const menu::Setting menu_settings[] {
    {
        .key = "directory",
        .display_name = "Directory",
        .summary =
            "iTunes data directory. A relative path resolves against the "
            "installation root.",
        .default_value = itunes::defaults::directory,
        .constraint = "Enter a relative or absolute directory.",
    },
    {
        .key = "auto_start",
        .display_name = "Auto-start",
        .summary = "Start iTunes when the iTunes component starts.",
        .default_value = "on",
        .choices = on_off,
    },
    {
        .key = "logging",
        .display_name = "Logging",
        .summary = "Controls logging for the iTunes component.",
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
        if (*logging == "off") {
            values.logging = false;
        }
        else if (*logging == "on") {
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

bool write_prompted(
    ac::Component& log,
    const std::filesystem::path& path,
    const PromptValues& values
) {
    const auto contents = menu::with_header(
        menu_settings,
        itunes::defaults::ini_for(
            values.directory,
            values.auto_start,
            values.logging
        )
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

std::string current_text(
    ac::Component& log,
    const std::filesystem::path& path,
    const menu::Setting& setting
) {
    bool readable = false;
    const auto values = values_in_file(log, path, readable);
    if (setting.key == "directory") {
        return values.directory;
    }
    if (setting.key == "auto_start") {
        return values.auto_start ? "on" : "off";
    }
    return values.logging ? "on" : "off";
}

menu::ApplyResult apply_setting(
    ac::Component& log,
    const std::filesystem::path& path,
    const menu::Setting& setting,
    const std::string_view value
) {
    bool readable = false;
    auto values = values_in_file(log, path, readable);
    if (!readable && std::filesystem::exists(path)) {
        return menu::ApplyResult::failed;
    }
    if (setting.key == "directory") {
        values.directory = std::string {value};
    }
    else if (setting.key == "auto_start") {
        values.auto_start = value == "on";
    }
    else {
        values.logging = value == "on";
    }
    if (!write_prompted(log, path, values)) {
        return menu::ApplyResult::failed;
    }
    return menu::ApplyResult::stored;
}

int run_configuration_menu(
    ac::Component& log,
    const std::filesystem::path& path
) {
    const auto result = menu::run_menu(
        "iTunes Configuration",
        menu_settings,
        [&](const menu::Setting& setting) {
            return current_text(log, path, setting);
        },
        [&](const menu::Setting& setting, const std::string_view value) {
            return apply_setting(log, path, setting, value);
        },
        std::cin,
        std::cout
    );
    return result.ok ? 0 : 1;
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
            if (!write_bytes(
                    itunes_config,
                    path,
                    menu::with_header(menu_settings, itunes::defaults::ini_text)
                )) {
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
            req::log_writing_defaults(itunes_config);
            if (!write_prompted(itunes_config, path, {})) {
                return 1;
            }
            const auto offer = menu::offer_configuration(
                "iTunes Configuration",
                menu_settings,
                [&](const menu::Setting& setting) {
                    return current_text(itunes_config, path, setting);
                },
                std::cin,
                std::cout
            );
            if (offer == menu::OfferResult::failed) {
                return 1;
            }
            if (offer == menu::OfferResult::configure &&
                run_configuration_menu(itunes_config, path) != 0) {
                return 1;
            }
        }
        return launch_owned_stores(itunes_config, L"--init");
    }

    if (!exists) {
        req::log_configuration_missing(itunes_config);
        req::log_writing_defaults(itunes_config);
        if (!write_prompted(itunes_config, path, {})) {
            return 1;
        }
    }
    return run_configuration_menu(itunes_config, path);
}
