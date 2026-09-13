/**
 * \file taskbar_component.ixx
 * \brief Main-process lifecycle client for `taskbar_ac.exe`.
 *
 * Snapshot lookups use `auto_core.taskbar::connect`. Named invokes use the
 * control pipe defined by `taskbar_protocol`.
 */
export module auto_core.main.components.taskbar;

import std;

export import command_registry;
export import taskbar_protocol;

export namespace taskbar_component::runtime_commands {
    /** Registers refresh, `activate_auto_core`, and Main-side WordPad/admin PowerShell. */
    void register_with(command_registry::Registry& registry);
}

export {
    /** Returns a keymap action that sends `name` on the taskbar control pipe. */
    [[nodiscard]] command_registry::Action taskbar_pipe_command(
        std::string_view name
    );
    /**
     * Starts `taskbar_ac.exe`, waits for `taskbar_ready`, then snapshot `connect`.
     * \return `false` if the pipe, process, ready-wait, or connect fails.
     */
    [[nodiscard]] bool initialize_taskbar_component();
    /** Sends control-pipe shutdown and disconnects the snapshot client. */
    void stop_taskbar_component() noexcept;
    /** Forwards `name` on the control pipe. */
    void invoke_taskbar_command(std::string_view name);
    /** Invokes `refresh_taskbar_positions` on `taskbar_ac.exe`. */
    void refresh_taskbar_positions();
}
