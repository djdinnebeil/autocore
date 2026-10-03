/**
 * \file itunes_db_protocol.ixx
 * \brief Private `ac.itunes.db.v1` contract between `itunes_ac.exe` and
 * `itunes_db.exe`.
 *
 * This is not `ac.component.v1` and `itunes_db.exe` is not a component.
 * SQL and schema stay off the pipe. Playback position stays in `itunes_ac.exe`.
 */
export module itunes_db_protocol;

import std;

export namespace itunes::db {

inline constexpr std::string_view protocol_id = "ac.itunes.db.v1";
inline constexpr std::wstring_view pipe_name = L"ac_itunes_db_pipe";

enum class Request : std::int32_t {
    observe = 1,
    shutdown = 2,
};

[[nodiscard]] constexpr std::int32_t to_wire(Request request) noexcept {
    return static_cast<std::int32_t>(request);
}

struct Observation {
    std::int64_t track_id {};
    std::string title;
    std::string artist;
    std::string album;
    int duration_seconds {};
    int credit_seconds {};
    std::string observed_at;
};

} // namespace itunes::db
