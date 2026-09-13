/**
 * \file slash_protocol.ixx
 * \brief Shared Slash component protocol.
 *
 * Slash is launched per command; there is no long-lived control pipe.
 */
export module slash_protocol;

import std;

export namespace ac::protocol::slash {

inline constexpr std::string_view manifest_filename =
    "slash.keymap_commands.txt";

struct CommandName {
    std::string_view name;
};

} // namespace ac::protocol::slash

export namespace slash::commands {

inline constexpr ac::protocol::slash::CommandName
report_and_empty_recycle_bin {
    "report_and_empty_recycle_bin"
};

inline constexpr std::array all {
    report_and_empty_recycle_bin,
};

} // namespace slash::commands
