/**
 * \file writer_component.ixx
 * \brief `writer_ac.exe` Component session.
 */
export module writer_component;

import auto_core.core.component;

export ac::Component& writer_component() {
    static ac::Component component {"writer"};
    return component;
}
