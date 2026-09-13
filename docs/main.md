# Main executable

`auto_core.exe` is the Auto Core keyboard manager. It lives in `app/main`,
links `auto_core.lib`, and loads `auto_core.dll`. Child product behavior is
documented with each component; this page covers the Main process only.

## Startup order

[`main.cxx`](../app/main/src/main.cxx) runs this sequence:

1. Load `config/auto_core.ini` (write portable defaults if the file is missing).
   A missing file or invalid `warn_without_winkey_mapping` uses `true` and is
   not fatal.
2. Set console output to UTF-8 (`SetConsoleOutputCP`; input CP is unchanged)
   and set the console title to Auto Core.
3. If `<exe>/crash/.crash` exists, prompt whether to continue. Yes removes the
   marker and continues; No exits (`1`) and leaves the marker so the next start
   asks again.
4. Install the unhandled-exception restart filter.
5. Capture `main_thread_id`, then install the low-level keyboard hook and
   shutdown listeners. Hook install is not checked. Shutdown-listener failure
   prints to stderr and is not fatal.
6. Start `logger_ac.exe` when central logging is enabled, then connect.
7. Initialize the component session (`taskbar_ac.exe`, pipes, child processes).
8. Load `keymap/bindings.ini`. If the file is missing, write a seed of every
   `key_codes` name (`numpad_0` / `numpad_1` filled, others `{, }`). Workspace
   or file-load failure installs a two-key emergency map in memory and does
   not rewrite an existing file.
9. Print the ready banner on a detached thread (weekday and
   `writer/task_list.txt`). A missing file is logged and the task section is
   omitted; an empty file prints "Nothing pending today."
10. Enter the thread message loop.

The message loop handles a posted shutdown request, then a posted key event,
then ordinary `TranslateMessage` / `DispatchMessage`. Exceptions in those two
handlers are printed and are not fatal; the same message still goes to
Translate/Dispatch. `GetMessage` returning `-1` ends the loop like `WM_QUIT`.
`main` does not call `close_program()` on that path. After the loop, Main shuts
down the logger and returns `0`. The component `Session` destructor runs when
`main` returns.

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
`LLKHF_EXTENDED`. `bindings.ini` names are resolved by `key_codes::resolve`;
plain `enter` is not a distinct mapping.

## Keymap

`initialize_keymap()` runs after the component session and always loads
`keymap/bindings.ini`. Name lookup happens only at init: each expression is
resolved into a `std::function` and stored in `active_keymap`. The hook then
calls `primary()` / `secondary()` with no further name lookup.

Workspace init creates `keymap/` and `keymap/components/`. If
`bindings.ini` is missing, it writes a seed listing every name in
`key_codes::keys`:

- `numpad_0`: `activate_function_key` / `deactivate_function_key`
- `numpad_1`: `activate_auto_core` / `close_program`
- All other keys: `{, }` (unbound)

An existing `bindings.ini` is never overwritten, including empty or broken
files. Workspace failure, or a file that cannot be opened or has no usable
rows (no resolved command on either side of any key), logs and installs an
in-memory emergency map with the same two filled bindings. That emergency
map is not written back to `bindings.ini`.

After a successful workspace, Main loads `journal_choices.ini` from the
configured journal data directory,
then rewrites `keymap/components/journal.keymap_commands.txt` (keeps factory
templates already in the file, then protocol names plus parsed alias names,
sorted) and refreshes `keymap/keymap_commands.txt` from the command registry
plus those alias names. Malformed or reserved alias lines are logged and
omitted from both files.

`bindings.ini` lines are `key = {primary, secondary}`. An optional `[keymap]`
header and `;` / `#` comments are ignored. A comma inside `()` or quotes is
not the action split. Each side is resolved on its own: a valid name still
binds when the other is empty or unknown. `{, }` (both actions empty after
trim) leaves the key unbound; it is not in `active_keymap` and is not logged
as invalid. A missing line is the same as `{, }`. After a successful load,
each `key_codes` name not in `active_keymap` prints `numpad 2 hasn't been
set` (underscore in the INI name becomes a space), unless
`silence_nonset_warning` is exactly `true`. Unbound keys are not Auto
Core's: the hook calls `CallNextHookEx`, so Windows and the focused app still
see the physical key. A key with at least one filled side is in the map and
eats the keystroke (`return 1`); an empty side is a no-op. Unknown command
expressions print `numpad 2 is set to an incorrect value: …` at load and
again on press of that side. Unknown keys and malformed lines are logged and
skipped. Zero usable rows (no resolved command), or a file that cannot be
opened, uses the emergency map. Unset messages are not printed after
emergency fallback.

Optional `config/keymap.ini` `[keymap] trace_enabled` must be
exactly `true` to log each binding as it is created. `[keymap]
silence_nonset_warning` must be exactly `true` to skip the load-time unset
messages. A missing file is written once with both flags `false`. Missing
file or any other value leaves both flags off. See
[configuration.md](configuration.md).

### Why bindings.ini is the only map

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
ignored), starts a new `auto_core.exe`, then runs `close_program()`, shuts
down the logger, and `ExitProcess(1)`. If the marker or artifact directory
cannot be written, the handler does not restart. See
[configuration.md](configuration.md) for `crash_recovery.ini`.

`send_crash_command` is a diagnostic keymap name that forces this path.
`encoding_test` round-trips UTF-16 through core encoding and prints PASS/FAIL.
Neither is intended for production maps; both are still registered so
`bindings.ini` can bind them.

## Shutdown

Console close, Ctrl+C, Ctrl+Break, and session end post `WM_APP+97` to the
main thread. That calls `close_program()`: set `program_closing`, stop the
local server, send pipe shutdowns to iTunes, Spotify, journal, wake, and
writer, stop `taskbar_ac.exe`, unhook the keyboard, and post `WM_QUIT`.
Windows may still terminate the process about five seconds after a console
close (`CTRL_CLOSE_EVENT`) if that work has not finished.

Logger shutdown is separate: Main requests a graceful stop, waits six
seconds, then terminates `logger_ac.exe` if needed. `server_ac.exe` is stopped with
`TerminateProcess`.

## Component session

`ac::main::components::initialize()` is the RAII session started from `main`:

1. Start `taskbar_ac.exe`, wait up to five seconds for `taskbar_ready`, then up
   to five seconds to connect to the snapshot authority. Failure logs and
   continues without native Win+position.
2. Create pipe servers for journal, Spotify, iTunes, wake, and writer.
3. Start those children and `server_ac.exe`.
4. Wait up to five seconds for `journal_ready` and `writer_ready`. Writer
   ready-wait failure is logged. Keymap still registers writer names; invoke
   and shutdown check `pipe.valid()`, and shutdown `reset()`s the handle.
   There is no session flag that skips registration. The journal ready result
   is ignored; a timeout is only visible if `wait_for_journal_ready` itself
   logged.

The returned `Session` destructor stops the taskbar client only. Pipe
shutdowns and `stop_server()` run from `close_program()` before `WM_QUIT`.

Dash is not started here; `launch_dash` runs on demand. See
[dash.md](dash.md) for the `--target` / `--parent-pid` launch line.

Slash is launched per recycle-bin command and has no long-lived pipe. Runtime
names come from `keymap/components/slash.keymap_commands.txt`, or from the protocol `all` list
(`report_and_empty_recycle_bin`) if that file is missing. The wait is infinite
on the main thread, so a Slash command blocks keyboard dispatch until
`slash_ac.exe` exits.

Wake has no ready-wait and no Main keymap names. Main creates `wake_pipe`,
starts `wake_ac.exe`, and at shutdown sends only the protocol `shutdown`
command.

## Pipe children

iTunes and Spotify have no ready-wait. Main creates `ac_itunes_pipe` /
`ac_spotify_pipe`, starts `itunes_ac.exe` / `spotify_ac.exe`, and sends named invokes
from the keymap. Spotify serializes invoke and shutdown on one mutex. iTunes
mutexes named invoke only; integer shutdown is not serialized (see
[app/main/TODO.md](../app/main/TODO.md)). Runtime names come from
`keymap/components/itunes.keymap_commands.txt` /
`keymap/components/spotify.keymap_commands.txt`, or from the
protocol `all` lists if those files are missing. Main also registers
`print_next_up_song_list` as an alias for `itunes_print_next_up`.

Journal waits up to five seconds for `journal_ready`. Parameterized keymap
names go through factories that forward `make_print_choice(...)` and
`print_and_insert_into_journal(...)` expressions on `ac_journal_pipe`.
`launch_journal_config` is Main-local: it starts `journal_config.exe` with
`CREATE_NEW_CONSOLE` and is not a pipe request. Person-name print-choice
aliases live in `journal_choices.ini` under the configured journal data
directory and expand only when
`bindings.ini` names them.

Writer waits up to five seconds for `writer_ready`. Invoke is by command
name on `ac_writer_pipe`. After shutdown the pipe handle is reset.

Registering runtime commands and adding a new child project are in
[development.md](development.md). Per-component contracts:

- [Dash](dash.md)
- [iTunes](itunes.md)
- [Spotify](spotify.md)
- [Taskbar](taskbar.md)
- Journal protocol: [`journal_protocol.ixx`](../app/shared/protocols/journal_protocol.ixx)
- Writer protocol: [`writer_protocol.ixx`](../app/shared/protocols/writer_protocol.ixx)
