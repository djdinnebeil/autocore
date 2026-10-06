module;

#include <sqlite3.h>
#include <Windows.h>
#include "../runtime/itunes_config_detail.hpp"

module itunes_sqlite;

import std;
import auto_core.core.ini;
import auto_core.core.paths;

namespace {

constexpr int busy_timeout_ms = 3000;

std::string sqlite_error(sqlite3* database, std::string_view context) {
    const char* message = database != nullptr ? sqlite3_errmsg(database) : "unknown SQLite error";
    return std::format("{}: {}", context, message != nullptr ? message : "unknown SQLite error");
}

std::string local_timestamp() {
    SYSTEMTIME time {};
    GetLocalTime(&time);
    return std::format(
        "{:04}-{:02}-{:02} {:02}:{:02}:{:02}",
        time.wYear,
        time.wMonth,
        time.wDay,
        time.wHour,
        time.wMinute,
        time.wSecond
    );
}

std::expected<void, std::string> exec_sql(sqlite3* database, const char* sql, std::string_view context) {
    char* error_text = nullptr;
    if (sqlite3_exec(database, sql, nullptr, nullptr, &error_text) != SQLITE_OK) {
        std::string message = error_text != nullptr ? error_text : "unknown SQLite error";
        sqlite3_free(error_text);
        return std::unexpected(std::format("{}: {}", context, message));
    }
    return {};
}

std::expected<void, std::string> ensure_schema(sqlite3* database) {
    if (const auto foreign_keys = exec_sql(database, "PRAGMA foreign_keys = ON;", "Unable to enable foreign keys");
        !foreign_keys) {
        return foreign_keys;
    }
    if (const auto tracks = exec_sql(
            database,
            "CREATE TABLE IF NOT EXISTS tracks ("
            "track_id INTEGER PRIMARY KEY, "
            "title TEXT NOT NULL, "
            "artist TEXT NOT NULL, "
            "album TEXT NOT NULL, "
            "duration_seconds INTEGER NOT NULL, "
            "first_seen_at TEXT NOT NULL, "
            "last_seen_at TEXT NOT NULL);",
            "Unable to initialize history.db"
        );
        !tracks) {
        return tracks;
    }
    if (const auto history = exec_sql(
            database,
            "CREATE TABLE IF NOT EXISTS listening_history ("
            "id INTEGER PRIMARY KEY, "
            "track_id INTEGER NOT NULL REFERENCES tracks(track_id), "
            "started_at TEXT NOT NULL, "
            "last_observed_at TEXT NOT NULL, "
            "ended_at TEXT, "
            "listened_seconds INTEGER NOT NULL);",
            "Unable to initialize history.db"
        );
        !history) {
        return history;
    }

    sqlite3_stmt* version = nullptr;
    if (sqlite3_prepare_v2(database, "PRAGMA user_version;", -1, &version, nullptr) != SQLITE_OK) {
        return std::unexpected(sqlite_error(database, "Unable to read history.db version"));
    }
    const int step = sqlite3_step(version);
    const int user_version = step == SQLITE_ROW ? sqlite3_column_int(version, 0) : -1;
    sqlite3_finalize(version);
    if (step != SQLITE_ROW) {
        return std::unexpected(sqlite_error(database, "Unable to read history.db version"));
    }
    if (user_version == 0) {
        if (const auto stamped = exec_sql(database, "PRAGMA user_version = 1;", "Unable to stamp history.db");
            !stamped) {
            return stamped;
        }
    }
    return {};
}

std::string column_text(sqlite3_stmt* statement, int index) {
    if (sqlite3_column_type(statement, index) == SQLITE_NULL) {
        return {};
    }
    const unsigned char* text = sqlite3_column_text(statement, index);
    return text != nullptr ? reinterpret_cast<const char*>(text) : std::string {};
}

void bind_text(sqlite3_stmt* statement, int index, std::string_view value) {
    sqlite3_bind_text(
        statement,
        index,
        value.data(),
        static_cast<int>(value.size()),
        SQLITE_TRANSIENT
    );
}

class Statement {
public:
    sqlite3_stmt* handle = nullptr;

    Statement() = default;
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    ~Statement() {
        sqlite3_finalize(handle);
    }
};

class Transaction {
public:
    explicit Transaction(sqlite3* database)
        : database_ {database} {
        begun_ = sqlite3_exec(database_, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr) == SQLITE_OK;
    }

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

    ~Transaction() {
        if (begun_) {
            sqlite3_exec(database_, "ROLLBACK;", nullptr, nullptr, nullptr);
        }
    }

    [[nodiscard]] bool begun() const noexcept {
        return begun_;
    }

    [[nodiscard]] bool commit() {
        if (!begun_) {
            return false;
        }
        if (sqlite3_exec(database_, "COMMIT;", nullptr, nullptr, nullptr) != SQLITE_OK) {
            return false;
        }
        begun_ = false;
        return true;
    }

private:
    sqlite3* database_ = nullptr;
    bool begun_ = false;
};

std::expected<void, std::string> seal_leftover_rows(sqlite3* database) {
    return exec_sql(
        database,
        "UPDATE listening_history SET ended_at = last_observed_at WHERE ended_at IS NULL;",
        "Unable to seal an open listening-history row"
    );
}

} // namespace

namespace itunes::sqlite {

struct Store::Impl {
    sqlite3* handle = nullptr;
    std::optional<std::int64_t> open_history_id;
    std::optional<std::int64_t> open_track_id;
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
    const auto ini_path = ac::paths::config_directory() / "itunes.ini";
    const auto document = ac::ini::read(ini_path);
    std::optional<std::string_view> stored;
    if (document) {
        stored = document->find("itunes", "directory");
    }
    return itunes::config::detail::resolve_directory(
               stored,
               ac::paths::installation_root()
           ) /
           "history.db";
}

std::expected<Store, std::string> open() {
    const auto path = file_path();
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) {
        return std::unexpected(std::format(
            "Unable to create {}: {}",
            path.parent_path().string(),
            error.message()
        ));
    }

    sqlite3* database = nullptr;
    if (sqlite3_open16(path.c_str(), &database) != SQLITE_OK) {
        const std::string message = sqlite_error(database, "Unable to open history.db");
        if (database != nullptr) {
            sqlite3_close(database);
        }
        return std::unexpected(message);
    }
    sqlite3_busy_timeout(database, busy_timeout_ms);
    if (const auto schema = ensure_schema(database); !schema) {
        sqlite3_close(database);
        return std::unexpected(schema.error());
    }
    if (const auto sealed = seal_leftover_rows(database); !sealed) {
        sqlite3_close(database);
        return std::unexpected(sealed.error());
    }

    auto impl = std::make_unique<Store::Impl>();
    impl->handle = database;
    return Store {std::move(impl)};
}

std::expected<void, std::string> Store::observe(const itunes::db::Observation& observation) {
    if (observation.credit_seconds < 0) {
        return std::unexpected("Listening credit must be zero or greater.");
    }
    if (observation.observed_at.empty()) {
        return std::unexpected("Observation time is missing.");
    }

    Transaction transaction {impl_->handle};
    if (!transaction.begun()) {
        return std::unexpected(sqlite_error(impl_->handle, "Unable to record a listening observation"));
    }

    {
        Statement statement;
        const char* sql =
            "INSERT INTO tracks "
            "(track_id, title, artist, album, duration_seconds, first_seen_at, last_seen_at) "
            "VALUES (?, ?, ?, ?, ?, ?, ?) "
            "ON CONFLICT(track_id) DO UPDATE SET "
            "title = excluded.title, "
            "artist = excluded.artist, "
            "album = excluded.album, "
            "duration_seconds = excluded.duration_seconds, "
            "last_seen_at = excluded.last_seen_at;";
        if (sqlite3_prepare_v2(impl_->handle, sql, -1, &statement.handle, nullptr) != SQLITE_OK) {
            return std::unexpected(sqlite_error(impl_->handle, "Unable to record a listening observation"));
        }
        sqlite3_bind_int64(statement.handle, 1, observation.track_id);
        bind_text(statement.handle, 2, observation.title);
        bind_text(statement.handle, 3, observation.artist);
        bind_text(statement.handle, 4, observation.album);
        sqlite3_bind_int(statement.handle, 5, observation.duration_seconds);
        bind_text(statement.handle, 6, observation.observed_at);
        bind_text(statement.handle, 7, observation.observed_at);
        if (sqlite3_step(statement.handle) != SQLITE_DONE) {
            return std::unexpected(sqlite_error(impl_->handle, "Unable to record a listening observation"));
        }
    }

    const bool same_track =
        impl_->open_history_id.has_value() &&
        impl_->open_track_id == observation.track_id;
    std::optional<std::int64_t> open_history_id = impl_->open_history_id;
    std::optional<std::int64_t> open_track_id = impl_->open_track_id;

    if (impl_->open_history_id.has_value() && !same_track) {
        Statement statement;
        const char* sql =
            "UPDATE listening_history SET ended_at = ? WHERE id = ? AND ended_at IS NULL;";
        if (sqlite3_prepare_v2(impl_->handle, sql, -1, &statement.handle, nullptr) != SQLITE_OK) {
            return std::unexpected(sqlite_error(impl_->handle, "Unable to record a listening observation"));
        }
        bind_text(statement.handle, 1, observation.observed_at);
        sqlite3_bind_int64(statement.handle, 2, *impl_->open_history_id);
        if (sqlite3_step(statement.handle) != SQLITE_DONE) {
            return std::unexpected(sqlite_error(impl_->handle, "Unable to record a listening observation"));
        }
        open_history_id.reset();
        open_track_id.reset();
    }

    if (!open_history_id.has_value()) {
        Statement statement;
        const char* sql =
            "INSERT INTO listening_history "
            "(track_id, started_at, last_observed_at, ended_at, listened_seconds) "
            "VALUES (?, ?, ?, NULL, ?);";
        if (sqlite3_prepare_v2(impl_->handle, sql, -1, &statement.handle, nullptr) != SQLITE_OK) {
            return std::unexpected(sqlite_error(impl_->handle, "Unable to record a listening observation"));
        }
        sqlite3_bind_int64(statement.handle, 1, observation.track_id);
        bind_text(statement.handle, 2, observation.observed_at);
        bind_text(statement.handle, 3, observation.observed_at);
        sqlite3_bind_int(statement.handle, 4, observation.credit_seconds);
        if (sqlite3_step(statement.handle) != SQLITE_DONE) {
            return std::unexpected(sqlite_error(impl_->handle, "Unable to record a listening observation"));
        }
        open_history_id = sqlite3_last_insert_rowid(impl_->handle);
        open_track_id = observation.track_id;
    }
    else {
        Statement statement;
        const char* sql =
            "UPDATE listening_history "
            "SET listened_seconds = listened_seconds + ?, last_observed_at = ? "
            "WHERE id = ?;";
        if (sqlite3_prepare_v2(impl_->handle, sql, -1, &statement.handle, nullptr) != SQLITE_OK) {
            return std::unexpected(sqlite_error(impl_->handle, "Unable to record a listening observation"));
        }
        sqlite3_bind_int(statement.handle, 1, observation.credit_seconds);
        bind_text(statement.handle, 2, observation.observed_at);
        sqlite3_bind_int64(statement.handle, 3, *open_history_id);
        if (sqlite3_step(statement.handle) != SQLITE_DONE) {
            return std::unexpected(sqlite_error(impl_->handle, "Unable to record a listening observation"));
        }
    }

    if (!transaction.commit()) {
        return std::unexpected(sqlite_error(impl_->handle, "Unable to record a listening observation"));
    }
    impl_->open_history_id = open_history_id;
    impl_->open_track_id = open_track_id;
    return {};
}

std::expected<void, std::string> Store::close_current(std::string_view ended_at) {
    if (!impl_->open_history_id.has_value()) {
        return {};
    }
    Statement statement;
    const char* sql =
        "UPDATE listening_history SET ended_at = ? WHERE id = ? AND ended_at IS NULL;";
    if (sqlite3_prepare_v2(impl_->handle, sql, -1, &statement.handle, nullptr) != SQLITE_OK) {
        return std::unexpected(sqlite_error(impl_->handle, "Unable to close the listening-history row"));
    }
    bind_text(statement.handle, 1, ended_at);
    sqlite3_bind_int64(statement.handle, 2, *impl_->open_history_id);
    if (sqlite3_step(statement.handle) != SQLITE_DONE) {
        return std::unexpected(sqlite_error(impl_->handle, "Unable to close the listening-history row"));
    }
    impl_->open_history_id.reset();
    impl_->open_track_id.reset();
    return {};
}

std::expected<std::vector<HistoryRow>, std::string> Store::select_all() {
    Statement statement;
    constexpr const char* sql =
        "SELECT h.id, h.track_id, t.title, t.artist, t.album, t.duration_seconds, "
        "h.started_at, h.last_observed_at, h.ended_at, h.listened_seconds "
        "FROM listening_history h "
        "JOIN tracks t ON t.track_id = h.track_id "
        "ORDER BY h.started_at, h.id;";
    if (sqlite3_prepare_v2(impl_->handle, sql, -1, &statement.handle, nullptr) != SQLITE_OK) {
        return std::unexpected(sqlite_error(impl_->handle, "Unable to read history.db"));
    }
    std::vector<HistoryRow> rows;
    while (true) {
        const int step = sqlite3_step(statement.handle);
        if (step == SQLITE_DONE) {
            break;
        }
        if (step != SQLITE_ROW) {
            return std::unexpected(sqlite_error(impl_->handle, "Unable to read history.db"));
        }
        HistoryRow row;
        row.id = sqlite3_column_int64(statement.handle, 0);
        row.track_id = sqlite3_column_int64(statement.handle, 1);
        row.title = column_text(statement.handle, 2);
        row.artist = column_text(statement.handle, 3);
        row.album = column_text(statement.handle, 4);
        row.duration_seconds = sqlite3_column_int(statement.handle, 5);
        row.started_at = column_text(statement.handle, 6);
        row.last_observed_at = column_text(statement.handle, 7);
        row.ended_at = column_text(statement.handle, 8);
        row.listened_seconds = sqlite3_column_int(statement.handle, 9);
        rows.push_back(std::move(row));
    }
    return rows;
}

} // namespace itunes::sqlite
