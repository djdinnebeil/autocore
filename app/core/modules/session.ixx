/**
 * \file session.ixx
 * \brief Provides session-start date and time information.
 */
module;

#include "ac_api.hpp"

export module session;

import std;

export namespace ac::session {
    /**
     * \brief Contains the date and time at which a session started.
     */
    struct Start {
        std::string date_iso;
        std::string timestamp_with_seconds;
        std::string datetime;
    };

    /**
     * \brief Captures the current local date and time as a session start.
     *
     * All values are derived from the same local-time snapshot.
     *
     * \return The captured session-start information.
     */
    AC_API Start make_start();
    AC_API std::string make_datetime();
}