/**
 * \file spotify_protocol.ixx
 * \brief Pipe contract and keymap names for `spotify_ac.exe`.
 */
export module spotify_protocol;

import std;

export namespace ac::protocol::spotify {

    /** Local pipe name without the `\\.\pipe\` prefix. */
    inline constexpr std::wstring_view pipe_name = L"ac_spotify_pipe";
    inline constexpr std::string_view manifest_filename = "spotify.keymap_commands.txt";

    /**
     * Integer commands sent on the pipe. Enumerator 7 is unused.
     * `download_album_cover` is a pipe command and is not in `spotify::commands::all`.
     */
    enum class Command : std::int32_t {
        shutdown = 0,
        play_pause = 1,
        next_song = 2,
        print_songs = 3,
        get_queue = 4,
        update_component = 5,
        switch_player = 6,
        download_album_cover = 8,
        invoke_named = 9,
    };

    [[nodiscard]]
    constexpr std::int32_t to_wire(Command command) noexcept {
        return static_cast<std::int32_t>(command);
    }

    /** A runtime keymap spelling for a Spotify command. */
    struct CommandName { std::string_view name; };

}

export namespace spotify::commands {
    inline constexpr ac::protocol::spotify::CommandName get_queue {"spotify_get_queue"};
    inline constexpr ac::protocol::spotify::CommandName print_songs {"spotify_print_songs"};
    inline constexpr ac::protocol::spotify::CommandName play_pause {"spotify_play_pause"};
    inline constexpr ac::protocol::spotify::CommandName next_song {"spotify_next_song"};
    inline constexpr ac::protocol::spotify::CommandName switch_player {"spotify_switch_player"};

    /** Keymap-facing commands registered with Main. */
    inline constexpr std::array all {
        get_queue, print_songs, play_pause, next_song, switch_player,
    };
}
