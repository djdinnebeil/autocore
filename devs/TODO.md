# Auto Core follow-up work

DLL-specific deferred work is tracked in [app/core/TODO.md](../app/core/TODO.md).
Main-specific deferred work is tracked in [app/main/runtime/TODO.md](../app/main/runtime/TODO.md).

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
`docs/manual/components/itunes.md`.

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
