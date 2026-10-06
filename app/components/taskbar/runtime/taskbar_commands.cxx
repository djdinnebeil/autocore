module taskbar_commands;

import std;
import auto_core.core.console;
import auto_core.taskbar;
import command_registry;
import taskbar_logging;
import taskbar_protocol;

void activate_auto_core() {
    if (ac::taskbar::try_activate_native("auto_core")) {
        taskbar_component().log_main(
            "activate_auto_core() - used the current Winkey mapping"
        );
        return;
    }

    const auto activated = ac::console::activate();
    if (!activated) {
        taskbar_component().log_print(
            "Unable to activate the shared Auto Core console: {}.",
            ac::console::error_message(activated.error())
        );
        return;
    }
    taskbar_component().log_main(
        "activate_auto_core() - activated the shared Auto Core console"
    );
}

void refresh_taskbar_positions() {
    taskbar_component().log_main("refresh_taskbar_positions");
    if (!ac::taskbar::request_refresh()) {
        taskbar_component().log_print(
            "Unable to update Winkey mappings."
        );
        return;
    }
    taskbar_component().log_print("Winkey mappings updated");
}

command_registry::Registry create_taskbar_command_registry() {
    command_registry::Registry registry;
    namespace commands = ac::protocol::taskbar::commands;

    registry.add(
        std::string {commands::activate_auto_core},
        &::activate_auto_core
    );
    registry.add(
        std::string {commands::refresh_taskbar_positions},
        &::refresh_taskbar_positions
    );

    return registry;
}
