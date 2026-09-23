/**
 * \file itunes_monitor.ixx
 * \brief Track-change monitor entry for iTunes.
 */
export module itunes_monitor;

/** Advances playback and notifies the monitor thread. */
export void itunes_next_song();
