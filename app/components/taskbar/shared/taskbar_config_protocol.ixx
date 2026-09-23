/**
 * \file taskbar_config_protocol.ixx
 * \brief On-demand discovery protocol for taskbar_config.exe.
 */
export module taskbar_config_protocol;

import std;

export namespace ac::protocol::taskbar_config {
    inline constexpr std::wstring_view pipe_name =
        L"ac_taskbar_config_pipe";
    /** Reply payload when discovery succeeds. */
    inline constexpr std::string_view success = "ok";

    enum class Request : std::int32_t {
        discover_all = 0
    };

    [[nodiscard]] constexpr std::int32_t to_wire(
        const Request request
    ) noexcept {
        return static_cast<std::int32_t>(request);
    }
}
