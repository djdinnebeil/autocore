/**
 * \file journal_commands.ixx
 * \brief Runtime command registry for `journal_ac.exe`.
 */
export module journal_commands;

import command_registry;
import std;

export inline constexpr std::string_view word_activation_fallback_message =
    "Pin or start Microsoft Word, then run taskbar_builder.exe to generate "
    "word.map.";

export command_registry::Registry create_journal_command_registry(
    command_registry::Action activate_word
);
export command_registry::Registry create_journal_command_registry();
