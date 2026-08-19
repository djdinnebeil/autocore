export module journal_component;

import auto_core.component;

export ac::Component& journal_component() {
    static ac::Component component {"journal"};
    return component;
}
