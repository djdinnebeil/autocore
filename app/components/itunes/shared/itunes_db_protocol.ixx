/**
 * \file itunes_db_protocol.ixx
 * \brief Listening observation passed to the iTunes history store.
 *
 * Playback position stays in `itunes_ac.exe`. The store receives the
 * credited sample only.
 */
export module itunes_db_protocol;

import std;

export namespace itunes::db {

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
