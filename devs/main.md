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
   `auto_core_settings.exe --initialize` and wait. Before its Auto Core menu, a missing
   `config/logging_policy.ini` is seeded with logging enabled and `retention_days = 10`.
   An existing file is left unchanged and resolved as saved. When this run creates
   the file, `Logging configuration initialized.` is printed with the same console
   status as the other Main configuration lines, immediately before
   `Runtime configuration initialized.`. On interactive Use defaults that print
   follows option 1. The initializer then logs
   `Auto Core initialization` when that policy allows it. `--seed` does not prompt.
   No-argument and `--init` show `Auto Core Configuration` with
   `1. Use defaults` and `2. Configure`. Use defaults propagates `--seed` and does not
   ask about logging. Configure propagates `--init` and does not attach `--seed`.
   When this run created `logging_policy.ini`, Configure asks
   `Enable Auto Core logging? [Y/n]:` before the other Main settings. Blank or Yes
   leaves logging enabled. No runs
   `auto_core_settings.exe --config --area logging-policy --disable`, which sets
   `disable_all = on` and leaves `retention_days` and every other stored setting
   unchanged. The policy command writes no log records when that update succeeds.
   The initializer then logs `Auto Core logging has been disabled.` from its cached
   enabled policy and suppresses further logging in that process. If the update fails,
   initialization prints a console error and keeps logging enabled.
   `auto_core_settings.exe --initialize` does not write the file itself.
   `--init --seed` does not ask about logging.
   `--seed` skips that menu and also skips the Components menu, launching
   `auto_core_components.exe --initialize --seed`. `--init --seed` skips the Auto Core menu,
   passes both flags to the Main configuration owners, then shows the
   Components menu. That Main mode is passed to the `auto-core`,
   `components-policy`, `shutdown`, `crash-recovery`, `startup`, and
   `warnings` routes of `auto_core_settings.exe --config`. The same mode is then
   passed to `auto_core_taskbar.exe --config` and `auto_core_writer.exe --config`.
   Use defaults passes `--seed`. Configure passes `--init`. `--init --seed` passes
   both. Those tools print `Taskbar initialized.` and then `Writer initialized.`
   before the Components menu. It does not select
   component initialization. After those owners succeed, no-argument, `--init`,
   and `--init --seed` ask `1. Use defaults for components`
   (`auto_core_components.exe --initialize --seed`), `2. Configure components`
   (`auto_core_components.exe --initialize`), or `3. Skip components`. Blank input
   selects defaults. Skip does not launch `auto_core_components.exe` and is not
   a failure. Taskbar and Writer have already run, so Skip still initializes or
   configures them. `auto_core_components.exe --initialize` asks each discovered component
   whether to use defaults, configure, or skip. `auto_core_components.exe --initialize --seed`
   does not prompt and launches each component config with `--init --seed`,
   which creates each component INI by launching `<name>_ac.exe --config --init --seed`.
   Writer is not in that pass. Its INI is `config/writer.ini`, written earlier by
   `auto_core_writer.exe --config`.
   After component initialization, `auto_core_components.exe --edit --seed` and
   `auto_core_keymap.exe --seed` run. No Main configuration owner runs after
   component initialization. A failed or cancelled required stage
   stops initialization. After the orchestrator returns, resolve the shared
   logging policy. If `logging_policy.ini` is still missing, exit `1`.
   `auto_core_settings.exe --config` writes `auto_core.ini`. Presence of the file means
   Auto Core is initialized. There is no `initialized` key. If a required
   step fails or the user cancels that step, and this run created
   `auto_core.ini`, the orchestrator removes the file before it exits.
   If the file still does not exist, exit `1`. Startup
   does not check `components.list` or `keymap.map`. A missing list is
   reported later and discovered `*_ac.exe` names are enabled in memory.
   The file is not created. A missing `keymap.map` installs the emergency
   keymap in memory and is not created.
3. If `auto_core.ini` already exists, resolve the shared logging policy
   before any other startup log. A missing `logging_policy.ini` uses built-in
   defaults (`disable_all = off`, `directory = data\logs`,
   `max_directory_mb = 25`, `retention_days = 10`, `level = info`,
   `log_console_output = on`,
   `write_logs_to_files = on`,
   `write_logs_to_console = off`, `write_alerts_to_console = on`,
   `component_logging_default = on`), prints
   `config/logging_policy.ini is missing; using built-in logging defaults.`,
   and does not create the file. After logging is resolved, if
   `[auto_core] version` differs from the compiled version, Main replaces
   only that value. A matching value is left untouched. A failed write is
   logged and is not fatal.
4. Load `[warnings] missing_winkey_mapping` and `unmapped_key` from
   `config/warnings.ini`. A missing or invalid `missing_winkey_mapping`
   uses `on`. A missing or invalid `unmapped_key` uses `off`. Either case
   is reported; the file is not created.
5. Set console output to UTF-8 (`SetConsoleOutputCP`; input CP is unchanged)
   and set the console title to Auto Core.
6. If `<installation_root>/data/crash_reports/.crash` exists, prompt whether to
   continue. Yes removes the marker and continues; No exits (`1`) and leaves the
   marker so the next start asks again.
7. Install the unhandled-exception restart filter.
8. Capture `main_thread_id`, then install the low-level keyboard hook and
   shutdown listeners. Hook install is not checked. Shutdown-listener failure
   prints to stderr and is not fatal.
9. Create the log directory and write Main's local session log.
10. Initialize the generic component session from `components.list` (hello on `ac.component.v2`, child process handles, and snapshot attach when `taskbar` is enabled). A `logger` entry is ignored. `log_merger` does not start a process. Its on/off state is resolved with this catalog load. A missing `log_merger` line, or a missing or unreadable list, leaves automatic merging on. Merge policy values come from `[log_merge]` in `logging_policy.ini`.
11. Load `<installation root>/keymap.map`. If the file is missing or unloadable, install a
   two-key emergency map in memory and report the gap. Runtime does not write
   `keymap.map`.
12. Load the sectionless `config/startup.ini`. `show_day_of_week` and
   `show_task_list` independently control the ready banner and both default to
   `on` when the file, key, or value is unavailable. Main always prints
   `Program ready` in one assembled message. The task section is an intentional
   Main banner, not component dispatch. Main opens Writer's `task_list.txt`
   only when `show_task_list` is enabled. A missing task file is logged and the
   task section is omitted; an empty file prints "Nothing pending today."
13. If `[auto_core] check_for_updates` is `on`, retrieve
   `https://raw.githubusercontent.com/djdinnebeil/autocore/main/version.txt`
   once on a background thread. A missing or invalid value uses `on`. The
   request uses a short timeout and does not delay the message loop. Published
   text is `MAJOR.MINOR.PATCH` during incremental work. The finalized release
   of that line is `MAJOR.MINOR`, and it is newer than every patch with the
   same major and minor. Major and minor are compared first, so `1.0` is older
   than `1.1` and `1.1.0`. A newer published version prints
   `[Auto Core] Update available: <installed> -> <latest>`. The installed
   version is the compiled constant. The same version,
   a newer installed version, and any failed request print nothing. `off`
   makes no request. The thread is joined after the message loop.
14. On background threads, run log cleanup and, when `log_merger` was on at
   startup, the periodic log merge inside this process. `log_merger` off does
   not start that worker. `request_stop` and `join` are safe on the idle
   worker. Cleanup uses the logging directory and limits already
   resolved for the host. The merge interval and shutdown flag come from
   `[log_merge]` in the same resolved `logging_policy.ini` and apply only
   while `log_merger` is on. Edits to the list or the policy apply after
   the host restarts. Interval `0` waits for shutdown and does not merge.
   Cleanup runs retention, then size. Main does not wait for the first pass
   before the message loop. After it returns, the thread waits until the next
   local midnight, or until shutdown, and then runs cleanup again. A positive
   interval merges once, then again after each interval. The merge mutex wait
   is interruptible. An in-flight cleanup or merge is allowed to finish.
   Routine cleanup summaries and an already-held cleanup mutex are
   `log_debug`. A directory or delete failure is `log_alert` and is not fatal.
   Merge diagnostics use the `auto_core_logger` area on the `auto_core` log.
   Shutdown joins the cleanup worker and the merge worker, including an idle
   merge worker, before Main's shutdown record.
15. Enter the thread message loop.

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

Posted keys run the matching keymap binding. Taskbar activation does not
keep a cycle session in Main.

Numpad Enter is normalized to `key_codes::numpad_enter` (`0x100`) using
`LLKHF_EXTENDED`. `keymap.map` names are resolved by `key_codes::resolve`;
plain `enter` is not a distinct mapping.

## Keymap

`initialize_keymap()` runs after the component session and always loads
`<installation root>/keymap.map`. Name lookup happens only at init: each expression is
resolved into a `std::function` and stored in `active_keymap`. The hook then
calls `primary()` / `secondary()` with no further name lookup.

`auto_core_keymap.exe --seed` writes a seed `keymap.map` listing every name in `key_codes::keys` when that file is missing:

- `numpad_0`: `activate_function_key` / `deactivate_function_key`
- `numpad_1`: `activate_auto_core` / `close_program`
- All other keys: blank (`key =`, unbound)
- A blank line, then an empty `[commands]` section. `--seed` does not export command names.

`--seed` does not overwrite an existing regular `keymap.map`, including an empty
or broken file. It creates a missing file with `CREATE_NEW`. A directory or
other non-file at that path is an error and is not replaced. A failed write
removes the new file instead of reporting success.

A no-argument `auto_core_keymap.exe` prints `Keymap commands`, runs `--seed`
only when `keymap.map` is missing, then runs `--build`. Those two calls stay
separate, so the build lock is acquired once. Success prints `Keymap commands
successfully built.` Failure prints `Keymap commands could not be built.`
The menu is `1. Rebuild keymap`, which repeats that sequence, and `2. Exit`.
An empty choice exits. `--seed`, `--build`, and `--help` keep their direct
dispatch.

`auto_core_keymap.exe --build` keeps the name-only catalog files and fills
`[commands]` from that same command-name collection. If `keymap.map` does not
exist, `--build` creates the short numpad and media template, then a blank
line and `[commands]`, and does not replace a file that appears before
publication. An existing file keeps the bytes before the single final
`[commands]` section. When that preserved text does not already end with a
blank line, the update inserts one. Extra blank lines already there stay.
The builder refuses the map update when `[commands]` is duplicated, when
another section follows it, or when a binding line is inside it, and leaves
the file unchanged. A failed read, or a file containing a NUL byte, is also
left unchanged. Catalog publication and the map update are separate. A later
failure does not roll back a file already replaced. `[commands]` is command
names only. Each per-provider catalog line is a command name, or `name - description` when
that command has a description. The merged `keymap/keymap_commands.txt` line is `name [Provider] - description`, or `name [Provider]` when the description is empty. A blank description is omitted, including
its separator. Main's startup rewrite of `keymap/keymap_commands.txt`
publishes those lines from the commands it registered.

A file that cannot be opened, or that has no usable
rows (no resolved command on either side of any key), logs and installs an
in-memory emergency map with the same two filled bindings. That emergency
map is not written back to `keymap.map`. Runtime does not create
`keymap.map`.

During keymap initialization, Main refreshes `keymap/keymap_commands.txt`
from the command registry, including names advertised by started generic
children. Journal aliases are advertised by `journal_ac.exe` after it reads its per-factory alias files.

`keymap.map` lines are `key = primary | secondary`. `[...]` headers and `;` /
`#` comments are ignored. A trimmed `[commands]` header starts a region the
binding parser ignores until the next section header or the end of the file.
Binding parsing resumes after that next header. A `|` inside `()` or quotes is not the action
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
name becomes a space), when `[warnings] unmapped_key` is `on`. Unbound keys are not Auto
Core's: the hook calls `CallNextHookEx`, so Windows and the focused app still
see the physical key. A key with at least one filled side is in the map and
eats the keystroke (`return 1`); an empty side is a no-op. Unknown command
expressions print `numpad 2 is set to an incorrect value: …` at load and
again on press of that side. A side whose command is an unresolved
`activate_commands.map` selection does not print that diagnostic at load or
on press. Its action still runs on press, and the side does not count as a
usable entry. A blank selection prints `No provider selected for <command>.
Set a provider in keymap\activate_commands.map, then restart Auto Core.` An
invalid or unavailable provider prints `The provider "<value>" selected for
<command> is unavailable. Check the provider choices in
keymap\activate_commands.map, then restart Auto Core.` The command name and
saved value come from the activation resolver. Pending `<key>.map` entries
still open setup. A command the resolver did not leave unresolved still uses
the incorrect-value diagnostic. Resolver startup reports are unchanged.
Unknown keys and malformed lines are logged and
skipped. Zero usable rows (no resolved command), or a file that cannot be
opened, uses the emergency map. Unset messages are not printed after
emergency fallback.

`config/warnings.ini` `[warnings] unmapped_key` defaults to `off`, which
skips the load-time unset messages. Only exact `on` prints them. A missing
file keeps the warning off and is reported; the file is not created. Any
other value leaves the warning off. See [configuration.md](configuration.md).

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
`<installation_root>/data/crash_reports` containing `crash.txt` and a
best-effort conservative `crash.dmp`. Diagnostics do not use normal logging and remain enabled when
`[logging] disable_all = on`. Reports contain no copied executable, DLL, or PDB.

Main separately preserves its recovery policy: it writes
`<installation_root>/data/crash_reports/.crash`, starts the installed
`auto_core.exe`, then runs noninteractive shutdown and `ExitProcess(1)`. If the marker cannot be
written, the handler does not restart. See
[configuration.md](configuration.md) for `crash_recovery.ini`.

`encoding_test` round-trips UTF-16 through core encoding and prints PASS/FAIL.
It is not intended for production maps but remains registered so `keymap.map`
can bind it.

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
   values are logged; those names are ignored or disabled. A missing or
   unreadable file is reported. Main enables every valid `*_ac.exe` beside
   it in memory and does not create the file. It does not launch
   `auto_core_components.exe`. Main does not start an executable
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
process left in the job. After those writers have stopped, and when the startup `log_merger` state is
on and `merge_logs_on_shutdown` is on, `auto_core_logger.exe --merge --shutdown`
is started outside these jobs and is not waited on. `log_merger` off skips
that launch and does not change the saved policy. A `logger` line in
`components.list` does not start a process. `log_merger` does not either.

On-demand components are not started here. An enabled name whose
`{name}_ac.exe` embeds `AC_LAUNCH_DESCRIPTOR` is launched when its
command runs. Dash's descriptor asks for a new console, the foreground HWND,
and the parent PID, and does not wait. The launch is
`dash_ac.exe --run --target <hwnd> --parent-pid <pid>`. Slash's descriptor passes the command
on the command line and waits until the process exits. See
[dash.md](../docs/manual/components/dash.md) for the `--target` / `--parent-pid` launch line.

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

- [Dash](../docs/manual/components/dash.md)
- [iTunes](../docs/manual/components/itunes.md)
- [Spotify](../docs/manual/components/spotify.md)
- [Taskbar](../docs/manual/components/taskbar.md)
