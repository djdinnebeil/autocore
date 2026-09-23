/**
 * \file journal_protocol.ixx
 * \brief Shared journal component protocol and typed commands.
 *
 * Product command tokens are implemented in `journal_ac.exe`. Person-name
 * print-choice aliases live in `journal/journal_choices.ini` under the
 * configured journal data directory and expand only when `keymap.map`
 * names them.
 */
export module journal_protocol;

import std;

export namespace ac::protocol::journal {

inline constexpr std::wstring_view pipe_name = L"ac_journal_pipe";
inline constexpr std::string_view manifest_filename =
    "journal.keymap_commands.txt";
/** Sent by `journal_ac.exe` after it is ready to accept invoke requests. */
inline constexpr std::string_view ready_message = "journal_ready";

enum class Request : std::int32_t {
    invoke = 0,
    shutdown = 1,
};

[[nodiscard]] constexpr std::int32_t to_wire(Request request) noexcept {
    return static_cast<std::int32_t>(request);
}

/** A keymap command name forwarded as an invoke payload. */
struct Command {
    std::string_view name;
};

} // namespace ac::protocol::journal

export inline constexpr ac::protocol::journal::Command print_extended_timestamp {"print_extended_timestamp"};
export inline constexpr ac::protocol::journal::Command print_episode_title {"print_episode_title"};
export inline constexpr ac::protocol::journal::Command save_file_and_create_new_file {"save_file_and_create_new_file"};

export inline constexpr std::array journal_commands {
    print_extended_timestamp,
    print_episode_title, save_file_and_create_new_file,
};
