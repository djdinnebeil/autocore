/**
 * \file main_taskbar.ixx
 * \brief Owns Main's Taskbar commands.
 */
export module auto_core.main.taskbar;

export import command_registry;
export import auto_core.taskbar;
import std;

export namespace taskbar_runtime_commands {
    /**
     * Registers activation commands from Taskbar application maps. Names in
     * `shared_names` are left for the activation-map resolution.
     */
    void register_configured_with(
        command_registry::Registry& registry,
        const std::unordered_set<std::string>& shared_names
    );
    /** Registers the remaining Main-owned Taskbar command surface. */
    void register_with(command_registry::Registry& registry);
}

/** Factory for a configured `activate_*` command. Empty `command` logs as `activate_<application>`. */
export command_registry::Action taskbar_activation_action(
    std::string_view application,
    std::string_view command = {}
);
