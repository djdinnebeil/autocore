/**
 * \file journal_component.ixx
 * \brief `journal_ac.exe` Component session.
 */
export module journal_component;

import auto_core.core.component;

export ac::Component& journal_component() {
    static ac::Component component {"journal"};
    return component;
}
