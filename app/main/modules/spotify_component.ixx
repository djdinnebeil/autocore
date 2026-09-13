/**
 * \file spotify_component.ixx
 * \brief Owns the main-process side of the Spotify component connection.
 */
export module auto_core.main.components.spotify;

export import spotify_protocol;
export import command_registry;
import std;

export namespace spotify_component::runtime_commands {
    /** Registers names from `spotify.keymap_commands.txt`, or the protocol `all` list. */
    void register_with(command_registry::Registry& registry);
}

export {
    /** Creates `ac_spotify_pipe`. There is no ready-wait. */
    void create_spotify_pipe();
    /** Starts `spotify_ac.exe`. */
    void start_spotify_component();
    /** Sends protocol shutdown. Serialized with named invokes. */
    void send_spotify_end_signal();
}
