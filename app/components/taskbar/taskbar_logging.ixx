/**
 * \file taskbar_logging.ixx
 * \brief Shared component logger for taskbar_ac.exe.
 */
export module taskbar_logging;

import auto_core.core.component;

export ac::Component& taskbar_component() {
    static ac::Component component {"taskbar"};
    return component;
}
