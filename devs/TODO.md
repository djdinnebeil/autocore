# Auto Core follow-up work

DLL-specific deferred work is tracked in [app/core/TODO.md](../app/core/TODO.md).
Main-specific deferred work is tracked in [app/main/runtime/TODO.md](../app/main/runtime/TODO.md).

## Done: incremental main-log merge

**Status:** `logger_ac.exe` merges local `.main.log` files.

Each executable still writes `{date}_{name}.log` and `{date}_{name}.main.log`.
`logger_ac.exe` appends new main-log records to `YYYY-MM-DD_main.log`.
`config/logging.ini` sets `disable_all`, `directory`,
`write_logs_to_files`, `write_logs_to_console`, `log_print_mode`, and
`component_logging_default`. Each family INI sets `logging` (`on` except Dash `off`).
`config/logger.ini` sets
`merge_interval_seconds` and `merge_logs_on_shutdown`. The shutdown merge
is a detached `logger_ac.exe --once` after the hosted logger has exited.

## Parked: journal_config.exe menu

**Status:** Deferred helper finetune

`journal_config.exe` prompts for `journal.ini` when that file is missing and
then launches each Journal data owner. It still writes only `journal.ini`.
Extra menus beyond Journal settings are later work, not a clone or
beta-tester blocker.

## Parked: wake event filters

**Status:** History directory landed; filters still deferred

`[wake] directory` owns `current.event`, `previous.event`, and `wake_events.log`.
Ordinary Wake logs still follow `logging.ini`.

Later, `wake.ini` can hold filters to exclude certain system wake events and
an `enabled` switch (`on` / `off` only)
for the history capture itself. Do not add those keys until Wake needs them.

## Encapsulate interactive taskbar cycling

`keyboard_input::process_key_event()` currently reads `taskbar.switch_set`
directly. Its
tested control-flow structure should remain unchanged during the taskbar
authority and configuration migration.

After the migration has stabilized:

- Make the interactive cycling fields in `MainTaskbarState` private.
- Add a read-only `taskbar.cycle_active()` query instead of exposing
  `switch_set`.
- Re-evaluate `keyboard_input::process_key_event()` only with regression coverage for
  ordinary shortcuts, held-Win cycling, and Win-key release behavior.

The instance member should continue to use object member access. The C++ scope
resolution operator is not appropriate for per-instance cycling state.

## Investigate iTunes privilege compatibility and security

**Status:** Deferred security and compatibility review

The current elevated Auto Core process cannot connect to an iTunes instance
that is already running without Administrator privileges. Starting Auto Core
first works because the iTunes instance it activates runs in a compatible
privilege context. Bounded COM initialization retry is now implemented, so the
remaining failure is associated with the process elevation mismatch rather
than startup timing.

The current limitation and workarounds are documented in `README.md` and
`docs/manual/itunes.md`.

Investigate whether the iTunes component can safely control an existing
non-administrator iTunes process. Review why Auto Core requires elevation,
whether the music integration can run at ordinary user privilege, and whether
introducing a lower-privilege helper or another COM activation strategy would
create unacceptable spoofing, IPC, file-access, or privilege-boundary risks.
Prefer a least-privilege design and do not weaken Windows security boundaries
merely to make the connection succeed.

### Acceptance criteria

- Document the exact Windows integrity/elevation combinations that work and
  fail.
- Determine whether Auto Core or its iTunes component can run without
  Administrator privileges.
- If non-administrator iTunes attachment is implemented, preserve process and
  IPC security boundaries and add regression coverage for both privilege
  configurations.
- Document any remaining elevation requirement and its security implications.
