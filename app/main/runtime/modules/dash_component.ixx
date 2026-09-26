/**
 * \file dash_component.ixx
 * \brief Main-process launcher for `dash_ac.exe`.
 */
export module auto_core.main.components.dash;

export import command_registry;

export namespace dash::runtime_commands {
    /** Registers `launch_dash`. */
    void register_with(command_registry::Registry& registry);
}

export {
    /**
     * Starts `dash_ac.exe` with `--target` (foreground HWND) and `--parent-pid`.
     * Dash has no long-lived pipe.
     */
    void launch_dash();
}
