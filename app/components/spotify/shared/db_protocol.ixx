/**
 * \file db_protocol.ixx
 * \brief Private `ac.spotify.db.v1` contract between `spotify_ac.exe` and
 * `spotify_db.exe`.
 *
 * This is not `ac.component.v1` and `spotify_db.exe` is not a component.
 * SQL and schema stay off the pipe.
 */
export module spotify_db_protocol;

import std;

export namespace spotify::db {

inline constexpr std::string_view protocol_id = "ac.spotify.db.v1";
inline constexpr std::wstring_view pipe_name = L"ac_spotify_db_pipe";

enum class Request : std::int32_t {
    record_play = 1,
    shutdown = 2,
};

[[nodiscard]] constexpr std::int32_t to_wire(Request request) noexcept {
    return static_cast<std::int32_t>(request);
}

struct Play {
    std::string name;
    std::string artist;
    std::string album;
    int duration_seconds {};
};

} // namespace spotify::db
