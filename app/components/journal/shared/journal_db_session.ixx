/**
 * \file journal_db_session.ixx
 * \brief Short-lived `ac.journal.db.v2` calls.
 *
 * Each call connects, handshakes, exchanges one command, and disconnects.
 * Process ownership stays with the caller.
 */
export module journal_db_session;

import std;
import journal_db_protocol;

export namespace journal::db::session {

[[nodiscard]] std::expected<void, std::string> probe();
[[nodiscard]] std::expected<Episode, std::string> allocate_episode(
    std::string_view series_key
);
[[nodiscard]] std::expected<std::vector<Series>, std::string> list_series();
[[nodiscard]] std::expected<void, std::string> add_series(
    std::string_view name,
    int padding
);
[[nodiscard]] std::expected<Series, std::string> set_counter(
    std::string_view name,
    int counter
);
[[nodiscard]] std::expected<Series, std::string> set_padding(
    std::string_view name,
    int padding
);
[[nodiscard]] std::expected<Series, std::string> find_series(std::string_view series_key);
[[nodiscard]] std::expected<void, std::string> shutdown();

} // namespace journal::db::session
