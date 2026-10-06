/**
 * \file crash_diagnostics.ixx
 * \brief Shared, logging-independent process crash diagnostics.
 */
module;

#include "ac_api.hpp"
#include <Windows.h>

export module auto_core.core.crash_diagnostics;

export namespace ac::crash {

    enum class Operation : unsigned char {
        unspecified,
        startup,
        idle,
        shutdown,
    };

    enum class State : unsigned char {
        unspecified,
        entering,
        active,
        leaving,
    };

    /**
     * Loads configuration and DbgHelp, caches process metadata, and installs
     * the process-wide unhandled-exception filter. Safe to call repeatedly.
     */
    AC_API void initialize() noexcept;

    /** Updates bounded, predefined context without accepting arbitrary text. */
    AC_API void set_context(Operation operation, State state) noexcept;

    /** True when the cached crash_recovery.ini policy enables diagnostics. */
    [[nodiscard]] AC_API bool enabled() noexcept;

    /**
     * Writes one crash event for this process. Main calls this from its own
     * recovery filter; other executables use the installed shared filter.
     */
    AC_API void write_report(EXCEPTION_POINTERS* exception) noexcept;

} // namespace ac::crash
