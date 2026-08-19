export module journal_commands;

import command_registry;

export command_registry::Registry create_journal_command_registry();
export void set_journal_taskbar_position(int position);
