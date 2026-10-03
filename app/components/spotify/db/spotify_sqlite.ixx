/**
 * \file spotify_sqlite.ixx
 * \brief Private SQLite store for `history.db`.
 *
 * Compiled only into `spotify_db.exe`.
 */
export module spotify_sqlite;

import std;

export namespace spotify::sqlite {

/** One row from `track_history`. */
struct TrackHistory {
    int id {};
    std::string name;
    std::string artist;
    std::string album;
    int duration {};
    int playcount {};
    std::string created_at;
    std::string last_played;
};

class Store {
public:
    Store(Store&&) noexcept;
    Store& operator=(Store&&) noexcept;
    ~Store();

    Store(const Store&) = delete;
    Store& operator=(const Store&) = delete;

    [[nodiscard]] std::expected<void, std::string> record_play(
        std::string_view name,
        std::string_view artist,
        std::string_view album,
        int duration_seconds
    );

    [[nodiscard]] std::expected<std::vector<TrackHistory>, std::string> select_all();

    struct Impl;
    explicit Store(std::unique_ptr<Impl> impl);

private:
    std::unique_ptr<Impl> impl_;
};

[[nodiscard]] std::filesystem::path file_path();
[[nodiscard]] std::expected<Store, std::string> open();

} // namespace spotify::sqlite
