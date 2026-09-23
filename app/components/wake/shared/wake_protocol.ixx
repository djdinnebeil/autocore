/**
 * \file wake_protocol.ixx
 * \brief Pipe contract for `wake_ac.exe`.
 */
export module wake_protocol;

import std;

export namespace ac::protocol::wake {

    /** Local pipe name without the `\\.\pipe\` prefix. */
    inline constexpr std::wstring_view pipe_name = L"wake_pipe";

    enum class Command : std::int32_t {
        shutdown = 0,
        log_last_wake = 1,
        update_component = 2,
    };

    [[nodiscard]]
    constexpr std::int32_t to_wire(Command command) noexcept {
        return static_cast<std::int32_t>(command);
    }

}
