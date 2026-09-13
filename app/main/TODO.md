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

**Status:** Deferred startup diagnostics

`components::initialize()` calls `wait_for_journal_ready()` and ignores the
result. Writer ready-wait failure is logged and writer commands stay
unavailable for the session. Journal has the same 5s ready contract but a
timeout is only visible if `wait_for_journal_ready` itself logged.

Treat a journal ready failure the same way as writer: log that journal
commands are unavailable, and keep startup non-fatal.

### Acceptance criteria

- A missing or late `journal_ac.exe` produces a single clear console/log line.
- Main still reaches the message loop.
- A successful ready handshake is unchanged.

## Graceful server shutdown

**Status:** Deferred process-lifetime hardening

`stop_server()` uses `TerminateProcess`. Logger shutdown already requests a
graceful stop and only terminates after a timeout. If `server_ac.exe` grows a
control channel or window-close path, prefer that before terminating.

### Acceptance criteria

- `server_ac.exe` can exit from a Main-initiated stop without `TerminateProcess`
  on the success path.
- A stuck server is still reaped so Main can exit.
- Start-while-already-running remains a no-op.

## Mutex iTunes shutdown with named invokes

**Status:** Deferred shutdown race

Spotify's integer pipe commands take `spotify_pipe_mutex`. iTunes `invoke_named`
is mutexed, but `send_command` (used for shutdown) is not. A keymap invoke
overlapping `close_program()` can interleave frames on `ac_itunes_pipe`.

### Acceptance criteria

- Shutdown and named invokes are serialized on the iTunes pipe.
- A failed shutdown send still continues the rest of `close_program()`.

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
