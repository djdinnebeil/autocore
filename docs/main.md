# Main executable

`auto_core.exe` is the Auto Core keyboard manager. It lives in
`app/main/runtime`, links `auto_core.lib`, and loads `auto_core.dll`. Child
product behavior is documented with each component; this page covers the Main
process only.

## Startup order

[`main.cxx`](../app/main/runtime/main.cxx) runs this sequence:

1. Wait for any previous `auto_core.exe` to exit (`Local\AutoCore.main`
   mutex, held until this process ends).
2. If `config/auto_core.ini` is missing, the installation is new. Launch
   `auto_core_init.exe` and wait. Its menu is `Auto Core` with
   `1. Use defaults` and `2. Configure`. That choice runs only
   `auto_core_config.exe --seed` or `auto_core_config.exe --init`. It then
   launches `logger_init.exe`, which has its own menu (`1. Use defaults`,
   `2. Configure`, `3. Disable logging`) and launches `logging_config.exe`
   and `logger_config.exe`. Use defaults passes `--seed`. Configure passes
   `--init`. Disable logging passes `--disable` to `logging_config.exe`,
   which writes the compiled logging file with `disable_all = on`, and
   `--seed` to `logger_config.exe`. Neither init executable writes an INI.
   `components_init.exe` then shows its own defaults/configure menu.
   The remaining owners are seeded: `keymap_config.exe`,
   `components_config.exe`, `shutdown_config.exe`,
   `crash_recovery_config.exe`, and `components_editor.exe --seed`, then
   `keymap_editor.exe`. A failed or cancelled Logger or Components stage
   stops initialization. After the orchestrator returns, resolve the shared
   logging policy. If `logging.ini` is still missing, exit `1`.
   `auto_core_config.exe` writes `auto_core.ini`. Presence of the file means
   Auto Core is initialized. There is no `initialized` key. If a required
   step fails or the user cancels that step, and this run created
   `auto_core.ini`, the orchestrator removes the file before it exits.
   If the file still does not exist, exit `1`. Startup
   does not check `components.list` or `keymap.map`. A missing list is
   reported later and discovered `*_ac.exe` names are enabled in memory.
   The file is not created. A missing `keymap.map` installs the emergency
   keymap in memory and is not created.
3. If `auto_core.ini` already exists, resolve the shared logging policy
   before any other startup log. A missing `logging.ini` uses built-in
   defaults (`disable_all = off`, `directory = logs`,
   `write_logs_to_files = on`, `write_logs_to_console = off`,
   `log_print_mode = print`, `component_logging_default = on`), prints
   `config/logging.ini is missing; using built-in logging defaults.`,
   and does not create the file.
4. Load `[auto_core] warn_without_winkey_mapping` from `config/auto_core.ini`.
   Missing or invalid values use `on` and are reported; the file is not
   created.
5. Set console output to UTF-8 (`SetConsoleOutputCP`; input CP is unchanged)
   and set the console title to Auto Core.
6. If `<installation_root>/crash/.crash` exists, prompt whether to continue. Yes removes the
   marker and continues; No exits (`1`) and leaves the marker so the next start
   asks again.
7. Install the unhandled-exception restart filter.
8. Capture `main_thread_id`, then install the low-level keyboard hook and
   shutdown listeners. Hook install is not checked. Shutdown-listener failure
   prints to stderr and is not fatal.
9. Create the log directory and write Main's local session log.
10. Initialize the generic component session from `components.list` (hello on `ac.component.v1`, child process handles, and snapshot attach when `taskbar` is enabled). A missing `logger.ini` is reported here. Built-in merge defaults stay in memory and the file is not created.
11. Load `keymap/keymap.map`. If the file is missing or unloadable, install a
   two-key emergency map in memory and report the gap. Runtime does not write
   `keymap.map`.
12. Print the ready banner on a detached thread (weekday and
   Writer's `task_list.txt`). That task section is an intentional Main
   banner, not component dispatch. A missing file is logged and the task
   section is omitted; an empty file prints "Nothing pending today."
13. Enter the thread message loop.

The message loop handles a posted shutdown request, then a posted key event,
then ordinary `TranslateMessage` / `DispatchMessage`. Exceptions in those two
handlers are printed and are not fatal; the same message still goes to
Translate/Dispatch. `GetMessage` returning `-1` ends the loop like `WM_QUIT`.
`main` does not call `close_program()` on that path. After the loop, Main writes
its local shutdown record and returns `0`. The component `Session` destructor
runs when `main` returns.

## Keyboard hook and F-lock

The hook runs on the main thread and must return quickly. Configured
`WM_KEYDOWN` events are posted as `WM_APP+96` and swallowed. Held mapped keys
auto-repeat as further posts and swallows. Other keys pass through. The
normalized key code is logged in `process_key_event`, not in the hook.

`process_key_event` runs the matching `active_keymap` binding. `primary`
selects the primary action; `activate_function_key` clears it so the next
binding uses `secondary`. After a secondary action, `primary` is restored
except when the key is numpad 0.

While an interactive taskbar cycle is active (`taskbar.switch_set`), posted
keys go to `MainTaskbarState::switch_windows` instead of the keymap. That
field is public because the hook reads it; encapsulation is tracked in
[TODO.md](TODO.md).

Numpad Enter is normalized to `key_codes::numpad_enter` (`0x100`) using
`LLKHF_EXTENDED`. `keymap.map` names are resolved by `key_codes::resolve`;
plain `enter` is not a distinct mapping.

## Keymap

`initialize_keymap()` runs after the component session and always loads
`keymap/keymap.map`. Name lookup happens only at init: each expression is
resolved into a `std::function` and stored in `active_keymap`. The hook then
calls `primary()` / `secondary()` with no further name lookup.

`keymap_editor.exe` writes a seed `keymap.map` listing every name in `key_codes::keys` when that file is missing:

- `numpad_0`: `activate_function_key` / `deactivate_function_key`
- `numpad_1`: `activate_auto_core` / `close_program`
- All other keys: blank (`key =`, unbound)

An existing `keymap.map` is never overwritten, including empty or broken
files. A file that cannot be opened, or that has no usable
rows (no resolved command on either side of any key), logs and installs an
in-memory emergency map with the same two filled bindings. That emergency
map is not written back to `keymap.map`. Runtime does not create
`keymap.map`.

During keymap initialization, Main refreshes `keymap/keymap_commands.txt`
from the command registry, including names advertised by started generic
children. Journal aliases are advertised by `journal_ac.exe` after it reads its per-factory alias files.

`keymap.map` lines are `key = primary | secondary`. `[...]` headers and `;` /
`#` comments are ignored. A `|` inside `()` or quotes is not the action
split, and a second top-level `|` is invalid. Each side is resolved on its
own: a valid name still binds when the other is empty or unknown. The seed
writes an unbound key as `key =`. `key =`, `key = |`, and
`key = primary | secondary` are the same unset pair. The word
`primary` is unset only on the primary side, and `secondary` only on the
secondary side. A swapped pair is an invalid line. A blank side loads as
empty. Both sides empty leaves the key unbound; it is not in
`active_keymap` and is not logged as invalid. A missing line is the same as
unbound. After a successful load, each `key_codes` name not in
`active_keymap` prints `numpad 2 hasn't been set` (underscore in the INI
name becomes a space), unless `silence_nonset_warning` is exactly `on`. Unbound keys are not Auto
Core's: the hook calls `CallNextHookEx`, so Windows and the focused app still
see the physical key. A key with at least one filled side is in the map and
eats the keystroke (`return 1`); an empty side is a no-op. Unknown command
expressions print `numpad 2 is set to an incorrect value: …` at load and
again on press of that side. Unknown keys and malformed lines are logged and
skipped. Zero usable rows (no resolved command), or a file that cannot be
opened, uses the emergency map. Unset messages are not printed after
emergency fallback.

Optional `config/keymap.ini` `[keymap] silence_nonset_warning` must be
exactly `on` to skip the load-time unset messages. A missing file keeps
the flag off and is reported; the file is not created. Any other value
leaves the flag off. See [configuration.md](configuration.md).

### Why keymap.map is the only map

A selectable compiled table was dropped. It saved about **34 µs** of table
fill versus about **0.9–1.5 ms** of registry + autocomplete refresh + parse
(median about **1.1 ms**), once per process start. Shared workspace and
journal-command I/O was about **1 ms** either way, so whole-init totals of
~1.1 ms vs ~2.2 ms were not the mode delta. The hook path was already the
same bound `std::function` call.

The two maps had also diverged (`keymap.ini` vs the old compiled table).
Restoring a compiled mode means putting a table fill back in front of the
registry/load path.

Measured on 2026-09-04 (`keymap_mode_benchmark.log` phase-split lines):

| Date | map | fill_us | registry_us | commands_us | load_us | comparable |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-09-04 16:30:16 | compiled fill | 34.3 | 0.0 | 0.0 | 0.0 | 34.3 µs |
| 2026-09-04 16:29:48 | keymap.ini | 0.0 | 427.3 | 94.0 | 370.8 | 892.1 µs |

Compiled `fill_us` ranged 26–40 µs. Runtime extra
(`registry_us` + `commands_us` + `load_us`) ranged 0.9–1.5 ms.

## Crash recovery

Auto Core restarts after an unhandled exception because it is the system
keyboard manager. Recovery stays off until `check_for_previous_crash()`
returns true.

Every Auto Core executable initializes the shared `auto_core.dll` crash facility
during its common Shell startup. With `[crash_recovery] crash_diagnostics = on`,
an unhandled exception creates a collision-safe UTC/executable/PID folder under
`<installation_root>/crash` containing `crash.txt` and a best-effort conservative
`crash.dmp`. Diagnostics do not use normal logging and remain enabled when
`[logging] disable_all = on`. Reports contain no copied executable, DLL, or PDB.

Main separately preserves its recovery policy: it writes
`<installation_root>/crash/.crash`, starts the installed `auto_core.exe`, then
runs noninteractive shutdown and `ExitProcess(1)`. If the marker cannot be
written, the handler does not restart. See
[configuration.md](configuration.md) for `crash_recovery.ini`.

`send_crash_command` is a diagnostic keymap name that forces this path.
`encoding_test` round-trips UTF-16 through core encoding and prints PASS/FAIL.
Neither is intended for production maps; both are still registered so
`keymap.map` can bind them.

## Shutdown

Console close, Ctrl+C, Ctrl+Break, and session end post `WM_APP+97` to the
main thread. That calls `close_program()`: set `program_closing`, remove the
keyboard hook, send v1 `shutdown` to all generic children, and supervise their
process handles under the shared `shutdown.ini` deadline. Popup mode hides and
detaches the console first; console mode keeps it for delayed-shutdown recovery.
Generic `{name}_ac.exe` children inherit
Main's console (`CreateProcess` flags `0`) so `print()` writes the same
`std::cout`. Dash and config tools keep `CREATE_NEW_CONSOLE`. A prompt
activates Main's console (Win+number when `auto_core` is in slots 1–10) or
allocates its own if attach fails. A new `auto_core.exe` waits for this
process to exit before initializing.
Title-bar close (`CTRL_CLOSE_EVENT`) is not a supported shutdown path.
Shared attachment means it can reach children. Windows may still terminate
the process about five seconds after a console close if that work has not
finished.

Main writes its local shutdown record separately from child shutdown.
Generic children, including `server_ac.exe`, receive v1 `shutdown` on their
control pipe.

## Component session

`ac::main::components::initialize()` is the RAII session started from `main`:

1. Load `components.list`. Invalid names and malformed
   values are logged; those names are ignored or disabled. A missing file
   is reconstructed by `components_editor.exe` at startup; if that fails,
   Main exits. An unreadable existing file is reported and every valid
   `*_ac.exe` beside Main is enabled. Main does not start an executable
   that embeds `AC_LAUNCH_DESCRIPTOR` as a v1 child.
2. If `taskbar` is enabled, start it and wait for hello, then attach the
   snapshot client. Snapshot failure keeps the control child.
3. Create pipes and start every other enabled `{name}_ac.exe` without
   waiting. Each child is created suspended, assigned to its own job, and
   resumed only after that assignment succeeds. If the job cannot be created
   or the child cannot be placed in it, Main terminates the still-suspended
   process and does not host that component. Wait for hellos in parallel
   (5s window). Failure disables only that child (pipe closed, job terminated).

The returned `Session` destructor calls `shutdown()` if it is still active.
Shutdown requests are sent in reverse successful-start order before Main waits
for any process. A component is finished only when its process has exited and
its job has no active processes. Main calls `TerminateJobObject` only for
children whose hello declares `force_allowed`. Graceful children are not
force-terminated: Main duplicates the job handle into the living root process,
or into a descendant opened and checked with `IsProcessInJob`, before it
closes its own handle. Closing the last of those handles still ends any
process left in the job. `logger_ac.exe --once --shutdown` is started outside
these jobs and is not waited on.

On-demand components are not started here. An enabled name whose
`{name}_ac.exe` embeds `AC_LAUNCH_DESCRIPTOR` is launched when its
command runs. Dash's descriptor asks for a new console, the foreground HWND,
and the parent PID, and does not wait. Slash's descriptor passes the command
on the command line and waits until the process exits. See
[dash.md](dash.md) for the `--target` / `--parent-pid` launch line.

Wake and server are generic v1 children. Wake has no advertised keymap
names. Server shutdown uses the control pipe.

## Generic children

Boot-time children speak [`component_protocol.ixx`](../app/shared/protocols/component_protocol.ixx).
Main forwards keymap names from each child's hello catalog and from
enabled executables that embed `AC_LAUNCH_DESCRIPTOR`. Journal aliases live in per-factory
`.list` files and are advertised by `journal_ac.exe`. A new ordinary
component does not require a Main
source edit.

Registering runtime commands and adding a new child project are in
[development.md](development.md). Per-component product docs:

- [Dash](dash.md)
- [iTunes](itunes.md)
- [Spotify](spotify.md)
- [Taskbar](taskbar.md)
