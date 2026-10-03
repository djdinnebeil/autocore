import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.ini;
import auto_core.core.paths;
import journal_auto_select;
import journal_db_protocol;
import journal_db_session;
import journal_episode_format;
import journal_series_map;
import components_editor_request;

import <iostream>;
import <Windows.h>;
import auto_core.core.shell;

namespace {

ac::Component journal_series {
    "journal_series",
    ac::logging::config::LoggingScope {"journal"}
};

struct OwnedService {
    HANDLE process = nullptr;
    bool owns_process = false;
};

std::wstring quote_argument(const std::wstring_view value) {
    std::wstring quoted;
    quoted.reserve(value.size() + 2);
    quoted.push_back(L'"');
    quoted.append(value);
    quoted.push_back(L'"');
    return quoted;
}

bool process_exited(const HANDLE process) {
    if (process == nullptr) {
        return true;
    }
    DWORD exit_code = STILL_ACTIVE;
    if (!GetExitCodeProcess(process, &exit_code)) {
        return true;
    }
    return exit_code != STILL_ACTIVE;
}

void close_process(OwnedService& service) {
    if (service.process != nullptr) {
        CloseHandle(service.process);
        service.process = nullptr;
    }
}

std::string_view trim(const std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

void print_error(const std::string& message) {
    journal_series.log_print("{}", message);
    std::cout << message << '\n';
}

bool write_map(const std::string& text) {
    const auto path = journal::series_map::file_path();
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) {
        print_error(std::format(
            "Unable to create {}: {}",
            path.parent_path().string(),
            error.message()
        ));
        return false;
    }
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        print_error(std::format("Unable to write {}.", path.string()));
        return false;
    }
    output << text;
    output.close();
    if (!output) {
        print_error(std::format("Unable to write {}.", path.string()));
        return false;
    }
    return true;
}

/**
 * \brief Rewrites `series.map` from the database.
 *
 * This is one of the only five snapshot writes. `journal_ac.exe` allocation
 * does not call it. A null override keeps a usable active name from the
 * current file. An override is the series just selected.
 */
bool refresh_map(const std::optional<std::string>& override_name) {
    std::string candidate;
    if (override_name) {
        candidate = *override_name;
    }
    else {
        const auto active = journal::series_map::read_active(
            journal::series_map::file_path()
        );
        if (active.kind == journal::series_map::ActiveKind::named) {
            candidate = active.name;
        }
    }

    std::string resolved;
    if (!candidate.empty()) {
        const auto found = journal::db::session::find_series(candidate);
        if (found) {
            resolved = found->name;
        }
        else if (!journal::db::is_unknown_series(found.error()) &&
                 !found.error().starts_with("No journal series yet")) {
            print_error(found.error());
            return false;
        }
    }
    if (resolved.empty()) {
        const auto latest = journal::db::session::find_series("");
        if (latest) {
            resolved = latest->name;
        }
        else if (!latest.error().starts_with("No journal series yet")) {
            print_error(latest.error());
            return false;
        }
    }

    const auto series = journal::db::session::list_series();
    if (!series) {
        print_error(series.error());
        return false;
    }
    std::vector<journal::series_map::SnapshotSeries> rows;
    rows.reserve(series->size());
    for (const auto& entry : *series) {
        rows.push_back(journal::series_map::SnapshotSeries {
            entry.name,
            entry.next_episode,
            entry.padding
        });
    }
    return write_map(journal::series_map::render(resolved, rows));
}

void report_no_series() {
    print_error("No journal series yet. Add one with journal_series.exe.");
}

std::optional<int> prompt_whole_number(const std::string_view label) {
    std::cout << label;
    std::string value;
    if (!std::getline(std::cin, value)) {
        return std::nullopt;
    }
    const auto trimmed = trim(value);
    if (trimmed.empty()) {
        print_error("Enter a whole number.");
        return std::nullopt;
    }
    int number = 0;
    const auto parsed = std::from_chars(
        trimmed.data(),
        trimmed.data() + trimmed.size(),
        number
    );
    if (parsed.ec != std::errc {} ||
        parsed.ptr != trimmed.data() + trimmed.size()) {
        print_error("Enter a whole number.");
        return std::nullopt;
    }
    return number;
}

void select_active() {
    const auto series = journal::db::session::list_series();
    if (!series) {
        print_error(series.error());
        return;
    }
    if (series->empty()) {
        report_no_series();
        return;
    }
    std::cout << "Journaling series:\n";
    for (const auto& entry : *series) {
        std::cout << std::format(
            "  {}  {}\n",
            entry.name,
            journal::format_episode_number(entry.next_episode, entry.padding)
        );
    }
    std::cout << "Active series: ";
    std::string name;
    if (!std::getline(std::cin, name)) {
        return;
    }
    const auto found = journal::db::session::find_series(trim(name));
    if (!found) {
        print_error(found.error());
        return;
    }
    if (!refresh_map(found->name)) {
        return;
    }
    journal_series.log_print("Active series is {}.", found->name);
}

void add_series() {
    std::cout << "\nAdd a journaling series\n\nName: ";
    std::string name;
    if (!std::getline(std::cin, name)) {
        return;
    }
    const auto padding = prompt_whole_number("Padding: ");
    if (!padding) {
        return;
    }
    const auto added = journal::db::session::add_series(name, *padding);
    if (!added) {
        print_error(added.error());
        return;
    }
    journal_series.log_print("Created the series.");
    const auto created = journal::db::session::find_series(trim(name));
    if (!created) {
        print_error(created.error());
        (void)refresh_map(std::nullopt);
        return;
    }
    const auto document = ac::ini::read(
        ac::paths::config_directory() / "journal.ini"
    );
    const bool select_new = journal::auto_select::enabled(
        document ? document->find("journal", "auto_select_new_series") : std::nullopt
    );
    if (select_new) {
        (void)refresh_map(created->name);
    }
    else {
        (void)refresh_map(std::nullopt);
    }
}

void update_counter() {
    std::cout << "Series name: ";
    std::string name;
    if (!std::getline(std::cin, name)) {
        return;
    }
    const auto current = journal::db::session::find_series(trim(name));
    if (!current) {
        print_error(current.error());
        return;
    }
    const auto counter = prompt_whole_number(std::format(
        "Current count for {}: {}\nNew count: ",
        current->name,
        current->next_episode
    ));
    if (!counter) {
        return;
    }
    const auto updated = journal::db::session::set_counter(current->name, *counter);
    if (!updated) {
        print_error(updated.error());
        return;
    }
    journal_series.log_print(
        "{} count is now {}.",
        updated->name,
        updated->next_episode
    );
    (void)refresh_map(std::nullopt);
}

void update_padding() {
    std::cout << "Series name: ";
    std::string name;
    if (!std::getline(std::cin, name)) {
        return;
    }
    const auto current = journal::db::session::find_series(trim(name));
    if (!current) {
        print_error(current.error());
        return;
    }
    const auto padding = prompt_whole_number(std::format(
        "Current padding for {}: {}\nNew padding: ",
        current->name,
        current->padding
    ));
    if (!padding) {
        return;
    }
    const auto updated = journal::db::session::set_padding(current->name, *padding);
    if (!updated) {
        print_error(updated.error());
        return;
    }
    journal_series.log_print(
        "{} padding is now {}.",
        updated->name,
        updated->padding
    );
    (void)refresh_map(std::nullopt);
}

int run_menu() {
    while (true) {
        std::cout
            << "\njournal series\n"
            << "  1. Select active series\n"
            << "  2. Add a journaling series\n"
            << "  3. Update series count\n"
            << "  4. Update series padding\n"
            << "  5. Refresh series.map\n"
            << "  6. Exit\n"
            << "> ";
        std::string choice;
        if (!std::getline(std::cin, choice)) {
            return 0;
        }
        choice = std::string {trim(choice)};
        if (choice == "1") {
            select_active();
        }
        else if (choice == "2") {
            add_series();
        }
        else if (choice == "3") {
            update_counter();
        }
        else if (choice == "4") {
            update_padding();
        }
        else if (choice == "5") {
            (void)refresh_map(std::nullopt);
        }
        else if (choice == "6" || choice == "q" || choice == "Q") {
            return 0;
        }
        else {
            std::cout << "Unknown option.\n";
        }
    }
}

std::expected<OwnedService, std::string> start_database() {
    const std::filesystem::path executable =
        ac::paths::bin_directory() / "journal_db.exe";
    std::wstring command =
        quote_argument(executable.wstring()) + L" " +
        quote_argument(L"--serve");
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
        return std::unexpected("Unable to start journal_db.exe.");
    }
    CloseHandle(process.hThread);
    OwnedService service;
    service.process = process.hProcess;
    const auto ready = journal::db::session::probe();
    if (!ready) {
        if (!process_exited(service.process)) {
            TerminateProcess(service.process, 1);
        }
        close_process(service);
        return std::unexpected(ready.error());
    }
    if (process_exited(service.process)) {
        close_process(service);
        service.owns_process = false;
    }
    else {
        service.owns_process = true;
    }
    return service;
}

void stop_database(OwnedService& service);

int generate_missing_map() {
    const auto map_path = journal::series_map::file_path();
    std::error_code error;
    const bool present = std::filesystem::exists(map_path, error);
    if (error) {
        print_error(std::format(
            "Failed to inspect {}: {}",
            map_path.string(),
            error.message()
        ));
        return 1;
    }
    if (present) {
        return 0;
    }

    const auto database_path = ac::paths::journal_directory() / "series.db";
    const bool database_present = std::filesystem::exists(database_path, error);
    if (error || !database_present) {
        print_error("series.db is missing. journal_db.exe owns that file.");
        return 1;
    }

    auto database = start_database();
    if (!database) {
        print_error(database.error());
        return 1;
    }
    struct Guard {
        OwnedService* service;
        ~Guard() { stop_database(*service); }
    } guard {&*database};

    if (!refresh_map(std::nullopt)) {
        return 1;
    }
    journal_series.log_print("Wrote {}", map_path.string());
    return 0;
}

void stop_database(OwnedService& service) {
    if (service.owns_process) {
        (void)journal::db::session::shutdown();
        if (service.process != nullptr) {
            WaitForSingleObject(service.process, 5000);
            if (!process_exited(service.process)) {
                TerminateProcess(service.process, 1);
                WaitForSingleObject(service.process, 1000);
            }
        }
    }
    close_process(service);
}

} // namespace

int main(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    journal_series.log_main("journal_series.exe started");

    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }
    if (launch->seed || launch->init) {
        namespace req = ac::config::components_request;
        req::log_config_request(journal_series, *launch);
        const auto map_path = journal::series_map::file_path().string();
        std::error_code error;
        const bool present = std::filesystem::exists(
            journal::series_map::file_path(),
            error
        );
        if (error) {
            print_error(std::format(
                "Failed to inspect {}: {}",
                map_path,
                error.message()
            ));
            return 1;
        }
        if (present) {
            if (launch->seed) {
                req::log_seed_skipped(journal_series, map_path);
            }
            else {
                req::log_initialization_skipped(journal_series, map_path);
            }
            return 0;
        }
        if (generate_missing_map() != 0) {
            return 1;
        }
        req::log_configuration_initialized(journal_series);
        return 0;
    }
    if (argc > 1) {
        std::cerr << "Usage: journal_series.exe [--seed | --init]\n";
        return 1;
    }

    auto database = start_database();
    if (!database) {
        print_error(database.error());
        return 1;
    }
    struct Guard {
        OwnedService* service;
        ~Guard() { stop_database(*service); }
    } guard {&*database};
    return run_menu();
}
