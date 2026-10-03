/**
 * \file itunes_sqlite.ixx
 * \brief Private SQLite store for iTunes `history.db`.
 *
 * Compiled only into `itunes_db.exe`.
 */
export module itunes_sqlite;

import std;
import itunes_db_protocol;

export namespace itunes::sqlite {

struct HistoryRow {
    std::int64_t id {};
    std::int64_t track_id {};
    std::string title;
    std::string artist;
    std::string album;
    int duration_seconds {};
    std::string started_at;
    std::string last_observed_at;
    std::string ended_at;
    int listened_seconds {};
};

class Store {
public:
    Store(Store&&) noexcept;
    Store& operator=(Store&&) noexcept;
    ~Store();

    Store(const Store&) = delete;
    Store& operator=(const Store&) = delete;

    [[nodiscard]] std::expected<void, std::string> observe(const itunes::db::Observation& observation);

    [[nodiscard]] std::expected<void, std::string> close_current(std::string_view ended_at);

    [[nodiscard]] std::expected<std::vector<HistoryRow>, std::string> select_all();

    struct Impl;
    explicit Store(std::unique_ptr<Impl> impl);

private:
    std::unique_ptr<Impl> impl_;
};

[[nodiscard]] std::filesystem::path file_path();
[[nodiscard]] std::expected<Store, std::string> open();

} // namespace itunes::sqlite
