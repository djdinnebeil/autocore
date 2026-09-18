# Auto Core follow-up work

DLL-specific deferred work is tracked in [app/core/TODO.md](../app/core/TODO.md).
Main-specific deferred work is tracked in [app/main/TODO.md](../app/main/TODO.md).

## Parked: log_merger_ac.exe

**Status:** Separate future project; not part of the pre-release logger

`logger_ac.exe` is feature-complete for pre-release and is in
field-testing/maintenance status. Do not begin `log_merger_ac.exe` while
finalizing that implementation. The future merger may consume daily component
`.main.log` files and produce a chronological centralized log, as described in
[configuration.md](configuration.md#logging). Its synchronization,
reconciliation, incremental-offset, scheduling, and merge design remain
deferred.

## Parked: keymap_config.exe

**Status:** Next core product upgrade after clone-and-run readiness

The keymap is already usable without a helper. Live `dist/keymap/bindings.ini`
is under gitignored `dist/`. A missing file is seeded on first start and never
overwritten. [`defaults/keymap/bindings.ini`](../defaults/keymap/bindings.ini)
is the tracked sample (documentation only; Auto Core does not read
`defaults/`). Autocomplete catalogs under `keymap/` are generated at
startup.

Do not start `keymap_config.exe` until clone-and-run is honest (build, vendor
runtime DLLs, first-run seeds). A console editor only pays off after a locked
autocomplete/editor decision (stock Windows console vs opening `bindings.ini`
in an editor vs a custom line editor).

## Parked: journal_config.exe and writer_config.exe menus

**Status:** Deferred helper finetune

`journal_config.exe` and `writer_config.exe` already prompt for data
directories when the live INI is missing. Main seeds missing `journal.ini` and
`writer.ini` from portable defaults. Extra menus are later work, not a
clone or beta-tester blocker.

## Parked: slash.ini

**Status:** Deferred; no settings yet

Slash stays on protocol and keymap names only. Do not add `config/slash.ini`
until slash has settings. A future silence mode belongs in that file when it
is opened.

## Parked: wake.ini

**Status:** Deferred

Wake extras already live under `logs/components/wake/` (`wake_latest.log`,
`wake_previous.log`, `wake_master.log`) next to the daily wake log. Do not add
`config/wake.ini` yet.

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
