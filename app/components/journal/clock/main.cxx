import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.paths;
import journal_extended_hours;
import components_editor_request;

import <iostream>;
import <Windows.h>;
import auto_core.core.shell;

namespace {

ac::Component journal_clock {
    "journal_clock",
    ac::logging::config::LoggingScope {"journal"}
};

constexpr journal::extended_hours::ExtendedHours recommended_hours {0, true};

std::string_view trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

std::wstring quote_argument(std::wstring_view value) {
    std::wstring quoted;
    quoted.reserve(value.size() + 2);
    quoted.push_back(L'"');
    quoted.append(value);
    quoted.push_back(L'"');
    return quoted;
}

bool journal_ini_present(bool& present) {
    const auto path = ac::paths::config_directory() / "journal.ini";
    std::error_code error;
    present = std::filesystem::exists(path, error);
    if (error) {
        journal_clock.log_print(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        );
        return false;
    }
    return true;
}

int run_journal_config(
    const ac::config::components_request::ConfigLaunch& launch
) {
    const std::filesystem::path executable =
        ac::paths::bin_directory() / "journal_config.exe";
    std::wstring arguments = launch.seed ? L"--seed" : L"--init";
    if (launch.seed && launch.init) {
        arguments += L" --init";
    }
    std::wstring command =
        quote_argument(executable.wstring()) + L" " + arguments;

    STARTUPINFOW startup {};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process {};
    if (!CreateProcessW(
            executable.c_str(),
            command.data(),
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            executable.parent_path().c_str(),
            &startup,
            &process
        )) {
        journal_clock.log_print("Unable to start journal_config.exe.");
        return 1;
    }

    CloseHandle(process.hThread);
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exit_code = 1;
    if (!GetExitCodeProcess(process.hProcess, &exit_code)) {
        CloseHandle(process.hProcess);
        journal_clock.log_print(
            "Unable to read the exit code from journal_config.exe."
        );
        return 1;
    }
    CloseHandle(process.hProcess);
    const int code = static_cast<int>(exit_code);
    if (code != 0) {
        journal_clock.log_print("journal_config.exe exited {}.", code);
    }
    return code;
}

bool file_present(bool& present) {
    const auto path = journal::extended_hours::file_path();
    std::error_code error;
    present = std::filesystem::exists(path, error);
    if (error) {
        journal_clock.log_print(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        );
        return false;
    }
    return true;
}

bool write_clock_file(const journal::extended_hours::ExtendedHours& hours) {
    const auto path = journal::extended_hours::file_path();
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) {
        journal_clock.log_print(
            "Failed to create {}: {}",
            path.parent_path().string(),
            error.message()
        );
        return false;
    }

    const auto contents = journal::extended_hours::file_text(hours);
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        journal_clock.log_print("Failed to write {}", path.string());
        return false;
    }
    output.write(
        contents.data(),
        static_cast<std::streamsize>(contents.size())
    );
    output.close();
    if (!output) {
        journal_clock.log_print("Failed to write {}", path.string());
        return false;
    }
    journal_clock.log_print("Wrote {}", path.string());
    return true;
}

void print_range() {
    std::cout << journal::extended_hours::range_description();
    std::cout << "\nEnter 0, +0, 1, +1 ... 12, or +12.\n";
}

std::optional<journal::extended_hours::ExtendedHours> prompt_token(
    const std::optional<std::string>& current
) {
    while (true) {
        print_range();
        const std::string recommended =
            journal::extended_hours::canonical_token(recommended_hours);
        std::cout << (current ? "extended_hours" : "Extended hours");
        std::cout << " [" << (current ? *current : recommended) << "]: ";

        std::string input;
        if (!std::getline(std::cin, input)) {
            return std::nullopt;
        }
        const auto value = trim(input);
        if (value == "cancel") {
            journal_clock.log_print("Cancelled.");
            return std::nullopt;
        }
        if (value.empty()) {
            if (!current) {
                return recommended_hours;
            }
            return journal::extended_hours::parse_token(*current);
        }
        if (const auto parsed = journal::extended_hours::parse_token(value)) {
            return parsed;
        }
        std::cout << "That value is not a supported extended-hour token.\n";
    }
}

bool prompt_and_write(const std::optional<std::string>& current) {
    const auto hours = prompt_token(current);
    if (!hours) {
        return false;
    }
    return write_clock_file(*hours);
}

void print_menu() {
    std::cout
        << "\nJournal Clock\n"
        << "\n"
        << "1. Configure extended hours\n"
        << "2. Exit\n"
        << "> ";
}

} // namespace

int main(int argc, char* argv[]) {
    std::setvbuf(stdin, nullptr, _IONBF, 0);
    ac::shell::set_process_app_user_model_id();
    journal_clock.log_main("journal_clock.exe started");

    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }

    bool ini_present = false;
    if (!journal_ini_present(ini_present)) {
        return 1;
    }
    if (!ini_present && run_journal_config(*launch) != 0) {
        return 1;
    }

    bool present = false;
    if (!file_present(present)) {
        return 1;
    }

    namespace req = ac::config::components_request;
    req::log_config_request(journal_clock, *launch);
    const auto clock_file = journal::extended_hours::file_path().string();
    if (launch->seed) {
        if (present) {
            req::log_seed_skipped(journal_clock, clock_file);
            return 0;
        }
        req::log_writing_defaults(journal_clock);
        if (!write_clock_file(recommended_hours)) {
            journal_clock.log_print(
                "Journal clock configuration was not initialized."
            );
            return 1;
        }
        req::log_configuration_initialized(journal_clock);
        return 0;
    }

    if (launch->init) {
        if (present) {
            req::log_initialization_skipped(
                journal_clock,
                clock_file
            );
            return 0;
        }
        req::log_configuration_missing(journal_clock);
        if (!prompt_and_write(std::nullopt)) {
            journal_clock.log_print(
                "Journal clock configuration was not initialized."
            );
            return 1;
        }
        return 0;
    }

    if (!present && !prompt_and_write(std::nullopt)) {
        journal_clock.log_print(
            "Journal clock configuration was not initialized."
        );
        return 1;
    }

    while (true) {
        print_menu();
        std::string choice;
        if (!std::getline(std::cin, choice)) {
            return 0;
        }
        const auto value = trim(choice);
        if (value == "1") {
            const auto current = journal::extended_hours::canonical_token(
                journal::extended_hours::load()
            );
            if (!prompt_and_write(current)) {
                return 1;
            }
        }
        else if (value == "2") {
            return 0;
        }
        else {
            std::cout << "Unknown option.\n";
        }
    }
}
