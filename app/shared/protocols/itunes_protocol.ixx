export module itunes_protocol;

import std;

export namespace ac::protocol::itunes {

    inline constexpr std::wstring_view pipe_name = L"ac_itunes_pipe";
    inline constexpr std::string_view manifest_filename = "itunes.keymap_commands.txt";

    enum class Command : std::int32_t {
        shutdown = 0,
        play_pause = 1,
        next_song = 2,
        print_songs = 3,
        print_next_up = 4,
        update_component = 5,
        previous_song = 6,
        stop_song = 7,
        remove_song = 9,
        invoke_named = 10,
    };

    [[nodiscard]]
    constexpr std::int32_t to_wire(Command command) noexcept {
        return static_cast<std::int32_t>(command);
    }

    struct CommandName { std::string_view name; };

}

export namespace itunes::commands {
    inline constexpr ac::protocol::itunes::CommandName print_songs {"itunes_print_songs"};
    inline constexpr ac::protocol::itunes::CommandName next_song {"itunes_next_song"};
    inline constexpr ac::protocol::itunes::CommandName print_next_up {"itunes_print_next_up"};
    inline constexpr ac::protocol::itunes::CommandName play_pause {"itunes_play_pause"};
    inline constexpr ac::protocol::itunes::CommandName stop_song {"itunes_stop_song"};
    inline constexpr ac::protocol::itunes::CommandName remove_song {"itunes_remove_song"};

    inline constexpr std::array all {
        print_songs, next_song, print_next_up, play_pause, stop_song, remove_song,
    };
}
