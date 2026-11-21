/**
 * \file taskbar_commands.ixx
 * \brief Rebuildable command registry owned by auto_core_taskbar.exe.
 */
export module taskbar_commands;

import command_registry;

export command_registry::Registry create_taskbar_command_registry();
