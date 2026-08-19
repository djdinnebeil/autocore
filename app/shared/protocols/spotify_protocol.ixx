export module spotify_protocol;

import std;

export namespace ac::protocol::spotify {

    inline constexpr std::wstring_view pipe_name = L"ac_sp_pipe";

    enum class Command : std::int32_t {
        shutdown = 0,
        play_pause = 1,
        next_song = 2,
        print_songs = 3,
        get_queue = 4,
        update_component = 5,
        switch_player = 6,
        download_album_cover = 8,
    };

    [[nodiscard]]
    constexpr std::int32_t to_wire(Command command) noexcept {
        return static_cast<std::int32_t>(command);
    }

}
