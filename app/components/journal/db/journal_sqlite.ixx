/**
 * \file journal_sqlite.ixx
 * \brief Private SQLite store for `series.db`.
 *
 * Compiled only into `journal_db.exe` and the Journal test binary. Other
 * Journal executables do not import this module.
 */
export module journal_sqlite;

import std;

export namespace journal::sqlite {

/** A series name, its next episode counter, and its display padding. */
struct Series {
    std::string name;
    int next_episode {};
    int padding {};
};

/** The committed allocation: the number just taken, the next count, and padding. */
struct Episode {
    std::string name;
    int allocated {};
    int next_episode {};
    int padding {};
};

/**
 * \brief Owns one SQLite connection to `series.db`.
 *
 * Every connection sets a 3-second busy timeout. `allocate_episode` commits
 * before it returns a number. Schema version 2 adds `padding` in place.
 */
class Store {
public:
    Store(Store&&) noexcept;
    Store& operator=(Store&&) noexcept;
    ~Store();

    Store(const Store&) = delete;
    Store& operator=(const Store&) = delete;

    [[nodiscard]] std::expected<std::vector<Series>, std::string> list_series();
    [[nodiscard]] std::expected<void, std::string> add_series(
        std::string_view name,
        int padding
    );
    [[nodiscard]] std::expected<Series, std::string> find_series(
        std::string_view series_key
    );
    [[nodiscard]] std::expected<Series, std::string> set_counter(
        std::string_view series_key,
        int counter
    );
    [[nodiscard]] std::expected<Series, std::string> set_padding(
        std::string_view series_key,
        int padding
    );
    [[nodiscard]] std::expected<Episode, std::string> allocate_episode(
        std::string_view series_key
    );

    struct Impl;
    explicit Store(std::unique_ptr<Impl> impl);

private:
    std::unique_ptr<Impl> impl_;
};

/** \brief Returns `series.db` under the configured journal data directory. */
[[nodiscard]] std::filesystem::path file_path();

/** \brief Opens or creates `series.db` in the configured journal directory. */
[[nodiscard]] std::expected<Store, std::string> open();

/** \brief Opens or creates a `series.db` at `path`. Used by schema tests. */
[[nodiscard]] std::expected<Store, std::string> open_at(
    const std::filesystem::path& path
);

} // namespace journal::sqlite
