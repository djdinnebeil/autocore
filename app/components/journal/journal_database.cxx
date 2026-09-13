module;

#include <sqlite3.h>

module journal_database;

import std;
import auto_core.core.paths;

namespace {

constexpr std::string_view config_table = "_config";

std::string utf8_path(const std::filesystem::path& path) {
    const auto u8 = path.u8string();
    return {u8.begin(), u8.end()};
}

std::string quote_identifier(std::string_view name) {
    std::string quoted;
    quoted.reserve(name.size() + 2);
    quoted.push_back('"');
    for (const char character : name) {
        if (character == '"') {
            quoted.push_back('"');
        }
        quoted.push_back(character);
    }
    quoted.push_back('"');
    return quoted;
}

std::string trim_copy(std::string_view value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t");
    return std::string {value.substr(first, last - first + 1)};
}

bool ascii_iequals(std::string_view left, std::string_view right) {
    if (left.size() != right.size()) {
        return false;
    }
    for (std::size_t index = 0; index < left.size(); ++index) {
        const auto fold = [](unsigned char character) {
            return character >= 'A' && character <= 'Z'
                ? static_cast<char>(character + ('a' - 'A'))
                : static_cast<char>(character);
        };
        if (fold(static_cast<unsigned char>(left[index])) !=
            fold(static_cast<unsigned char>(right[index]))) {
            return false;
        }
    }
    return true;
}

bool is_reserved_table(std::string_view name) {
    if (ascii_iequals(name, config_table)) {
        return true;
    }
    return name.size() >= 7 && ascii_iequals(name.substr(0, 7), "sqlite_");
}

bool is_valid_series_name(std::string_view name) {
    if (name.empty() || is_reserved_table(name)) {
        return false;
    }
    for (const unsigned char character : name) {
        if (character < 32 || character == 127) {
            return false;
        }
    }
    return true;
}

std::string sqlite_error(sqlite3* database, std::string_view prefix) {
    const char* message = database != nullptr
        ? sqlite3_errmsg(database)
        : "SQLite did not provide a database handle";
    return std::format("{}: {}", prefix, message);
}

bool execute_sql(sqlite3* database, const char* sql, std::string& error) {
    char* error_message = nullptr;
    const int result = sqlite3_exec(database, sql, nullptr, nullptr, &error_message);
    if (result == SQLITE_OK) {
        return true;
    }
    error = std::format(
        "SQL error: {}",
        error_message != nullptr ? error_message : sqlite3_errmsg(database)
    );
    sqlite3_free(error_message);
    return false;
}

using Database = std::unique_ptr<sqlite3, decltype(&sqlite3_close)>;

std::expected<Database, std::string> open_database() {
    const auto path = journal_database::file_path();
    std::error_code error_code;
    std::filesystem::create_directories(path.parent_path(), error_code);
    if (error_code) {
        return std::unexpected(
            std::format("Unable to create {}: {}", path.parent_path().string(), error_code.message())
        );
    }

    sqlite3* handle = nullptr;
    const std::string utf8 = utf8_path(path);
    const int open_result = sqlite3_open(utf8.c_str(), &handle);
    Database database {handle, &sqlite3_close};
    if (open_result != SQLITE_OK) {
        return std::unexpected(sqlite_error(handle, "Unable to open journals.db"));
    }

    std::string error;
    if (!execute_sql(
            database.get(),
            "CREATE TABLE IF NOT EXISTS \"_config\" (firebase_url TEXT);",
            error
        )) {
        return std::unexpected(error);
    }
    if (!execute_sql(
            database.get(),
            "INSERT INTO \"_config\" (firebase_url) SELECT NULL "
            "WHERE NOT EXISTS (SELECT 1 FROM \"_config\");",
            error
        )) {
        return std::unexpected(error);
    }
    return database;
}

std::expected<std::string, std::string> latest_series_table(sqlite3* database) {
    sqlite3_stmt* statement = nullptr;
    constexpr const char* sql =
        "SELECT name FROM sqlite_master WHERE type = 'table' "
        "AND name NOT LIKE 'sqlite_%' AND name != '_config' "
        "ORDER BY rowid DESC LIMIT 1;";
    if (sqlite3_prepare_v2(database, sql, -1, &statement, nullptr) != SQLITE_OK) {
        sqlite3_finalize(statement);
        return std::unexpected(
            sqlite_error(database, "Unable to look up journal series")
        );
    }
    const int step = sqlite3_step(statement);
    if (step != SQLITE_ROW) {
        sqlite3_finalize(statement);
        return std::unexpected(
            "No journal series yet. Add one with journal_config.exe."
        );
    }
    const unsigned char* stored = sqlite3_column_text(statement, 0);
    std::string table = stored != nullptr
        ? reinterpret_cast<const char*>(stored)
        : std::string {};
    sqlite3_finalize(statement);
    if (table.empty() || is_reserved_table(table)) {
        return std::unexpected(
            "No journal series yet. Add one with journal_config.exe."
        );
    }
    return table;
}

std::expected<std::string, std::string> resolve_table_name(
    sqlite3* database,
    std::string_view series_key
) {
    const std::string key = trim_copy(series_key);
    if (key.empty()) {
        return latest_series_table(database);
    }
    if (is_reserved_table(key)) {
        return std::unexpected(
            std::format("Invalid journal series '{}'.", key)
        );
    }

    sqlite3_stmt* statement = nullptr;
    constexpr const char* sql =
        "SELECT name FROM sqlite_master WHERE type = 'table' AND name = ? COLLATE NOCASE;";
    if (sqlite3_prepare_v2(database, sql, -1, &statement, nullptr) != SQLITE_OK) {
        sqlite3_finalize(statement);
        return std::unexpected(sqlite_error(database, "Unable to look up journal series"));
    }
    sqlite3_bind_text(
        statement,
        1,
        key.data(),
        static_cast<int>(key.size()),
        SQLITE_TRANSIENT
    );
    const int step = sqlite3_step(statement);
    if (step != SQLITE_ROW) {
        sqlite3_finalize(statement);
        return std::unexpected(
            std::format("Unknown journal series '{}'.", key)
        );
    }
    const unsigned char* stored = sqlite3_column_text(statement, 0);
    std::string table = stored != nullptr
        ? reinterpret_cast<const char*>(stored)
        : std::string {};
    sqlite3_finalize(statement);
    if (table.empty() || is_reserved_table(table)) {
        return std::unexpected(
            std::format("Unknown journal series '{}'.", key)
        );
    }
    return table;
}

} // namespace

namespace journal_database {

std::filesystem::path file_path() {
    return ac::paths::journal_directory() / "journals.db";
}

std::expected<std::vector<Series>, std::string> list_series() {
    auto database = open_database();
    if (!database) {
        return std::unexpected(database.error());
    }

    sqlite3_stmt* statement = nullptr;
    constexpr const char* sql =
        "SELECT name FROM sqlite_master WHERE type = 'table' "
        "AND name NOT LIKE 'sqlite_%' AND name != '_config' "
        "ORDER BY name COLLATE NOCASE;";
    if (sqlite3_prepare_v2(database->get(), sql, -1, &statement, nullptr) !=
        SQLITE_OK) {
        sqlite3_finalize(statement);
        return std::unexpected(
            sqlite_error(database->get(), "Unable to list journal series")
        );
    }

    std::vector<Series> series;
    while (sqlite3_step(statement) == SQLITE_ROW) {
        const unsigned char* table_text = sqlite3_column_text(statement, 0);
        if (table_text == nullptr) {
            continue;
        }
        const std::string table {reinterpret_cast<const char*>(table_text)};
        const std::string select = std::format(
            "SELECT name, counter FROM {} LIMIT 1;",
            quote_identifier(table)
        );
        sqlite3_stmt* row_statement = nullptr;
        if (sqlite3_prepare_v2(
                database->get(), select.c_str(), -1, &row_statement, nullptr
            ) != SQLITE_OK) {
            sqlite3_finalize(row_statement);
            sqlite3_finalize(statement);
            return std::unexpected(
                sqlite_error(database->get(), "Unable to read journal series")
            );
        }
        Series entry;
        if (sqlite3_step(row_statement) == SQLITE_ROW) {
            const unsigned char* name_text = sqlite3_column_text(row_statement, 0);
            entry.name = name_text != nullptr
                ? reinterpret_cast<const char*>(name_text)
                : table;
            entry.counter = sqlite3_column_int(row_statement, 1);
        }
        else {
            entry.name = table;
        }
        sqlite3_finalize(row_statement);
        series.push_back(std::move(entry));
    }
    sqlite3_finalize(statement);
    return series;
}

std::expected<void, std::string> add_series(std::string_view name) {
    const std::string trimmed = trim_copy(name);
    if (!is_valid_series_name(trimmed)) {
        return std::unexpected(
            "Series name is empty, reserved, or contains invalid characters."
        );
    }

    auto database = open_database();
    if (!database) {
        return std::unexpected(database.error());
    }

    sqlite3_stmt* existing = nullptr;
    constexpr const char* existing_sql =
        "SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = ? COLLATE NOCASE;";
    if (sqlite3_prepare_v2(
            database->get(), existing_sql, -1, &existing, nullptr
        ) != SQLITE_OK) {
        sqlite3_finalize(existing);
        return std::unexpected(
            sqlite_error(database->get(), "Unable to check journal series")
        );
    }
    sqlite3_bind_text(
        existing,
        1,
        trimmed.data(),
        static_cast<int>(trimmed.size()),
        SQLITE_TRANSIENT
    );
    if (sqlite3_step(existing) == SQLITE_ROW) {
        sqlite3_finalize(existing);
        return std::unexpected(
            std::format("A journal series named '{}' already exists.", trimmed)
        );
    }
    sqlite3_finalize(existing);

    const std::string quoted = quote_identifier(trimmed);
    const std::string create = std::format(
        "CREATE TABLE {} (name TEXT NOT NULL, counter INTEGER NOT NULL);",
        quoted
    );
    std::string error;
    if (!execute_sql(database->get(), create.c_str(), error)) {
        return std::unexpected(error);
    }

    const std::string insert = std::format(
        "INSERT INTO {} (name, counter) VALUES (?, 1);",
        quoted
    );
    sqlite3_stmt* insert_statement = nullptr;
    if (sqlite3_prepare_v2(
            database->get(), insert.c_str(), -1, &insert_statement, nullptr
        ) != SQLITE_OK) {
        sqlite3_finalize(insert_statement);
        return std::unexpected(
            sqlite_error(database->get(), "Unable to seed journal series")
        );
    }
    sqlite3_bind_text(
        insert_statement,
        1,
        trimmed.data(),
        static_cast<int>(trimmed.size()),
        SQLITE_TRANSIENT
    );
    if (sqlite3_step(insert_statement) != SQLITE_DONE) {
        sqlite3_finalize(insert_statement);
        return std::unexpected(
            sqlite_error(database->get(), "Unable to seed journal series")
        );
    }
    sqlite3_finalize(insert_statement);
    return {};
}

std::expected<Series, std::string> read_series_row(
    sqlite3* database,
    std::string_view table
) {
    const std::string select = std::format(
        "SELECT name, counter FROM {} LIMIT 1;",
        quote_identifier(table)
    );
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(
            database, select.c_str(), -1, &statement, nullptr
        ) != SQLITE_OK) {
        sqlite3_finalize(statement);
        return std::unexpected(
            sqlite_error(database, "Unable to read journal series")
        );
    }
    if (sqlite3_step(statement) != SQLITE_ROW) {
        sqlite3_finalize(statement);
        return std::unexpected(
            std::format("Journal series '{}' has no counter row.", table)
        );
    }
    Series entry;
    const unsigned char* name_text = sqlite3_column_text(statement, 0);
    entry.name = name_text != nullptr
        ? reinterpret_cast<const char*>(name_text)
        : std::string {table};
    entry.counter = sqlite3_column_int(statement, 1);
    sqlite3_finalize(statement);
    return entry;
}

std::expected<Series, std::string> latest_series() {
    auto database = open_database();
    if (!database) {
        return std::unexpected(database.error());
    }
    const auto table = latest_series_table(database->get());
    if (!table) {
        return std::unexpected(table.error());
    }
    return read_series_row(database->get(), *table);
}

std::expected<Series, std::string> find_series(std::string_view series_key) {
    auto database = open_database();
    if (!database) {
        return std::unexpected(database.error());
    }
    const auto table = resolve_table_name(database->get(), series_key);
    if (!table) {
        return std::unexpected(table.error());
    }
    return read_series_row(database->get(), *table);
}

std::expected<Series, std::string> set_counter(
    std::string_view series_key,
    int counter
) {
    if (counter < 0) {
        return std::unexpected("Counter must be zero or greater.");
    }

    auto database = open_database();
    if (!database) {
        return std::unexpected(database.error());
    }

    const auto table = resolve_table_name(database->get(), series_key);
    if (!table) {
        return std::unexpected(table.error());
    }

    const std::string update = std::format(
        "UPDATE {} SET counter = ?;",
        quote_identifier(*table)
    );
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(
            database->get(), update.c_str(), -1, &statement, nullptr
        ) != SQLITE_OK) {
        sqlite3_finalize(statement);
        return std::unexpected(
            sqlite_error(database->get(), "Unable to update journal counter")
        );
    }
    sqlite3_bind_int(statement, 1, counter);
    if (sqlite3_step(statement) != SQLITE_DONE) {
        sqlite3_finalize(statement);
        return std::unexpected(
            sqlite_error(database->get(), "Unable to update journal counter")
        );
    }
    sqlite3_finalize(statement);
    if (sqlite3_changes(database->get()) < 1) {
        return std::unexpected(
            std::format("Journal series '{}' has no counter row.", *table)
        );
    }

    auto updated = read_series_row(database->get(), *table);
    if (!updated) {
        return std::unexpected(updated.error());
    }
    return *updated;
}

std::expected<Episode, std::string> take_next_episode(std::string_view series_key) {
    auto database = open_database();
    if (!database) {
        return std::unexpected(database.error());
    }

    const auto table = resolve_table_name(database->get(), series_key);
    if (!table) {
        return std::unexpected(table.error());
    }

    const std::string quoted = quote_identifier(*table);
    const std::string select = std::format(
        "SELECT name, counter FROM {} LIMIT 1;",
        quoted
    );
    sqlite3_stmt* select_statement = nullptr;
    if (sqlite3_prepare_v2(
            database->get(), select.c_str(), -1, &select_statement, nullptr
        ) != SQLITE_OK) {
        sqlite3_finalize(select_statement);
        return std::unexpected(
            sqlite_error(database->get(), "Unable to read journal counter")
        );
    }
    if (sqlite3_step(select_statement) != SQLITE_ROW) {
        sqlite3_finalize(select_statement);
        return std::unexpected(
            std::format("Journal series '{}' has no counter row.", *table)
        );
    }
    const unsigned char* name_text = sqlite3_column_text(select_statement, 0);
    Episode episode;
    episode.name = name_text != nullptr
        ? reinterpret_cast<const char*>(name_text)
        : *table;
    episode.number = sqlite3_column_int(select_statement, 1);
    sqlite3_finalize(select_statement);

    const std::string update = std::format(
        "UPDATE {} SET counter = ?;",
        quoted
    );
    sqlite3_stmt* update_statement = nullptr;
    if (sqlite3_prepare_v2(
            database->get(), update.c_str(), -1, &update_statement, nullptr
        ) != SQLITE_OK) {
        sqlite3_finalize(update_statement);
        return std::unexpected(
            sqlite_error(database->get(), "Unable to update journal counter")
        );
    }
    sqlite3_bind_int(update_statement, 1, episode.number + 1);
    if (sqlite3_step(update_statement) != SQLITE_DONE) {
        sqlite3_finalize(update_statement);
        return std::unexpected(
            sqlite_error(database->get(), "Unable to update journal counter")
        );
    }
    sqlite3_finalize(update_statement);
    return episode;
}

std::expected<std::optional<std::string>, std::string> firebase_url() {
    auto database = open_database();
    if (!database) {
        return std::unexpected(database.error());
    }

    sqlite3_stmt* statement = nullptr;
    constexpr const char* sql = "SELECT firebase_url FROM \"_config\" LIMIT 1;";
    if (sqlite3_prepare_v2(database->get(), sql, -1, &statement, nullptr) !=
        SQLITE_OK) {
        sqlite3_finalize(statement);
        return std::unexpected(
            sqlite_error(database->get(), "Unable to read Firebase URL")
        );
    }
    std::optional<std::string> url;
    if (sqlite3_step(statement) == SQLITE_ROW) {
        if (sqlite3_column_type(statement, 0) != SQLITE_NULL) {
            const unsigned char* text = sqlite3_column_text(statement, 0);
            if (text != nullptr) {
                url = reinterpret_cast<const char*>(text);
            }
        }
    }
    sqlite3_finalize(statement);
    if (url && url->empty()) {
        url.reset();
    }
    return url;
}

std::expected<void, std::string> set_firebase_url(std::string_view url) {
    auto database = open_database();
    if (!database) {
        return std::unexpected(database.error());
    }

    const std::string trimmed = trim_copy(url);
    sqlite3_stmt* statement = nullptr;
    constexpr const char* sql = "UPDATE \"_config\" SET firebase_url = ?;";
    if (sqlite3_prepare_v2(database->get(), sql, -1, &statement, nullptr) !=
        SQLITE_OK) {
        sqlite3_finalize(statement);
        return std::unexpected(
            sqlite_error(database->get(), "Unable to update Firebase URL")
        );
    }
    if (trimmed.empty()) {
        sqlite3_bind_null(statement, 1);
    }
    else {
        sqlite3_bind_text(
            statement,
            1,
            trimmed.data(),
            static_cast<int>(trimmed.size()),
            SQLITE_TRANSIENT
        );
    }
    if (sqlite3_step(statement) != SQLITE_DONE) {
        sqlite3_finalize(statement);
        return std::unexpected(
            sqlite_error(database->get(), "Unable to update Firebase URL")
        );
    }
    sqlite3_finalize(statement);
    return {};
}

} // namespace journal_database
