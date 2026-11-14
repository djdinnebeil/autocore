/**
 * \file spotify_history.cxx
 * \brief Records listening history in the Spotify runtime's open database.
 */
module spotify_client;

import std;
import spotify_component;
import spotify_db_client;

void track_spotify_history(const SongMetadata& meta) {
    spotify_component.log_debug("track_spotify_history() called");
    if (const auto recorded = spotify::db::record_play(
            meta.name,
            meta.artist,
            meta.album,
            meta.duration_seconds
        );
        !recorded) {
        spotify_component.log_debug("{}", recorded.error());
    }
    spotify_component.log_debug("track_spotify_history() finished");
}
