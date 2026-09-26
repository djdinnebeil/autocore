# Main executable

`auto_core.exe` is the Auto Core keyboard manager. It lives in
`app/main/runtime`, links `auto_core.lib`, and loads `auto_core.dll`. Child
product behavior is documented with each component; this page covers the Main
process only.

## Startup order

[`main.cxx`](../app/main/runtime/src/main.cxx) runs this sequence:

1. Wait for any previous `auto_core.exe` to exit (`Local\AutoCore.main`
   mutex, held until this process ends).
2. If `config/auto_core.ini` is missing, launch `auto_core_config.exe` and wait.
   That helper runs the five configuration programs, prompts for
   `warn_without_winkey_mapping`, and writes `auto_core.ini` only after they
   succeed. Presence of the file means Auto Core is initialized. There is no
   `initialized` key. If the file still does not exist, exit `1`. If
   `components.list` is missing, launch `components_editor.exe` (full catalog
   reconstruction) and wait. Main does not write that file. If the list is
   still missing, exit `1`.
3. Load `[auto_core] warn_without_winkey_mapping` from `config/auto_core.ini`.
   Missing or invalid values use `true` and are reported; the file is not
   created.
4. Set console output to UTF-8 (`SetConsoleOutputCP`; input CP is unchanged)
   and set the console title to Auto Core.
5. If `<exe>/crash/.crash` exists, prompt whether to continue. Yes removes the
   marker and continues; No exits (`1`) and leaves the marker so the next start
   asks again.
6. Install the unhandled-exception restart filter.
7. Capture `main_thread_id`, then install the low-level keyboard hook and
   shutdown listeners. Hook install is not checked. Shutdown-listener failure
   prints to stderr and is not fatal.
8. Create the log directory and write Main's local session log.
9. Initialize the generic component session from `components.list` (hello on `ac.component.v1`, child process handles, and snapshot attach when `taskbar` is enabled).
10. Load `keymap/keymap.map`. If the file is missing or unloadable, install a
   two-key emergency map in memory and report the gap. Runtime does not write
   `keymap.map`.
11. Print the ready banner on a detached thread (weekday and
   `writer/task_list.txt`). A missing file is logged and the task section is
   omitted; an empty file prints "Nothing pending today."
12. Enter the thread message loop.

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
files. Workspace failure, or a file that cannot be opened or has no usable
rows (no resolved command on either side of any key), logs and installs an
in-memory emergency map with the same two filled bindings. That emergency
map is not written back to `keymap.map`. Runtime does not create
`keymap.map`.

After a successful workspace, Main refreshes `keymap/keymap_commands.txt`
from the command registry, including names advertised by started generic
children. Journal aliases are advertised by `journal_ac.exe`.

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
name becomes a space), unless `silence_nonset_warning` is exactly `true`. Unbound keys are not Auto
Core's: the hook calls `CallNextHookEx`, so Windows and the focused app still
see the physical key. A key with at least one filled side is in the map and
eats the keystroke (`return 1`); an empty side is a no-op. Unknown command
expressions print `numpad 2 is set to an incorrect value: …` at load and
again on press of that side. Unknown keys and malformed lines are logged and
skipped. Zero usable rows (no resolved command), or a file that cannot be
opened, uses the emergency map. Unset messages are not printed after
emergency fallback.

Optional `config/keymap.ini` `[keymap] silence_nonset_warning` must be
exactly `true` to skip the load-time unset messages. A missing file keeps
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
registry/load path. The compiled table lived in `keymap_hardcoded.ixx`, which
is no longer part of the project.

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

On a crash, Main writes `<exe>/crash/.crash`, copies `auto_core.exe` and
`symbols/auto_core.pdb` into a dated `crash/<date>_crash` folder with
`crash.log` (a same-day collision uses `<date>_crash_N`; copy failures are
ignored), starts a new `auto_core.exe`, then runs `close_program()`, writes
the local shutdown record, and `ExitProcess(1)`. If the marker or artifact directory
cannot be written, the handler does not restart. See
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
   `*_ac.exe` beside Main is enabled. Known specials (`dash` and
   `slash`) are not started as v1 children.
2. If `taskbar` is enabled, start it and wait for hello, then attach the
   snapshot client. Snapshot failure keeps the control child.
3. Create pipes and start every other enabled `{name}_ac.exe` without
   waiting. Wait for those hellos in parallel (5s window). Failure disables
   only that child (pipe closed, process terminated).

The returned `Session` destructor calls `shutdown()` if it is still active.
Shutdown requests are sent in reverse successful-start order before Main waits
for any process. Main only offers OS termination for children whose hello
declares `force_allowed`; graceful children can be left running.

Dash is not started here; `launch_dash` runs on demand when `dash` is
enabled in `components.list`. See
[dash.md](dash.md) for the `--target` / `--parent-pid` launch line.

Slash is launched per recycle-bin command when `slash` is listed enabled
and has no long-lived pipe.

Wake and server are generic v1 children. Wake has no advertised keymap
names. Server shutdown uses the control pipe.

## Generic children

Boot-time children speak [`component_protocol.ixx`](../app/shared/protocols/component_protocol.ixx).
Main forwards keymap names from each child's hello catalog. Journal aliases
live in `journal_choices.ini` and are advertised by `journal_ac.exe`.
`launch_journal_config` is Main-local.

Registering runtime commands and adding a new child project are in
[development.md](development.md). Per-component product docs:

- [Dash](dash.md)
- [iTunes](itunes.md)
- [Spotify](spotify.md)
- [Taskbar](taskbar.md)
