#include "catch_amalgamated.hpp"

#include <filesystem>
#include <sqlite3.h>

import journal_episode_format;
import journal_series_map;
import journal_sqlite;

namespace {

std::filesystem::path fresh_database() {
    const auto directory = std::filesystem::temp_directory_path() / "ac_journal_schema_test";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);
    return directory / "series.db";
}

void exec(sqlite3* database, const char* sql) {
    char* error = nullptr;
    const int result = sqlite3_exec(database, sql, nullptr, nullptr, &error);
    if (result != SQLITE_OK && error != nullptr) {
        FAIL(error);
    }
    sqlite3_free(error);
    REQUIRE(result == SQLITE_OK);
}

int user_version(const std::filesystem::path& path) {
    sqlite3* database = nullptr;
    const std::string utf8 = path.string();
    REQUIRE(sqlite3_open_v2(utf8.c_str(), &database, SQLITE_OPEN_READONLY, nullptr) == SQLITE_OK);
    sqlite3_stmt* statement = nullptr;
    REQUIRE(sqlite3_prepare_v2(database, "PRAGMA user_version;", -1, &statement, nullptr) == SQLITE_OK);
    REQUIRE(sqlite3_step(statement) == SQLITE_ROW);
    const int version = sqlite3_column_int(statement, 0);
    sqlite3_finalize(statement);
    sqlite3_close(database);
    return version;
}

} // namespace

TEST_CASE("series.db v1 migrates in place and keeps padding at 0", "[journal][unit]") {
    const auto path = fresh_database();
    sqlite3* database = nullptr;
    REQUIRE(sqlite3_open(path.string().c_str(), &database) == SQLITE_OK);
    exec(database,
        "CREATE TABLE series ("
        "id INTEGER PRIMARY KEY, "
        "name TEXT NOT NULL COLLATE NOCASE UNIQUE, "
        "next_episode INTEGER NOT NULL);");
    exec(database, "INSERT INTO series (name, next_episode) VALUES ('Personal', 3);");
    exec(database, "INSERT INTO series (name, next_episode) VALUES ('Auto Core', 5);");
    exec(database, "PRAGMA user_version = 1;");
    sqlite3_close(database);

    {
        auto store = journal::sqlite::open_at(path);
        REQUIRE(store.has_value());
        const auto series = store->list_series();
        REQUIRE(series.has_value());
        REQUIRE(series->size() == 2);
        CHECK(series->at(0).name == "Auto Core");
        CHECK(series->at(0).next_episode == 5);
        CHECK(series->at(0).padding == 0);
        CHECK(series->at(1).name == "Personal");
        CHECK(series->at(1).next_episode == 3);
        CHECK(series->at(1).padding == 0);

        CHECK_FALSE(store->add_series("Research", -1).has_value());
        CHECK_FALSE(store->add_series("Research", 17).has_value());
        const auto added = store->add_series("Research", 3);
        REQUIRE(added.has_value());
        const auto research = store->find_series("Research");
        REQUIRE(research.has_value());
        CHECK(research->next_episode == 1);
        CHECK(research->padding == 3);

        const auto latest = store->find_series("");
        REQUIRE(latest.has_value());
        CHECK(latest->name == "Research");
    }

    CHECK(user_version(path) == 2);
}

TEST_CASE("A new series snapshot is rendered from the database row", "[journal][unit]") {
    const auto path = fresh_database();
    auto store = journal::sqlite::open_at(path);
    REQUIRE(store.has_value());
    REQUIRE(store->add_series("Auto Core", 2).has_value());
    const auto series = store->list_series();
    REQUIRE(series.has_value());
    REQUIRE(series->size() == 1);
    const auto& row = series->front();
    CHECK(row.next_episode == 1);
    CHECK(row.padding == 2);

    const journal::series_map::SnapshotSeries snapshot[] {
        {row.name, row.next_episode, row.padding}
    };
    const std::string episode = journal::format_episode_number(
        row.next_episode,
        row.padding
    );
    const std::string rendered = journal::series_map::render(row.name, snapshot);
    CHECK(rendered ==
        "active = " + row.name + "\n\n[snapshot]\n" + row.name + " " + episode + "\n");
    CHECK(journal::series_map::parse_active(rendered).name == row.name);
}
