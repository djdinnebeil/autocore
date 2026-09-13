/**
 * \file main_taskbar.ixx
 * \brief Owns Main's interactive taskbar cycling session and commands.
 *
 * Cycling fields are currently public because `process_key_event` reads
 * `switch_set`. Encapsulation is tracked in docs/TODO.md.
 */
export module auto_core.main.taskbar;

export import command_registry;
export import auto_core.taskbar;
import std;
import <Windows.h>;

export namespace taskbar_runtime_commands {
    /** Registers `refresh_firefox`, `start_reddit_new_tab`, `activate()`, and INI `activate_*`. */
    void register_with(command_registry::Registry& registry);
}

/** Factory for a configured `activate_*` command. Empty `command` logs as `activate_<application>`. */
export command_registry::Action taskbar_activation_action(
    std::string_view application,
    std::string_view command = {}
);

export {
    /** Main-owned Firefox refresh; not a `taskbar_ac.exe` authority command. */
    void refresh_firefox();
    /** Opens a Reddit new-tab workflow in the configured browser. */
    void start_reddit_new_tab();
}

export class MainTaskbarState {
public:
    ac::taskbar::Position switch_position {};
    /** Last dispatched key; compared in `switch_windows` to advance vs end a cycle. */
    int switch_keycode {};
    /** `true` while an interactive Win-held cycling session is active. */
    bool switch_set {false};
    /** `true` when the cycle is Main-emulated rather than native Win+position. */
    bool emulated_cycle {false};
    std::string cycle_application;
    std::vector<ac::taskbar::WindowHandle> cycle_windows;
    std::size_t cycle_index {};

    /** Same key advances the cycle; any other key ends it. */
    void switch_windows(int keycode);
    /** Native Win+position activate or native cycle. `false` falls through to emulate. */
    [[nodiscard]] bool try_activate_configured(std::string_view name);
    /** Native first, then emulate (launch / restore / minimize / cycle). */
    void activate_configured(std::string_view name);

private:
    void end_cycle_session();
    void emulate_configured(std::string_view name);
    void begin_emulated_cycle(
        std::string_view name,
        std::vector<ac::taskbar::WindowHandle> windows
    );
    void advance_emulated_cycle();
};

/** Single Main-owned cycling and activation instance. */
export MainTaskbarState taskbar;
