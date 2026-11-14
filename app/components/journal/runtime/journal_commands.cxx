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
std::mutex title_workers_mutex;
std::vector<std::thread> title_workers;

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
    const std::lock_guard lock {title_workers_mutex};
    title_workers.push_back(std::move(worker));
}

} // namespace

command_registry::Registry create_journal_command_registry(
    command_registry::Action activate_word
) {
    command_registry::Registry registry;
    registry.add("activate_word", std::move(activate_word));
    registry.add(
        std::string {::print_extended_timestamp.name},
        actions::print_extended_timestamp
    );
    registry.add(std::string {::print_episode_title.name}, [] {
        start_title_action(&journal_title::print_episode_title);
    });
    registry.add(std::string {::save_and_create_new_document.name}, [] {
        start_title_action(&journal_title::save_and_create_new_document);
    });
    journal::factories::register_factory_commands(registry);
    journal::factories::load_alias_commands(registry);
    return registry;
}

void join_title_actions() {
    std::vector<std::thread> workers;
    {
        const std::lock_guard lock {title_workers_mutex};
        workers.swap(title_workers);
    }
    for (std::thread& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

command_registry::Registry create_journal_command_registry() {
    return create_journal_command_registry([] {
        journal_component().print(word_activation_fallback_message);
    });
}
