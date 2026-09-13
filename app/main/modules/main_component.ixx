/**
 * \file main_component.ixx
 * \brief Owns the Main `Component`, process launch, F-lock, and shutdown.
 */
export module auto_core.main.application;

export import command_registry;
import auto_core.core.component;
import std;
import <Windows.h>;

export namespace main_component::runtime_commands {
    /** Registers `close_program` and the F-lock commands. */
    void register_with(command_registry::Registry& registry);
}

/** Process-wide Main component. Logging is synchronized per Component. */
export extern ac::Component auto_core;

export namespace ac::main {
    /**
     * Set on the shutdown path. The keyboard hook and message loop observe
     * this flag.
     */
    bool program_closing = false;
    /** Low-level keyboard hook installed on the main thread. */
    HHOOK keyboard_hook;
    /** Thread that owns the message loop and hook dispatch. */
    DWORD main_thread_id;

    /**
     * \brief Starts `executable_path` with optional arguments.
     *
     * Process and thread handles are closed immediately. Does not wait for
     * the child.
     * \return `false` if `CreateProcessW` fails.
     */
    bool create_process(
        const std::filesystem::path& executable_path,
        std::wstring_view arguments = {},
        std::uint32_t creation_flags = 0
    );
    /**
     * \brief Starts a process and tries to focus its new window.
     *
     * Default `creation_flags` is `CREATE_NEW_CONSOLE`. Returns `true` if
     * the process started even when focus cannot be confirmed.
     */
    bool create_process_and_focus(
        const std::filesystem::path& executable_path,
        std::wstring_view arguments = {},
        std::uint32_t creation_flags = CREATE_NEW_CONSOLE
    );
}

/**
 * Selects the primary keymap action when `true`. `activate_function_key`
 * sets this to `false` so the next binding uses `secondary`.
 */
export bool primary = true;

export {
    /** Signals components to stop, unhooks the keyboard, and posts `WM_QUIT`. */
    void close_program();
    /** Uses each key's secondary keymap action until deactivated. */
    void activate_function_key();
    /** Restores primary keymap actions. */
    void deactivate_function_key();
    /** Focuses the Auto Core console for an interactive prompt. */
    void set_focus_auto_core();
}
