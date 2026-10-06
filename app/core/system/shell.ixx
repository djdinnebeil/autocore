/**
 * \file shell.ixx
 * \brief Assigns the shared Auto Core process AppUserModelID.
 *
 * `Djdinn.AutoCore` is the machine-facing Shell identity for the runtime
 * family. `SetCurrentProcessExplicitAppUserModelID` applies it to the
 * calling process only, so each participating executable calls
 * `set_process_app_user_model_id` during its own startup, before UI or
 * Jump List work. That shared startup call also initializes crash diagnostics
 * from `config/crash_recovery.ini` for every Auto Core executable.
 *
 * Failure is reported through `ac::error` and does not stop the process.
 */
module;

#include "ac_api.hpp"

export module auto_core.core.shell;

export namespace ac::shell {
    /**
     * \brief Assigns `Djdinn.AutoCore` to the current process.
     *
     * Calls `SetCurrentProcessExplicitAppUserModelID`. A failed call is
     * written to stderr and `errors/errors.log`. Startup continues either way.
     */
    AC_API void set_process_app_user_model_id() noexcept;
}
