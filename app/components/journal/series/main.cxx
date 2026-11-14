import std;
import auto_core.core.component;
import auto_core.core.logging.config;
import auto_core.core.ini;
import auto_core.core.paths;
import journal_auto_select;
import journal_db_protocol;
import journal_episode_format;
import journal_series_map;
import journal_sqlite;
import components_editor_request;

import <iostream>;
import auto_core.core.shell;

namespace {

ac::Component journal_series {
    "journal_series",
    ac::logging::config::LoggingScope {"journal"}
};

std::optional<journal::sqlite::Store> series_connection;
journal::sqlite::Store* series_store = nullptr;

constexpr std::string_view seed_series_name = "Auto Core";
constexpr int seed_series_padding = 2;

std::string_view trim(const std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

void print_error(const std::string& message) {
    journal_series.log_alert("{}", message);
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
        const auto found = series_store->find_series(candidate);
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
        const auto latest = series_store->find_series("");
        if (latest) {
            resolved = latest->name;
        }
        else if (!latest.error().starts_with("No journal series yet")) {
            print_error(latest.error());
            return false;
        }
    }

    const auto series = series_store->list_series();
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
    print_error("No journal series yet. Add one with journal_ac.exe --series.");
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
    const auto series = series_store->list_series();
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
    const auto found = series_store->find_series(trim(name));
    if (!found) {
        print_error(found.error());
        return;
    }
    if (!refresh_map(found->name)) {
        return;
    }
    journal_series.log_alert("Active series is {}.", found->name);
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
    const auto added = series_store->add_series(name, *padding);
    if (!added) {
        print_error(added.error());
        return;
    }
    journal_series.log_alert("Created the series.");
    const auto created = series_store->find_series(trim(name));
    if (!created) {
        print_error(created.error());
        (void)refresh_map(std::nullopt);
        return;
    }
    const auto document = ac::ini::read(
        ac::paths::component_config_file("journal")
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
    const auto current = series_store->find_series(trim(name));
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
    const auto updated = series_store->set_counter(current->name, *counter);
    if (!updated) {
        print_error(updated.error());
        return;
    }
    journal_series.log_alert(
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
    const auto current = series_store->find_series(trim(name));
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
    const auto updated = series_store->set_padding(current->name, *padding);
    if (!updated) {
        print_error(updated.error());
        return;
    }
    journal_series.log_alert(
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

std::expected<void, std::string> open_connection() {
    auto opened = journal::sqlite::open();
    if (!opened) {
        return std::unexpected(opened.error());
    }
    series_connection = std::move(*opened);
    series_store = &*series_connection;
    return {};
}

void close_connection() {
    series_store = nullptr;
    series_connection.reset();
}

bool database_file_present(bool& present) {
    const auto path = journal::sqlite::file_path();
    std::error_code error;
    present = std::filesystem::exists(path, error);
    if (error) {
        print_error(std::format(
            "Failed to inspect {}: {}",
            path.string(),
            error.message()
        ));
        return false;
    }
    return true;
}

std::optional<std::string> prompt_series_name() {
    while (true) {
        std::cout << "Series name [" << seed_series_name << "]: ";
        std::string input;
        if (!std::getline(std::cin, input)) {
            print_error("Journal database was not initialized.");
            return std::nullopt;
        }
        const auto value = trim(input);
        if (value == "cancel") {
            print_error("Cancelled.");
            return std::nullopt;
        }
        const std::string name = value.empty()
            ? std::string {seed_series_name}
            : std::string {value};
        if (name.empty()) {
            std::cout << "Enter a series name.\n";
            continue;
        }
        return name;
    }
}

std::optional<int> prompt_initial_padding() {
    while (true) {
        std::cout << "Padding [" << seed_series_padding << "]: ";
        std::string input;
        if (!std::getline(std::cin, input)) {
            print_error("Journal database was not initialized.");
            return std::nullopt;
        }
        const auto value = trim(input);
        if (value == "cancel") {
            print_error("Cancelled.");
            return std::nullopt;
        }
        const std::string text = value.empty()
            ? std::to_string(seed_series_padding)
            : std::string {value};
        int padding = 0;
        const auto parsed = std::from_chars(
            text.data(),
            text.data() + text.size(),
            padding
        );
        if (parsed.ec != std::errc {} || parsed.ptr != text.data() + text.size()) {
            std::cout << "Enter a whole number.\n";
            continue;
        }
        if (padding < 0 || padding > 16) {
            std::cout << "Padding must be from 0 through 16.\n";
            continue;
        }
        return padding;
    }
}

int ensure_database(const bool init) {
    bool present = false;
    if (!database_file_present(present)) {
        return 1;
    }
    const auto path = journal::sqlite::file_path().string();
    namespace req = ac::config::components_request;
    if (present) {
        if (init) {
            req::log_initialization_skipped(journal_series, path);
        }
        else {
            req::log_seed_skipped(journal_series, path);
        }
        return 0;
    }

    if (init) {
        req::log_configuration_missing(journal_series);
        const auto name = prompt_series_name();
        if (!name) {
            return 1;
        }
        const auto padding = prompt_initial_padding();
        if (!padding) {
            return 1;
        }
        const auto created = journal::sqlite::create_initial(*name, *padding);
        if (!created) {
            print_error(created.error());
            return 1;
        }
        journal_series.log_debug("Wrote {}", path);
        return 0;
    }

    req::log_writing_defaults(journal_series);
    const auto created = journal::sqlite::create_initial(
        seed_series_name,
        seed_series_padding
    );
    if (!created) {
        print_error(created.error());
        return 1;
    }
    journal_series.log_debug("Wrote {}", path);
    req::log_configuration_initialized(journal_series);
    return 0;
}

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
    if (series_store == nullptr) {
        print_error("series.db is missing.");
        return 1;
    }
    if (!refresh_map(std::nullopt)) {
        return 1;
    }
    journal_series.log_debug("Wrote {}", map_path.string());
    return 0;
}

} // namespace

namespace ac::journal_app::series {

int run(int argc, char* argv[]) {
    ac::shell::set_process_app_user_model_id();
    journal_series.log_info("journal_ac.exe --series started");

    const auto launch =
        ac::config::components_request::parse_config_launch(argc, argv);
    if (!launch) {
        return 1;
    }
    if (launch->seed || launch->init) {
        namespace req = ac::config::components_request;
        req::log_config_request(journal_series, *launch);
        if (ensure_database(launch->init) != 0) {
            return 1;
        }
        if (const auto opened = open_connection(); !opened) {
            print_error(opened.error());
            return 1;
        }
        struct ConnectionGuard {
            ~ConnectionGuard() { close_connection(); }
        } connection_guard;
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
        std::cerr << "Usage: journal_ac.exe --series [--seed | --init]\n";
        return 1;
    }

    if (const auto opened = open_connection(); !opened) {
        print_error(opened.error());
        return 1;
    }
    struct ConnectionGuard {
        ~ConnectionGuard() { close_connection(); }
    } connection_guard;
    return run_menu();
}

} // namespace ac::journal_app::series
