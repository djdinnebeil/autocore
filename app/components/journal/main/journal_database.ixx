/**
 * \file journal_database.ixx
 * \brief Single-file journals.db: one table per series plus optional Firebase URL.
 *
 * Failures are returned as `std::expected` error strings. The database file is
 * `journals.db` under `ac::paths::journal_directory()`.
 */
export module journal_database;

import std;

export namespace journal_database {

/** A named series and its next episode counter. */
struct Series {
    std::string name;
    int counter {};
};

/** The episode name and number allocated by `take_next_episode`. */
struct Episode {
    std::string name;
    int number {};
};

/** \brief Returns `journals.db` under the configured journal data directory. */
[[nodiscard]] std::filesystem::path file_path();

/** \brief The most recently created series table, if any. */
[[nodiscard]] std::expected<Series, std::string> latest_series();

/** \brief Lists series tables in file order. */
[[nodiscard]] std::expected<std::vector<Series>, std::string> list_series();
/** \brief Creates an empty series table named `name`. */
[[nodiscard]] std::expected<void, std::string> add_series(std::string_view name);
[[nodiscard]] std::expected<Series, std::string> find_series(
    std::string_view series_key
);
[[nodiscard]] std::expected<Series, std::string> set_counter(
    std::string_view series_key,
    int counter
);
/**
 * \brief Increments the series counter and returns the new episode.
 *
 * An empty `series_key` uses the most recently created series table.
 * The returned `name` is the formatted episode title stem.
 */
[[nodiscard]] std::expected<Episode, std::string> take_next_episode(
    std::string_view series_key
);
/** \brief Optional Firebase URL stored in the `_config` table. */
[[nodiscard]] std::expected<std::optional<std::string>, std::string>
    firebase_url();
[[nodiscard]] std::expected<void, std::string> set_firebase_url(
    std::string_view url
);

} // namespace journal_database
