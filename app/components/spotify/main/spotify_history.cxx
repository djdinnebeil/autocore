/**
 * \file spotify_history.cxx
 * \brief Sends listening-history updates to `spotify_db.exe`.
 */
module spotify_client;

import std;
import spotify_component;
import spotify_db_client;

void track_spotify_history(const SongMetadata& meta) {
    spotify_component.log("track_spotify_history() called");
    if (const auto recorded = spotify::db::record_play(
            meta.name,
            meta.artist,
            meta.album,
            meta.duration_seconds
        );
        !recorded) {
        spotify_component.log("{}", recorded.error());
    }
    spotify_component.log("track_spotify_history() finished");
}
