/**
 * \file writer_component.ixx
 * \brief `writer_ac.exe` Component session.
 */
export module writer_component;

import auto_core.core.component;
import auto_core.core.logging.config;

export ac::Component& writer_component() {
    static ac::Component component {
        "writer",
        ac::logging::config::LoggingScope {"writer"}
    };
    return component;
}
