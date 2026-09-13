import std;
import auto_core.core.config;
import auto_core.core.paths;
import journal_database;

import <iostream>;
import <Windows.h>;

namespace {

std::string_view trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

bool ensure_journal_ini() {
    const auto path = ac::paths::config_directory() / "journal.ini";

    std::error_code error;
    if (std::filesystem::exists(path, error)) {
        return true;
    }
    if (error) {
        std::cerr
            << "Failed to inspect "
            << path.string()
            << ": "
            << error.message()
            << '\n';
        return false;
    }

    std::cout << "Journal data directory [.\\journal]: ";
    std::string input;
    std::getline(std::cin, input);

    const auto directory = trim(input);
    return ac::config::write_journal_ini_if_missing(
        directory.empty() ? "journal" : directory
    );
}

void print_error(const std::string& message) {
    std::cout << message << '\n';
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
    std::cout << "Created the series.\n";
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
    std::cout << std::format(
        "{} count is now {}.\n", result->name, result->counter
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
    std::cout << "Saved.\n";
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

int main() {
    if (!ensure_journal_ini()) {
        return 1;
    }

    ac::config::initialize_core_settings();
    ac::config::seed_missing_journal_choices();

    activate_own_console();
    std::cout << "Database: " << journal_database::file_path().string() << '\n';

    while (true) {
        print_menu();
        std::string choice;
        if (!std::getline(std::cin, choice)) {
            return 0;
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
            return 0;
        }
        else {
            std::cout << "Unknown option.\n";
        }
    }
}
