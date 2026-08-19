/** \file journal_protocol.ixx Shared journal component protocol and typed commands. */
export module journal_protocol;

import std;

export namespace ac::protocol::journal {

inline constexpr std::wstring_view pipe_name = L"ac_journal_pipe";
inline constexpr std::string_view manifest_filename =
    "journal.keymap_commands.txt";
inline constexpr std::string_view ready_message = "journal_ready";

enum class Request : std::int32_t {
    invoke = 0,
    shutdown = 1,
};

[[nodiscard]] constexpr std::int32_t to_wire(Request request) noexcept {
    return static_cast<std::int32_t>(request);
}

struct Command {
    std::string_view name;
};

} // namespace ac::protocol::journal

// These public tokens deliberately preserve the familiar keymap spellings.
// They carry names only; the implementations live exclusively in journal.exe.
export inline constexpr ac::protocol::journal::Command print_Tabby_choice {"print_Tabby_choice"};
export inline constexpr ac::protocol::journal::Command print_Eric_choice {"print_Eric_choice"};
export inline constexpr ac::protocol::journal::Command print_Katrina_choice {"print_Katrina_choice"};
export inline constexpr ac::protocol::journal::Command print_Lily_choice {"print_Lily_choice"};
export inline constexpr ac::protocol::journal::Command print_Star_choice {"print_Star_choice"};
export inline constexpr ac::protocol::journal::Command print_Luna_choice {"print_Luna_choice"};
export inline constexpr ac::protocol::journal::Command print_Daniel_choice {"print_Daniel_choice"};
export inline constexpr ac::protocol::journal::Command print_Jose_choice {"print_Jose_choice"};
export inline constexpr ac::protocol::journal::Command print_Jose_choices {"print_Jose_choices"};
export inline constexpr ac::protocol::journal::Command print_James_choice {"print_James_choice"};
export inline constexpr ac::protocol::journal::Command print_Jace_choice {"print_Jace_choice"};
export inline constexpr ac::protocol::journal::Command print_Tyler_choice {"print_Tyler_choice"};
export inline constexpr ac::protocol::journal::Command print_Gin_choice {"print_Gin_choice"};
export inline constexpr ac::protocol::journal::Command print_Gianna_choice {"print_Gianna_choice"};
export inline constexpr ac::protocol::journal::Command print_one_is_selected {"print_one_is_selected"};
export inline constexpr ac::protocol::journal::Command print_two_is_selected {"print_two_is_selected"};
export inline constexpr ac::protocol::journal::Command print_extended_timestamp {"print_extended_timestamp"};
export inline constexpr ac::protocol::journal::Command print_episode_title {"print_episode_title"};
export inline constexpr ac::protocol::journal::Command save_file_and_create_new_file {"save_file_and_create_new_file"};

export inline constexpr std::array journal_commands {
    print_Tabby_choice, print_Eric_choice, print_Katrina_choice,
    print_Lily_choice, print_Star_choice, print_Luna_choice,
    print_Daniel_choice, print_Jose_choice, print_Jose_choices,
    print_James_choice, print_Jace_choice, print_Tyler_choice,
    print_Gin_choice, print_Gianna_choice,
    print_one_is_selected, print_two_is_selected,
    print_extended_timestamp,
    print_episode_title, save_file_and_create_new_file,
};
