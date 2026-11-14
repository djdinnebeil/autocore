/**
 * \file journal_db_protocol.ixx
 * \brief Shared Journal database helpers that are not SQL.
 *
 * `journal_ac.exe` and `journal_ac.exe --series` open `series.db` through
 * `journal_sqlite`. Unknown-series text stays here so callers can recognize
 * it without depending on the store.
 */
export module journal_db_protocol;

import std;

export namespace journal::db {

inline constexpr std::string_view unknown_series_prefix = "Unknown journal series";

[[nodiscard]] inline bool is_unknown_series(const std::string_view message) {
    return message.starts_with(unknown_series_prefix);
}

} // namespace journal::db
