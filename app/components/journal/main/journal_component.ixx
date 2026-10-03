/**
 * \file journal_component.ixx
 * \brief `journal_ac.exe` Component session.
 */
export module journal_component;

import auto_core.core.component;
import auto_core.core.logging.config;

export ac::Component& journal_component() {
    static ac::Component component {
        "journal",
        ac::logging::config::LoggingScope {"journal"}
    };
    return component;
}
