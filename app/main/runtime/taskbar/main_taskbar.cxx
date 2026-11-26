module;

#include "activate_factory.hpp"

module auto_core.main.taskbar;

import std;
import auto_core.main.application;
import auto_core.taskbar;

command_registry::Action configured_activation_action(
    std::string_view arguments
) {
    const auto first = arguments.find_first_not_of(" \t");
    if (first == std::string_view::npos) return {};
    const auto last = arguments.find_last_not_of(" \t");
    arguments = arguments.substr(first, last - first + 1);

    if (arguments.size() >= 2 &&
        ((arguments.front() == '"' && arguments.back() == '"') ||
         (arguments.front() == '\'' && arguments.back() == '\''))) {
        arguments.remove_prefix(1);
        arguments.remove_suffix(1);
    }
    if (arguments.empty() || arguments.contains(',')) return {};

    return taskbar_activation_action(arguments);
}

command_registry::Action taskbar_activation_action(
    const std::string_view application,
    const std::string_view command
) {
    std::string log_name {command};
    if (log_name.empty()) {
        log_name = "activate_";
        log_name += application;
    }
    return [
        application = std::string {application},
        command = std::string {command},
        log_name = std::move(log_name)
    ] {
        auto_core.log_info("{}", log_name);
        if (command.empty()) {
            ac::taskbar::activate_application(application);
            return;
        }
        ac::taskbar::activate_application(application, command);
    };
}

void taskbar_runtime_commands::register_with(
    command_registry::Registry& registry
) {
    registry.add_factory(
        std::string {ac::main::activate_factory_name},
        &::configured_activation_action,
        std::string {ac::main::activate_factory_autocomplete},
        std::string {ac::main::activate_factory_description},
        std::string {ac::main::activate_factory_provider}
    );
}

void taskbar_runtime_commands::register_configured_with(
    command_registry::Registry& registry,
    const std::unordered_set<std::string>& shared_names
) {
    for (const auto& command :
         ac::taskbar::configured_activation_bindings_from_disk()) {
        if (shared_names.contains(command.command)) {
            continue;
        }
        if (registry.contains(command.command)) {
            if (command.command == "activate_auto_core") {
                auto_core.log_info(
                    "Ignoring configured taskbar command '{}' for key '{}' "
                    "because that command name is already registered.",
                    command.command,
                    command.application
                );
            }
            else {
                auto_core.log_alert(
                    "Ignoring configured taskbar command '{}' for key '{}' "
                    "because that command name is already registered.",
                    command.command,
                    command.application
                );
            }
            continue;
        }
        registry.add(
            command.command,
            taskbar_activation_action(command.application, command.command),
            command.description,
            "taskbar"
        );
    }
}
