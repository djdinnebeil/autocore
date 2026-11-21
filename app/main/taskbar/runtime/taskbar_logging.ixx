/**
 * \file taskbar_logging.ixx
 * \brief Shared logger for auto_core_taskbar.exe.
 */
export module taskbar_logging;

import auto_core.core.component;
import auto_core.core.logging.config;

export ac::Component& taskbar_component() {
    static ac::Component component {
        "taskbar",
        ac::logging::config::LoggingScope {"taskbar"}
    };
    return component;
}
