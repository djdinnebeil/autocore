# Main executable follow-up work

This file records improvements that are intentionally deferred and do not
block the current Main version. Product-level follow-ups are tracked in
[devs/TODO.md](../../../devs/TODO.md). DLL-specific items stay in
[app/core/TODO.md](../../core/TODO.md).

Interactive taskbar cycling encapsulation (`switch_set` read by
`keyboard_input::process_key_event`) is tracked in [devs/TODO.md](../../../devs/TODO.md). Do not
change that control flow here.

## Do not block the message loop on Slash

**Status:** Deferred responsiveness

A one-shot descriptor with `wait=infinite` waits with
`WaitForSingleObject(..., INFINITE)` on the main thread after starting the
child. Slash uses that setting. Recycle-bin work can take long
enough that posted keymap events sit until the child exits.

Future work should wait off the message-loop thread, or use a bounded wait
with an explicit user-visible failure, without changing Slash's
one-shot-per-command launch model.

### Acceptance criteria

- A Slash keymap command does not prevent other configured keys from running
  while `slash_ac.exe` is still working.
- Recycle-bin deletion remains the child's job, not Main's.
- Shutdown can still complete if a Slash process is in flight.
