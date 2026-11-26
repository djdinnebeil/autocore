module;
#include "../../../core/taskbar/activation_catalog_description.hpp"

module taskbar_commands;

import std;
import auto_core.taskbar;
import command_registry;
import taskbar_logging;
import taskbar_protocol;

void activate_auto_core() {
    taskbar_component().log_info("activate_auto_core()");
    ac::taskbar::activate_application("auto_core");
}

void refresh_taskbar_positions() {
    taskbar_component().log_info("refresh_taskbar_positions");
    if (!ac::taskbar::request_refresh()) {
        taskbar_component().log_alert(
            "Unable to update Winkey mappings."
        );
        return;
    }
    taskbar_component().log_alert("Winkey mappings updated");
}

command_registry::Registry create_taskbar_command_registry() {
    command_registry::Registry registry;
    namespace commands = ac::protocol::taskbar::commands;

    registry.add(
        std::string {commands::activate_auto_core},
        &::activate_auto_core,
        ac::taskbar::activation_catalog_description(
            commands::activate_auto_core,
            "auto_core",
            {}
        )
    );
    registry.add(
        std::string {commands::refresh_taskbar_positions},
        &::refresh_taskbar_positions,
        "Recalculates taskbar Win+number positions."
    );

    return registry;
}
