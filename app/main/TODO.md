# Main executable follow-up work

This file records improvements that are intentionally deferred and do not
block the current Main version. Product-level follow-ups are tracked in
[docs/TODO.md](../../docs/TODO.md). DLL-specific items stay in
[app/core/TODO.md](../core/TODO.md).

Interactive taskbar cycling encapsulation (`switch_set` read by
`keyboard_input::process_key_event`) is tracked in [docs/TODO.md](../../docs/TODO.md). Do not
change that control flow here.

## Do not block the message loop on Slash

**Status:** Deferred responsiveness

`slash_component_command` waits with `WaitForSingleObject(..., INFINITE)` on
the main thread after starting `slash_ac.exe`. Recycle-bin work can take long
enough that posted keymap events sit until the child exits.

Future work should wait off the message-loop thread, or use a bounded wait
with an explicit user-visible failure, without changing Slash's
one-shot-per-command launch model.

### Acceptance criteria

- A Slash keymap command does not prevent other configured keys from running
  while `slash_ac.exe` is still working.
- Recycle-bin deletion remains the child's job, not Main's.
- Shutdown can still complete if a Slash process is in flight.

## Surface journal ready-wait failure

**Status:** Done by the generic host

A failed `ac.component.v1` hello, including journal, logs and disables only
that child. Commands from that catalog are not registered.

## Graceful server shutdown

**Status:** Done by the generic host

`server_ac.exe` speaks `ac.component.v1`. Main sends `shutdown` on
`ac_server_pipe`, waits briefly, then terminates if the process is still
alive.

## Mutex iTunes shutdown with named invokes

**Status:** Done by the generic host

Each generic child serializes invoke and shutdown on one per-child mutex.

## Unused keymap.runtime getters

**Status:** Deferred API cleanup

`get_runtime_command_names()` and
`get_runtime_command_autocomplete_values()` are exported from
`auto_core.main.keymap.runtime` but have no in-repo callers. Autocomplete
bytes are written from the registry directly. Removing them is a contract
change, not a hot-path win.

### Acceptance criteria

- Either the getters are used by the workspace writer, or they are removed
  from the module interface.
- Runtime `keymap_commands.txt` refresh behavior is unchanged.
