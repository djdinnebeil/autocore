/**
 * \file devices_protocol.ixx
 * \brief Private request from `spotify_editor.exe` to `spotify_ac.exe`.
 *
 * This is not `ac.component.v1` and it is not a keymap command.
 */
export module spotify_devices_protocol;

import std;

export namespace spotify::devices {

inline constexpr std::string_view protocol_id = "ac.spotify.devices.v1";
inline constexpr std::wstring_view pipe_name = L"ac_spotify_devices_pipe";

enum class Request : std::int32_t {
    list_devices = 1,
};

[[nodiscard]] constexpr std::int32_t to_wire(Request request) noexcept {
    return static_cast<std::int32_t>(request);
}

} // namespace spotify::devices
