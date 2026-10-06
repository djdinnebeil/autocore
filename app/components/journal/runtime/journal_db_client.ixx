/**
 * \file journal_db_client.ixx
 * \brief `journal_ac.exe` process owner for `journal_db.exe --serve`.
 */
export module journal_db_client;

import std;
import journal_db_protocol;

export namespace journal::db {

[[nodiscard]] std::expected<void, std::string> start_service();

[[nodiscard]] std::expected<Episode, std::string> allocate_episode(
    std::string_view series_key
);

void shutdown_service();

} // namespace journal::db
