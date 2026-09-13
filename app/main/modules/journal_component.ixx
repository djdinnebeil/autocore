/**
 * \file journal_component.ixx
 * \brief Main-process client for `journal_ac.exe`.
 */
export module auto_core.main.components.journal;

export import journal_protocol;
export import command_registry;
import std;

export namespace journal::runtime_commands {
    /** Registers protocol names, print-choice factories, and `launch_journal_config`. */
    void register_with(command_registry::Registry& registry);
}

export {
    /** Creates `ac_journal_pipe`. */
    void create_journal_pipe();
    /** Starts `journal_ac.exe`. */
    void start_journal_component();
    /** Waits up to five seconds for `journal_ready`. */
    bool wait_for_journal_ready();
    /** Sends protocol shutdown. */
    void send_journal_end_signal();
    /** Parameterized print-choice invoke (`name`, optional include-zero). */
    std::function<void()> journal_print_choice_command(
        std::string name,
        bool include_zero = false
    );
    /** Parameterized print-choice invoke (`name`, lower bound, optional count). */
    std::function<void()> journal_print_choice_command(
        std::string name,
        int lower_bound,
        int count = 1
    );
    /** Parameterized print-and-insert invoke (quoted text). */
    std::function<void()> journal_print_and_insert_command(std::string text);
}
