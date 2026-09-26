import std;
import auto_core.core.component;
import auto_core.core.config;
import auto_core.core.paths;
import journal_database;
import journal_defaults;
import components_editor_request;

import <iostream>;
import <Windows.h>;

namespace {

ac::Component journal_config {"journal_config"};

std::string_view trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
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
    if (prompt) {
        std::cout << "Journal data directory [.\\journal]: ";
        std::string input;
        std::getline(std::cin, input);
        directory = std::string {trim(input)};
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
    const auto contents = journal::defaults::ini_for(directory);
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    output.close();
    return static_cast<bool>(output);
}

void print_error(const std::string& message) {
    journal_config.log_print("{}", message);
}

void list_series() {
    const auto series = journal_database::list_series();
    if (!series) {
        print_error(series.error());
        return;
    }
    if (series->empty()) {
        std::cout << "No journaling series yet.\n";
        return;
    }
    std::cout << "Journaling series:\n";
    for (const auto& entry : *series) {
        std::cout << std::format("  {}  {}\n", entry.name, entry.counter);
    }
}

void add_series() {
    std::cout << "Series name: ";
    std::string name;
    std::getline(std::cin, name);
    const auto result = journal_database::add_series(name);
    if (!result) {
        print_error(result.error());
        return;
    }
    journal_config.log_print("Created the series.");
}

void update_counter() {
    std::cout << "Series name: ";
    std::string name;
    std::getline(std::cin, name);
    const auto current = journal_database::find_series(name);
    if (!current) {
        print_error(current.error());
        return;
    }
    std::cout << std::format(
        "Current count for {}: {}\nNew count: ",
        current->name,
        current->counter
    );
    std::string value;
    std::getline(std::cin, value);
    const auto first = value.find_first_not_of(" \t");
    const auto last = value.find_last_not_of(" \t");
    if (first == std::string::npos) {
        print_error("Count must be a whole number.");
        return;
    }
    value = value.substr(first, last - first + 1);
    int counter = 0;
    const auto parsed = std::from_chars(
        value.data(), value.data() + value.size(), counter
    );
    if (parsed.ec != std::errc {} ||
        parsed.ptr != value.data() + value.size()) {
        print_error("Count must be a whole number.");
        return;
    }
    const auto result = journal_database::set_counter(name, counter);
    if (!result) {
        print_error(result.error());
        return;
    }
    journal_config.log_print(
        "{} count is now {}.",
        result->name,
        result->counter
    );
}

void set_firebase_url() {
    const auto current = journal_database::firebase_url();
    if (!current) {
        print_error(current.error());
        return;
    }
    if (*current) {
        std::cout << "Current Firebase URL: " << **current << '\n';
    }
    else {
        std::cout << "Firebase URL is not set.\n";
    }
    std::cout << "New URL (blank to clear): ";
    std::string url;
    std::getline(std::cin, url);
    const auto result = journal_database::set_firebase_url(url);
    if (!result) {
        print_error(result.error());
        return;
    }
    journal_config.log_print("Saved Firebase URL.");
}

void print_menu() {
    std::cout
        << "\njournal_config\n"
        << "  1. List journaling series\n"
        << "  2. Add a journaling series\n"
        << "  3. Update series count\n"
        << "  4. Set Firebase URL\n"
        << "  5. Exit\n"
        << "> ";
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
    journal_config.log_main("journal_config.exe started");

    const bool initialize =
        ac::config::components_request::is_initialize_run(argc, argv);
    if (!ensure_journal_ini(!initialize)) {
        journal_config.log_print("Journal configuration was not initialized.");
        return 1;
    }

    ac::config::seed_missing_journal_choices();

    if (initialize) {
        journal_config.log_main("journal.ini initialized");
        return ac::config::components_request::run_component_update("journal");
    }

    activate_own_console();
    std::cout << "Database: " << journal_database::file_path().string() << '\n';

    while (true) {
        print_menu();
        std::string choice;
        if (!std::getline(std::cin, choice)) {
            return ac::config::components_request::run_component_update("journal");
        }
        if (choice == "1") {
            list_series();
        }
        else if (choice == "2") {
            add_series();
        }
        else if (choice == "3") {
            update_counter();
        }
        else if (choice == "4") {
            set_firebase_url();
        }
        else if (choice == "5" || choice == "q" || choice == "Q") {
            return ac::config::components_request::run_component_update("journal");
        }
        else {
            std::cout << "Unknown option.\n";
        }
    }
}
