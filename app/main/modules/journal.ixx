/** \file journal.ixx Main-process client for journal.exe. */
export module journal;

export import journal_protocol;
export import command_registry;
import std;

export namespace journal::runtime_commands {
    void register_with(command_registry::Registry& registry);
}

export {
    void create_journal_pipe();
    void start_journal_component();
    bool wait_for_journal_ready();
    void send_journal_end_signal();
    std::function<void()> journal_command(ac::protocol::journal::Command command);
    std::function<void()> journal_print_choice_command(
        std::string name,
        bool include_zero = false
    );
}
