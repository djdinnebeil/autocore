module journal_commands;

import std;
import auto_core.core.thread;
import command_registry;
import journal_clock;
import journal_component;
import journal_factories;
import journal_protocol;
import journal_title;

namespace {

std::mutex title_action_mutex;

namespace actions {

void print_extended_timestamp() {
    journal_component().print_and_insert(
        journal_clock::get_extended_timestamp()
    );
}

} // namespace actions

void start_title_action(void (*action)()) {
    std::thread worker([action] {
        const std::scoped_lock lock {title_action_mutex};
        ac::thread::run_with_exception_handling(action, journal_component());
    });
    worker.detach();
}

} // namespace

command_registry::Registry create_journal_command_registry() {
    command_registry::Registry registry;
    registry.add(
        std::string {::print_extended_timestamp.name},
        actions::print_extended_timestamp
    );
    registry.add(std::string {::print_episode_title.name}, [] {
        start_title_action(&journal_title::print_episode_title);
    });
    registry.add(std::string {::save_file_and_create_new_file.name}, [] {
        start_title_action(&journal_title::save_file_and_create_new_file);
    });
    journal::factories::register_factory_commands(registry);
    journal::factories::load_alias_commands(registry);
    return registry;
}
