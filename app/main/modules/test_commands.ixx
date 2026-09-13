/**
 * \file test_commands.ixx
 * \brief Temporary diagnostic keymap commands.
 *
 * Not intended for production keymaps.
 */
export module auto_core.main.test_commands;

export import command_registry;

export namespace test_commands::runtime_commands {
    /** Registers `encoding_test` and `send_crash_command`. */
    void register_with(command_registry::Registry& registry);
}
