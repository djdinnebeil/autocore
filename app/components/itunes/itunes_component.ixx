/**
 * \file itunes_component.ixx
 * \brief `itunes_ac.exe` Component session.
 */
export module itunes_component;
import auto_core.core.component;

export ac::Component itunes_component("itunes");
/** Rolls the local iTunes log to today's file if needed. */
export void update_itunes_component();
