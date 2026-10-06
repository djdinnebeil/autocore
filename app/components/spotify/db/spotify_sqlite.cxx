module;

#include <sqlite3.h>
#include <Windows.h>

module spotify_sqlite;

import std;
import spotify_data_directory;

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

std::expected<void, std::string> ensure_schema(sqlite3* database) {
    char* error_text = nullptr;
    const char* sql =
        "CREATE TABLE IF NOT EXISTS track_history ("
        "id INTEGER PRIMARY KEY, "
        "name TEXT NOT NULL, "
        "artist TEXT NOT NULL, "
        "album TEXT NOT NULL, "
        "duration INTEGER NOT NULL, "
        "playcount INTEGER NOT NULL, "
        "created_at TEXT NOT NULL, "
        "last_played TEXT NOT NULL, "
        "UNIQUE(name, artist, album));";
    const int result = sqlite3_exec(database, sql, nullptr, nullptr, &error_text);
    if (result != SQLITE_OK) {
        std::string message = error_text != nullptr ? error_text : "unknown SQLite error";
        sqlite3_free(error_text);
        return std::unexpected(std::format("Unable to initialize history.db: {}", message));
    }
    return {};
}

std::string column_text(sqlite3_stmt* statement, int index) {
    const unsigned char* text = sqlite3_column_text(statement, index);
    return text != nullptr ? reinterpret_cast<const char*>(text) : std::string {};
}

} // namespace

namespace spotify::sqlite {

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
    return spotify::data_directory() / "history.db";
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

    auto impl = std::make_unique<Store::Impl>();
    impl->handle = database;
    return Store {std::move(impl)};
}

std::expected<void, std::string> Store::record_play(
    std::string_view name,
    std::string_view artist,
    std::string_view album,
    int duration_seconds
) {
    const std::string timestamp = local_timestamp();
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO track_history "
        "(name, artist, album, duration, playcount, created_at, last_played) "
        "VALUES (?, ?, ?, ?, 1, ?, ?) "
        "ON CONFLICT(name, artist, album) DO UPDATE SET "
        "playcount = track_history.playcount + 1, "
        "last_played = excluded.last_played;";
    if (sqlite3_prepare_v2(impl_->handle, sql, -1, &statement, nullptr) != SQLITE_OK) {
        return std::unexpected(sqlite_error(impl_->handle, "Unable to record a Spotify play"));
    }
    const auto bind_text = [&](int index, std::string_view value) {
        sqlite3_bind_text(
            statement,
            index,
            value.data(),
            static_cast<int>(value.size()),
            SQLITE_TRANSIENT
        );
    };
    bind_text(1, name);
    bind_text(2, artist);
    bind_text(3, album);
    sqlite3_bind_int(statement, 4, duration_seconds);
    bind_text(5, timestamp);
    bind_text(6, timestamp);
    const int step = sqlite3_step(statement);
    sqlite3_finalize(statement);
    if (step != SQLITE_DONE) {
        return std::unexpected(sqlite_error(impl_->handle, "Unable to record a Spotify play"));
    }
    return {};
}

std::expected<std::vector<TrackHistory>, std::string> Store::select_all() {
    sqlite3_stmt* statement = nullptr;
    constexpr const char* sql = "SELECT * FROM track_history;";
    if (sqlite3_prepare_v2(impl_->handle, sql, -1, &statement, nullptr) != SQLITE_OK) {
        sqlite3_finalize(statement);
        return std::unexpected(sqlite_error(impl_->handle, "Unable to read history.db"));
    }
    std::vector<TrackHistory> rows;
    while (true) {
        const int step = sqlite3_step(statement);
        if (step == SQLITE_DONE) {
            break;
        }
        if (step != SQLITE_ROW) {
            sqlite3_finalize(statement);
            return std::unexpected(sqlite_error(impl_->handle, "Unable to read history.db"));
        }
        TrackHistory row;
        row.id = sqlite3_column_int(statement, 0);
        row.name = column_text(statement, 1);
        row.artist = column_text(statement, 2);
        row.album = column_text(statement, 3);
        row.duration = sqlite3_column_int(statement, 4);
        row.playcount = sqlite3_column_int(statement, 5);
        row.created_at = column_text(statement, 6);
        row.last_played = column_text(statement, 7);
        rows.push_back(std::move(row));
    }
    sqlite3_finalize(statement);
    return rows;
}

} // namespace spotify::sqlite
