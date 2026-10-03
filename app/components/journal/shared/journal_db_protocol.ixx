/**
 * \file journal_db_protocol.ixx
 * \brief Private `ac.journal.db.v2` contract between Journal executables and
 * `journal_db.exe`.
 *
 * This is not `ac.component.v1` and `journal_db.exe` is not a component.
 * SQL and schema stay off the pipe. Each client connects, exchanges one or
 * more commands, and disconnects. The server accepts the next client after
 * that.
 */
export module journal_db_protocol;

import std;

export namespace journal::db {

inline constexpr std::string_view protocol_id = "ac.journal.db.v2";
inline constexpr std::wstring_view pipe_name = L"ac_journal_db_pipe";
inline constexpr std::string_view unknown_series_prefix = "Unknown journal series";

enum class Request : std::int32_t {
    allocate_episode = 1,
    shutdown = 2,
    list_series = 3,
    add_series = 4,
    set_counter = 5,
    set_padding = 6,
    find_series = 7,
};

[[nodiscard]] constexpr std::int32_t to_wire(const Request request) noexcept {
    return static_cast<std::int32_t>(request);
}

[[nodiscard]] inline bool is_unknown_series(const std::string_view message) {
    return message.starts_with(unknown_series_prefix);
}

struct Series {
    std::string name;
    int next_episode {};
    int padding {};
};

/** Episode allocated by `allocate_episode` after the store commits. */
struct Episode {
    std::string name;
    int allocated {};
    int next_episode {};
    int padding {};
};

} // namespace journal::db
