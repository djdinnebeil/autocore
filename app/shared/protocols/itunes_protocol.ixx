export module itunes_protocol;

import std;

export namespace ac::protocol::itunes {

    inline constexpr std::wstring_view pipe_name = L"ac_itunes_pipe";

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
    };

    [[nodiscard]]
    constexpr std::int32_t to_wire(Command command) noexcept {
        return static_cast<std::int32_t>(command);
    }

}
