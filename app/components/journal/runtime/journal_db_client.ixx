/**
 * \file journal_db_client.ixx
 * \brief `journal_ac.exe` owner of one persistent `series.db` connection.
 */
export module journal_db_client;

import std;
import journal_sqlite;

export namespace journal::db {

[[nodiscard]] std::expected<void, std::string> start_service();

[[nodiscard]] std::expected<journal::sqlite::Episode, std::string> allocate_episode(
    std::string_view series_key
);

void shutdown_service();

} // namespace journal::db
