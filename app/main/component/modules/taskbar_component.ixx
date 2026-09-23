/**
 * \file taskbar_component.ixx
 * \brief Main-local taskbar commands that are not generic child catalogs.
 *
 * Snapshot lookups use `auto_core.taskbar::connect` from the generic host.
 * `activate_*` INI commands are registered by `main_taskbar`.
 */
export module auto_core.main.components.taskbar;

import std;

export import command_registry;
export import taskbar_protocol;

export namespace taskbar_component::runtime_commands {
    /** Registers Main-side WordPad/admin PowerShell, launches, and config. */
    void register_with(command_registry::Registry& registry);
}

export {
    /** Returns a keymap action that sends `name` on the taskbar control pipe. */
    [[nodiscard]] command_registry::Action taskbar_pipe_command(
        std::string_view name
    );
    /** Forwards `name` on the generic taskbar control pipe. */
    void invoke_taskbar_command(std::string_view name);
    /** Invokes `refresh_taskbar_positions` on `taskbar_ac.exe`. */
    void refresh_taskbar_positions();
}
