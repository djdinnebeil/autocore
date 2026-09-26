# Auto Core follow-up work

DLL-specific deferred work is tracked in [app/core/TODO.md](../app/core/TODO.md).
Main-specific deferred work is tracked in [app/main/runtime/TODO.md](../app/main/runtime/TODO.md).

## Done: incremental main-log merge

**Status:** `logger_ac.exe` merges local `.main.log` files.

Each executable still writes `{date}_{name}.log` and `{date}_{name}.main.log`.
`logger_ac.exe` appends new main-log records to `YYYY-MM-DD_main.log`.
`config/logger.ini` sets `directory`, `merge_interval_seconds`,
`merge_logs_on_shutdown`, and `write_logs_to_console`. The shutdown merge
is a detached `logger_ac.exe --once` after the hosted logger has exited.

## Done: keymap_config.exe

**Status:** Shipped with Main configuration architecture

`keymap_config.exe` owns `config/keymap.ini`, creates `dist/keymap/` and
`keymap/components/`, and writes a seed `mappings.ini` if that file is
missing. Runtime never writes those INI files. Autocomplete catalogs under
`keymap/` are still generated at startup.

A later console editor (stock Windows console vs opening `mappings.ini`
in an editor vs a custom line editor) remains optional follow-up.

## Parked: journal_config.exe and writer_config.exe menus

**Status:** Deferred helper finetune

`journal_config.exe` and `writer_config.exe` already prompt for data
directories when the live INI is missing. They own those files; Main does not
seed `journal.ini` or `writer.ini`. Extra menus are later work, not a
clone or beta-tester blocker.

## Parked: slash.ini extras

**Status:** Schema defined; extra keys deferred

`config/slash.ini` is an empty `[slash]` section (`shared/defaults.ixx`). Do
not add silence-mode or other tunables until slash needs them.

## Parked: wake.ini extras

**Status:** Schema defined; extra keys deferred

Wake extras already live under `logs/components/wake/` (`wake_latest.log`,
`wake_previous.log`, `wake_master.log`) next to the daily wake log.
`config/wake.ini` is an empty `[wake]` section so the shared config contract
applies.

Later, that file can hold filters to exclude certain system wake events, an
`enabled` switch (`on` / `off`, with `true` / `false` accepted as aliases),
and an optional path for the master log (default remains the logger
components/wake directory). Do not relocate the master log in this phase.

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
`docs/itunes.md`.

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
