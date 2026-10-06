/**
 * \file itunes_db_client.ixx
 * \brief `itunes_ac.exe` client for `itunes_db.exe --serve`.
 */
export module itunes_db_client;

import std;
export import itunes_db_protocol;

export namespace itunes::db {

[[nodiscard]] std::expected<void, std::string> start_service();

[[nodiscard]] std::expected<void, std::string> observe(const Observation& observation);

void shutdown_service();

} // namespace itunes::db
