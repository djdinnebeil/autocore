/**
 * \file taskbar_protocol.ixx
 * \brief Control protocol between Main and taskbar_ac.exe.
 */
export module taskbar_protocol;

import std;

export namespace ac::protocol::taskbar {

    inline constexpr std::wstring_view pipe_name = L"ac_taskbar_pipe";
    /** Sent by `taskbar_ac.exe` after the authority is ready. */
    inline constexpr std::string_view ready_message = "taskbar_ready";
    inline constexpr std::string_view manifest_filename =
        "taskbar.keymap_commands.txt";

    namespace commands {
        inline constexpr std::string_view activate_auto_core =
            "activate_auto_core";
        inline constexpr std::string_view launch_powershell =
            "launch_powershell";
        inline constexpr std::string_view activate_powershell_in_admin =
            "activate_powershell_in_admin";
        inline constexpr std::string_view activate_wordpad =
            "activate_wordpad";
        inline constexpr std::string_view launch_gitbash = "launch_gitbash";
        inline constexpr std::string_view refresh_taskbar_positions =
            "refresh_taskbar_positions";

        /** Compiled command names that INI handlers must not replace.
            This is a reserved-name list, not a pipe-destination list.
            `activate_auto_core` is invoked on the `taskbar_ac.exe` pipe.
            `launch_powershell` and `launch_gitbash` run on Main.
            `activate_wordpad` and `activate_powershell_in_admin` run on
            Main so unmapped icons can emulate Win+position behavior. */
        inline constexpr std::array authority {
            activate_auto_core,
            launch_powershell,
            activate_powershell_in_admin,
            activate_wordpad,
            launch_gitbash
        };
    }

    enum class Request : std::int32_t {
        invoke_named = 0,
        shutdown = 1
    };

    [[nodiscard]] constexpr std::int32_t to_wire(
        const Request request
    ) noexcept {
        return static_cast<std::int32_t>(request);
    }

} // namespace ac::protocol::taskbar
