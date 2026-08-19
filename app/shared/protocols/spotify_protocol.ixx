export module spotify_protocol;

import std;

export namespace ac::protocol::spotify {

    inline constexpr std::wstring_view pipe_name = L"ac_sp_pipe";
    inline constexpr std::string_view manifest_filename = "spotify.keymap_commands.txt";

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

    struct CommandName { std::string_view name; };

}

export namespace sp::commands {
    inline constexpr ac::protocol::spotify::CommandName get_queue {"sp_get_queue"};
    inline constexpr ac::protocol::spotify::CommandName print_songs {"sp_print_songs"};
    inline constexpr ac::protocol::spotify::CommandName play_pause {"sp_play_pause"};
    inline constexpr ac::protocol::spotify::CommandName next_song {"sp_next_song"};
    inline constexpr ac::protocol::spotify::CommandName switch_player {"sp_switch_player"};

    inline constexpr std::array all {
        get_queue, print_songs, play_pause, next_song, switch_player,
    };
}
