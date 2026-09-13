/**
 * \file journal_commands.ixx
 * \brief Runtime command registry for `journal_ac.exe`.
 */
export module journal_commands;

import command_registry;

export command_registry::Registry create_journal_command_registry();
