/**
 * \file spotify_db_client.ixx
 * \brief `spotify_ac.exe` owner of one persistent `history.db` connection.
 */
export module spotify_db_client;

import std;

export namespace spotify::db {

[[nodiscard]] std::expected<void, std::string> start_service();

[[nodiscard]] std::expected<void, std::string> record_play(
    std::string_view name,
    std::string_view artist,
    std::string_view album,
    int duration_seconds
);

void shutdown_service();

} // namespace spotify::db
