/**
 * \file slash_component.ixx
 * \brief Main-side launcher for `slash_ac.exe` recycle-bin commands.
 *
 * Deletion is performed by the child executable, not by Main.
 */
export module auto_core.main.components.slash;

export import command_registry;
export import slash_protocol;
import std;

export namespace slash_component::runtime_commands {
    /** Registers names from `slash.keymap_commands.txt`, or the protocol `all` list. */
    void register_with(command_registry::Registry& registry);
}

/**
 * Starts `slash_ac.exe` with `command` and waits infinitely on the calling
 * thread. Recycle-bin deletion stays in the child.
 */
export std::function<void()> slash_component_command(
    ac::protocol::slash::CommandName command
);
