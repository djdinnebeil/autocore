/**
 * \file journal_commands.ixx
 * \brief Runtime command registry for `journal_ac.exe`.
 */
export module journal_commands;

import command_registry;
import std;

export command_registry::Registry create_journal_command_registry(
    command_registry::Action activate_word
);
export command_registry::Registry create_journal_command_registry();

/** \brief Joins episode-title workers before the database connection closes. */
export void join_title_actions();
