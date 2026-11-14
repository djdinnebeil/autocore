module;

#include <sqlite3.h>
#include <Windows.h>

module journal_sqlite;

import std;
import journal_data_directory;

namespace {

constexpr int busy_timeout_ms = 3000;
constexpr int schema_version = 2;
constexpr int maximum_padding = 16;
constexpr std::string_view no_series_message =
    "No journal series yet. Add one with journal_ac.exe --series.";

struct ResolvedSeries {
    int id {};
    std::string name;
    int counter {};
    int padding {};
};

std::string utf8_path(const std::filesystem::path& path) {
    const auto u8 = path.u8string();
    return {u8.begin(), u8.end()};
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

bool is_reserved_name(std::string_view name) {
    if (ascii_iequals(name, "_config")) {
        return true;
    }
    return name.size() >= 7 && ascii_iequals(name.substr(0, 7), "sqlite_");
}

bool is_valid_series_name(std::string_view name) {
    if (name.empty() || is_reserved_name(name)) {
        return false;
    }
    for (const unsigned char character : name) {
        if (character < 32 || character == 127) {
            return false;
        }
    }
    return true;
}

bool is_busy(sqlite3* database) {
    return database != nullptr && sqlite3_errcode(database) == SQLITE_BUSY;
}

std::string sqlite_error(sqlite3* database, std::string_view prefix) {
    if (is_busy(database)) {
        return "The journal database is busy.";
    }
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
    if (result == SQLITE_BUSY || is_busy(database)) {
        error = "The journal database is busy.";
    }
    else {
        error = std::format(
            "SQL error: {}",
            error_message != nullptr ? error_message : sqlite3_errmsg(database)
        );
    }
    sqlite3_free(error_message);
    return false;
}

using Database = std::unique_ptr<sqlite3, decltype(&sqlite3_close)>;

std::expected<Database, std::string> connect_database(
    const std::filesystem::path& path,
    int flags
) {
    sqlite3* handle = nullptr;
    const std::string utf8 = utf8_path(path);
    const int open_result = sqlite3_open_v2(utf8.c_str(), &handle, flags, nullptr);
    Database database {handle, &sqlite3_close};
    if (open_result != SQLITE_OK) {
        return std::unexpected(sqlite_error(handle, "Unable to open series.db"));
    }
    sqlite3_busy_timeout(database.get(), busy_timeout_ms);
    return database;
}

std::expected<void, std::string> begin_immediate(sqlite3* database) {
    std::string error;
    if (!execute_sql(database, "BEGIN IMMEDIATE;", error)) {
        return std::unexpected(error);
    }
    return {};
}

class ImmediateTransaction {
public:
    explicit ImmediateTransaction(sqlite3* database) noexcept
        : database_ {database} {}

    ImmediateTransaction(const ImmediateTransaction&) = delete;
    ImmediateTransaction& operator=(const ImmediateTransaction&) = delete;

    ~ImmediateTransaction() noexcept {
        if (database_ != nullptr) {
            sqlite3_exec(database_, "ROLLBACK;", nullptr, nullptr, nullptr);
        }
    }

    [[nodiscard]] std::expected<void, std::string> commit() {
        std::string error;
        if (!execute_sql(database_, "COMMIT;", error)) {
            return std::unexpected(error);
        }
        database_ = nullptr;
        return {};
    }

private:
    sqlite3* database_;
};

class Statement {
public:
    explicit Statement(sqlite3_stmt* handle) noexcept
        : handle_ {handle} {}

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    ~Statement() noexcept {
        sqlite3_finalize(handle_);
    }

    [[nodiscard]] sqlite3_stmt* get() const noexcept {
        return handle_;
    }

private:
    sqlite3_stmt* handle_;
};

std::expected<int, std::string> read_user_version(sqlite3* database) {
    sqlite3_stmt* prepared = nullptr;
    constexpr const char* sql = "PRAGMA user_version;";
    if (sqlite3_prepare_v2(database, sql, -1, &prepared, nullptr) != SQLITE_OK) {
        sqlite3_finalize(prepared);
        return std::unexpected(sqlite_error(database, "Unable to read journal schema version"));
    }
    Statement statement {prepared};
    if (sqlite3_step(statement.get()) != SQLITE_ROW) {
        return std::unexpected(sqlite_error(database, "Unable to read journal schema version"));
    }
    return sqlite3_column_int(statement.get(), 0);
}

std::expected<bool, std::string> series_has_padding(sqlite3* database) {
    sqlite3_stmt* prepared = nullptr;
    constexpr const char* sql = "PRAGMA table_info(series);";
    if (sqlite3_prepare_v2(database, sql, -1, &prepared, nullptr) != SQLITE_OK) {
        sqlite3_finalize(prepared);
        return std::unexpected(sqlite_error(database, "Unable to read journal schema"));
    }
    Statement statement {prepared};
    bool found = false;
    while (true) {
        const int step = sqlite3_step(statement.get());
        if (step == SQLITE_DONE) {
            break;
        }
        if (step != SQLITE_ROW) {
            return std::unexpected(sqlite_error(database, "Unable to read journal schema"));
        }
        const unsigned char* name_text = sqlite3_column_text(statement.get(), 1);
        if (name_text != nullptr &&
            std::string_view {reinterpret_cast<const char*>(name_text)} == "padding") {
            found = true;
        }
    }
    return found;
}

std::expected<void, std::string> ensure_schema(sqlite3* database) {
    const auto version = read_user_version(database);
    if (!version) {
        return std::unexpected(version.error());
    }
    if (*version > schema_version) {
        return std::unexpected(
            std::format("Unsupported series.db schema version {}.", *version)
        );
    }
    std::string error;
    if (!execute_sql(
            database,
            "CREATE TABLE IF NOT EXISTS series ("
            "id INTEGER PRIMARY KEY, "
            "name TEXT NOT NULL COLLATE NOCASE UNIQUE, "
            "next_episode INTEGER NOT NULL, "
            "padding INTEGER NOT NULL DEFAULT 0);",
            error
        )) {
        return std::unexpected(error);
    }
    if (*version < schema_version) {
        const auto padding = series_has_padding(database);
        if (!padding) {
            return std::unexpected(padding.error());
        }
        if (!*padding) {
            if (!execute_sql(
                    database,
                    "ALTER TABLE series ADD COLUMN padding INTEGER NOT NULL DEFAULT 0;",
                    error
                )) {
                return std::unexpected(error);
            }
        }
        if (!execute_sql(database, "PRAGMA user_version = 2;", error)) {
            return std::unexpected(error);
        }
    }
    return {};
}

std::expected<ResolvedSeries, std::string> resolve_series(
    sqlite3* database,
    std::string_view series_key
) {
    const std::string key = trim_copy(series_key);
    sqlite3_stmt* prepared = nullptr;
    const char* sql = key.empty()
        ? "SELECT id, name, next_episode, padding FROM series ORDER BY id DESC LIMIT 1;"
        : "SELECT id, name, next_episode, padding FROM series WHERE name = ? LIMIT 1;";
    if (sqlite3_prepare_v2(database, sql, -1, &prepared, nullptr) != SQLITE_OK) {
        sqlite3_finalize(prepared);
        return std::unexpected(sqlite_error(database, "Unable to look up journal series"));
    }
    Statement statement {prepared};
    if (!key.empty()) {
        if (is_reserved_name(key)) {
            return std::unexpected(std::format("Invalid journal series '{}'.", key));
        }
        sqlite3_bind_text(
            statement.get(),
            1,
            key.data(),
            static_cast<int>(key.size()),
            SQLITE_TRANSIENT
        );
    }
    const int step = sqlite3_step(statement.get());
    if (step == SQLITE_BUSY) {
        return std::unexpected("The journal database is busy.");
    }
    if (step != SQLITE_ROW) {
        if (key.empty()) {
            return std::unexpected(std::string {no_series_message});
        }
        return std::unexpected(std::format("Unknown journal series '{}'.", key));
    }
    ResolvedSeries resolved;
    resolved.id = sqlite3_column_int(statement.get(), 0);
    const unsigned char* name_text = sqlite3_column_text(statement.get(), 1);
    resolved.name = name_text != nullptr
        ? reinterpret_cast<const char*>(name_text)
        : std::string {};
    resolved.counter = sqlite3_column_int(statement.get(), 2);
    resolved.padding = sqlite3_column_int(statement.get(), 3);
    if (resolved.name.empty()) {
        return std::unexpected(
            key.empty()
                ? std::string {no_series_message}
                : std::format("Unknown journal series '{}'.", key)
        );
    }
    return resolved;
}

} // namespace

namespace journal::sqlite {

struct Store::Impl {
    sqlite3* handle = nullptr;
};

Store::Store(std::unique_ptr<Impl> impl)
    : impl_ {std::move(impl)} {}

Store::Store(Store&&) noexcept = default;
Store& Store::operator=(Store&&) noexcept = default;

Store::~Store() {
    if (impl_ && impl_->handle != nullptr) {
        sqlite3_close(impl_->handle);
        impl_->handle = nullptr;
    }
}

std::filesystem::path file_path() {
    return journal::data_directory() / "series.db";
}

std::expected<Store, std::string> open_at(const std::filesystem::path& path) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) {
        return std::unexpected(
            std::format(
                "Unable to create {}: {}",
                path.parent_path().string(),
                error.message()
            )
        );
    }

    auto database = connect_database(
        path,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE
    );
    if (!database) {
        return std::unexpected(database.error());
    }
    auto schema = ensure_schema(database->get());
    if (!schema) {
        return std::unexpected(schema.error());
    }

    auto impl = std::make_unique<Store::Impl>();
    impl->handle = database->release();
    return Store {std::move(impl)};
}

std::expected<Store, std::string> open() {
    return open_at(file_path());
}

std::expected<void, std::string> create_initial(
    const std::string_view name,
    const int padding
) {
    const auto path = file_path();
    const auto temporary = std::filesystem::path {path.wstring() + L".new"};
    const auto remove_quiet = [](const std::filesystem::path& target) {
        std::error_code error;
        std::filesystem::remove(target, error);
    };
    remove_quiet(temporary);

    std::expected<void, std::string> added;
    {
        auto opened = open_at(temporary);
        if (!opened) {
            remove_quiet(temporary);
            return std::unexpected(opened.error());
        }
        Store store = std::move(*opened);
        added = store.add_series(name, padding);
    }
    if (!added) {
        remove_quiet(temporary);
        remove_quiet(std::filesystem::path {temporary.wstring() + L"-journal"});
        remove_quiet(std::filesystem::path {temporary.wstring() + L"-wal"});
        remove_quiet(std::filesystem::path {temporary.wstring() + L"-shm"});
        return std::unexpected(added.error());
    }
    if (!MoveFileExW(
            temporary.c_str(),
            path.c_str(),
            MOVEFILE_WRITE_THROUGH
        )) {
        remove_quiet(temporary);
        return std::unexpected(std::format("Unable to create {}.", path.string()));
    }
    return {};
}

std::expected<std::vector<Series>, std::string> Store::list_series() {
    sqlite3_stmt* prepared = nullptr;
    constexpr const char* sql =
        "SELECT name, next_episode, padding FROM series ORDER BY name COLLATE NOCASE;";
    if (sqlite3_prepare_v2(impl_->handle, sql, -1, &prepared, nullptr) !=
        SQLITE_OK) {
        sqlite3_finalize(prepared);
        return std::unexpected(
            sqlite_error(impl_->handle, "Unable to list journal series")
        );
    }
    Statement statement {prepared};
    std::vector<Series> series;
    while (true) {
        const int step = sqlite3_step(statement.get());
        if (step == SQLITE_DONE) {
            break;
        }
        if (step != SQLITE_ROW) {
            return std::unexpected(
                sqlite_error(impl_->handle, "Unable to list journal series")
            );
        }
        Series entry;
        const unsigned char* name_text = sqlite3_column_text(statement.get(), 0);
        entry.name = name_text != nullptr
            ? reinterpret_cast<const char*>(name_text)
            : std::string {};
        entry.next_episode = sqlite3_column_int(statement.get(), 1);
        entry.padding = sqlite3_column_int(statement.get(), 2);
        series.push_back(std::move(entry));
    }
    return series;
}

std::expected<void, std::string> Store::add_series(std::string_view name, const int padding) {
    const std::string trimmed = trim_copy(name);
    if (!is_valid_series_name(trimmed)) {
        return std::unexpected(
            "Series name is empty, reserved, or contains invalid characters."
        );
    }
    if (padding < 0 || padding > maximum_padding) {
        return std::unexpected("Padding must be from 0 through 16.");
    }

    auto began = begin_immediate(impl_->handle);
    if (!began) {
        return std::unexpected(began.error());
    }
    ImmediateTransaction transaction {impl_->handle};
    auto existing = resolve_series(impl_->handle, trimmed);
    if (existing) {
        return std::unexpected(
            std::format("A journal series named '{}' already exists.", trimmed)
        );
    }
    if (existing.error().starts_with("Unknown journal series")) {
        // The name is free. Continue the insert inside this transaction.
    }
    else if (existing.error().starts_with("No journal series yet")) {
        // The table is empty. Continue the insert inside this transaction.
    }
    else {
        return std::unexpected(existing.error());
    }

    {
        sqlite3_stmt* prepared = nullptr;
        constexpr const char* sql =
            "INSERT INTO series (name, next_episode, padding) VALUES (?, 1, ?);";
        if (sqlite3_prepare_v2(impl_->handle, sql, -1, &prepared, nullptr) != SQLITE_OK) {
            sqlite3_finalize(prepared);
            return std::unexpected(sqlite_error(impl_->handle, "Unable to add journal series"));
        }
        Statement statement {prepared};
        sqlite3_bind_text(
            statement.get(),
            1,
            trimmed.data(),
            static_cast<int>(trimmed.size()),
            SQLITE_TRANSIENT
        );
        sqlite3_bind_int(statement.get(), 2, padding);
        if (sqlite3_step(statement.get()) != SQLITE_DONE) {
            const std::string error = sqlite_error(impl_->handle, "Unable to add journal series");
            return std::unexpected(error);
        }
    }
    return transaction.commit();
}

std::expected<Series, std::string> Store::find_series(std::string_view series_key) {
    const auto resolved = resolve_series(impl_->handle, series_key);
    if (!resolved) {
        return std::unexpected(resolved.error());
    }
    return Series {resolved->name, resolved->counter, resolved->padding};
}

std::expected<Series, std::string> Store::set_counter(
    std::string_view series_key,
    int counter
) {
    if (counter < 0) {
        return std::unexpected("Counter must be zero or greater.");
    }
    auto began = begin_immediate(impl_->handle);
    if (!began) {
        return std::unexpected(began.error());
    }
    ImmediateTransaction transaction {impl_->handle};
    const auto resolved = resolve_series(impl_->handle, series_key);
    if (!resolved) {
        return std::unexpected(resolved.error());
    }

    {
        sqlite3_stmt* prepared = nullptr;
        constexpr const char* sql =
            "UPDATE series SET next_episode = ? WHERE id = ?;";
        if (sqlite3_prepare_v2(impl_->handle, sql, -1, &prepared, nullptr) !=
            SQLITE_OK) {
            sqlite3_finalize(prepared);
            return std::unexpected(
                sqlite_error(impl_->handle, "Unable to update journal counter")
            );
        }
        Statement statement {prepared};
        sqlite3_bind_int(statement.get(), 1, counter);
        sqlite3_bind_int(statement.get(), 2, resolved->id);
        if (sqlite3_step(statement.get()) != SQLITE_DONE) {
            const std::string error = sqlite_error(
                impl_->handle,
                "Unable to update journal counter"
            );
            return std::unexpected(error);
        }
    }
    auto committed = transaction.commit();
    if (!committed) {
        return std::unexpected(committed.error());
    }
    return Series {resolved->name, counter, resolved->padding};
}

std::expected<Series, std::string> Store::set_padding(
    std::string_view series_key,
    int padding
) {
    if (padding < 0 || padding > maximum_padding) {
        return std::unexpected("Padding must be from 0 through 16.");
    }
    auto began = begin_immediate(impl_->handle);
    if (!began) {
        return std::unexpected(began.error());
    }
    ImmediateTransaction transaction {impl_->handle};
    const auto resolved = resolve_series(impl_->handle, series_key);
    if (!resolved) {
        return std::unexpected(resolved.error());
    }

    {
        sqlite3_stmt* prepared = nullptr;
        constexpr const char* sql =
            "UPDATE series SET padding = ? WHERE id = ?;";
        if (sqlite3_prepare_v2(impl_->handle, sql, -1, &prepared, nullptr) !=
            SQLITE_OK) {
            sqlite3_finalize(prepared);
            return std::unexpected(
                sqlite_error(impl_->handle, "Unable to update journal padding")
            );
        }
        Statement statement {prepared};
        sqlite3_bind_int(statement.get(), 1, padding);
        sqlite3_bind_int(statement.get(), 2, resolved->id);
        if (sqlite3_step(statement.get()) != SQLITE_DONE) {
            const std::string error = sqlite_error(
                impl_->handle,
                "Unable to update journal padding"
            );
            return std::unexpected(error);
        }
    }
    auto committed = transaction.commit();
    if (!committed) {
        return std::unexpected(committed.error());
    }
    return Series {resolved->name, resolved->counter, padding};
}

std::expected<Episode, std::string> Store::allocate_episode(
    std::string_view series_key
) {
    auto began = begin_immediate(impl_->handle);
    if (!began) {
        return std::unexpected(began.error());
    }
    ImmediateTransaction transaction {impl_->handle};
    const auto resolved = resolve_series(impl_->handle, series_key);
    if (!resolved) {
        return std::unexpected(resolved.error());
    }

    {
        sqlite3_stmt* prepared = nullptr;
        constexpr const char* sql =
            "UPDATE series SET next_episode = ? WHERE id = ? AND next_episode = ?;";
        if (sqlite3_prepare_v2(impl_->handle, sql, -1, &prepared, nullptr) !=
            SQLITE_OK) {
            sqlite3_finalize(prepared);
            return std::unexpected(
                sqlite_error(impl_->handle, "Unable to update journal counter")
            );
        }
        Statement statement {prepared};
        sqlite3_bind_int(statement.get(), 1, resolved->counter + 1);
        sqlite3_bind_int(statement.get(), 2, resolved->id);
        sqlite3_bind_int(statement.get(), 3, resolved->counter);
        if (sqlite3_step(statement.get()) != SQLITE_DONE) {
            const std::string error = sqlite_error(
                impl_->handle,
                "Unable to update journal counter"
            );
            return std::unexpected(error);
        }
    }
    if (sqlite3_changes(impl_->handle) != 1) {
        return std::unexpected("Unable to update journal counter.");
    }

    auto committed = transaction.commit();
    if (!committed) {
        return std::unexpected(committed.error());
    }
    return Episode {
        resolved->name,
        resolved->counter,
        resolved->counter + 1,
        resolved->padding
    };
}

} // namespace journal::sqlite
