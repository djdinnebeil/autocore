/**
 * \file taskbar_commands.ixx
 * \brief Rebuildable command registry owned by taskbar_ac.exe.
 */
export module taskbar_commands;

import command_registry;

export command_registry::Registry create_taskbar_command_registry();
