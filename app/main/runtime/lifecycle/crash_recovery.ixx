/**
 * \file crash_recovery.ixx
 * \brief Installs Auto Core's crash handler and detects a previous crash.
 *
 * Auto Core restarts after an unhandled exception because it serves as the
 * system's keyboard manager. Recovery remains disabled until the previous-crash
 * acknowledgement has completed.
 *
 * The marker is `<installation_root>/crash/.crash`. Yes removes it; No leaves
 * it so the next start prompts again. Shared diagnostics in `auto_core.dll`
 * independently write one `crash.txt` and best-effort `crash.dmp` event. No
 * executable or PDB is copied. If the marker cannot be written, the handler
 * does not restart. After a successful spawn it
 * runs `close_program()`, writes the local shutdown record, and `ExitProcess(1)`.
 * `config/crash_recovery.ini` `[crash_recovery] default_response` is `yes` or
 * `no`.
 */
export module auto_core.main.crash_recovery;

export {
    /**
     * \brief Prompts if a previous-crash marker exists.
     *
     * Yes removes the marker (best-effort) and continues. No keeps the marker
     * and returns `false`.
     *
     * \return `false` when the user declines to continue startup.
     */
    [[nodiscard]] bool check_for_previous_crash();
    /**
     * \brief Installs the unhandled-exception restart handler.
     *
     * Call after `check_for_previous_crash()` has returned `true`.
     */
    void enable_automatic_crash_recovery();
}
