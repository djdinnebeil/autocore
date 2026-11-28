# Configuration

Component configuration is `config/components/<name>.ini`, written only by `<name>_ac.exe --config`. Main host files stay directly under `config/` and are written by the corresponding route of `auto_core_settings.exe --config`. `auto_core.exe` and `<name>_ac.exe` read configuration and do not create or rewrite it, except that Main startup replaces a differing `[auto_core] version` with the compiled constant. That write does not change any other line.

For Main configuration, `auto_core_settings.exe --config` with no arguments opens the seven-area menu; `--area <route>` opens one area directly. For all configuration owners, `--seed` writes compiled defaults only when the file is missing. `--init` is interactive initialization. `--initialize` is an alias of `--init`. When both flags are set, seed runs first. Owners then return without prompting, except `spotify_ac.exe --config`, which continues into client ID and OAuth. `auto_core_settings.exe --config --area logging-policy --disable` sets `disable_all = on`. A missing file is created from compiled defaults with that value and `retention_days = 10`. An existing file changes only `disable_all`. A successful update writes no log records.

A missing or unreadable file, a missing key, or an invalid value uses that key's compiled default in memory. Another valid key in the same file still applies. Readers do not repair the file. The startup update of a differing `[auto_core] version` is the exception.

`auto_core.ini` is created during initialization and retained as the sentinel only if the required initialization sequence succeeds. There is no `initialized` key. The startup sequence is in [main.md](main.md#startup-order).

Boolean switches are `on` and `off`. `default_response` uses `yes` and `no` because it is an answer. `components_policy.ini` compares `new_components`, `sort_components`, and `remove_missing_components` case-insensitively. Other boolean parsers require exact `on` or `off`.

A relative path resolves from the installation root (`dist/` for a normal build). An absolute path is used directly. The process working directory and `app/` are not the base.

Compiled default text for a component lives in that child's `shared/defaults.ixx`. Main host default text lives in [`app/main/shared/defaults.ixx`](../app/main/shared/defaults.ixx).

## Main configuration

### `config/startup.ini`

Writer `auto_core_settings.exe --config --area startup`. This file is sectionless.

| Key | Type | Allowed | Default | Invalid or missing |
| --- | --- | --- | --- | --- |
| `show_day_of_week` | bool | `on`, `off` | `on` | uses `on` |
| `show_task_list` | bool | `on`, `off` | `on` | uses `on` |

When task-list display is off, Main does not open Writer's `task_list.txt`.
When it is on, an empty task list displays `Nothing pending today.`

### `config/auto_core.ini`

Writer `auto_core_settings.exe --config --area auto-core`. Section `[auto_core]`.

| Key | Type | Allowed | Default | Invalid or missing |
| --- | --- | --- | --- | --- |
| `logging` | bool | `on`, `off` | `on` | uses `component_logging_default` |
| `version` | version | compiled `MAJOR.MINOR` or `MAJOR.MINOR.PATCH` | `1.0.1` | uses `1.0.1` |
| `check_for_updates` | bool | `on`, `off` | `on` | uses `on` |

`version` records the compiled Auto Core version. Incremental releases use `MAJOR.MINOR.PATCH`. The finalized release of that line uses `MAJOR.MINOR` and is newer than every patch with the same major and minor. The `auto-core` route writes the compiled constant into a new file and, when it opens an existing file, updates only `[auto_core] version` when the text differs. `auto_core.exe` does the same update at startup. `logging`, `check_for_updates`, and every other line stay as stored. The GitHub comparison uses the compiled constant and accepts either form in `version.txt`. When `check_for_updates` is `on`, `auto_core.exe` retrieves `version.txt` from GitHub once after the ready banner and prints one line if that version is newer. `off` makes no request. A failed request does not change startup.

### `config/components_policy.ini`

Writer `auto_core_settings.exe --config --area components-policy`. Section `[settings]`. This file is component-system policy. It is not `components.list`.

| Key | Type | Allowed | Default | Invalid or missing |
| --- | --- | --- | --- | --- |
| `new_components` | enum | `prompt`, `on`, `off` | `on` | an unrecognized value becomes `prompt` |
| `sort_components` | bool | `on`, `off` | `on` | uses the current or compiled default |
| `remove_missing_components` | bool | `on`, `off` | `off` | uses the current or compiled default |

### `config/crash_recovery.ini`

Writer `auto_core_settings.exe --config --area crash-recovery`. Section `[crash_recovery]`.

| Key | Type | Allowed | Default | Invalid or missing |
| --- | --- | --- | --- | --- |
| `default_response` | answer | `yes`, `no` | `no` | uses `no` |
| `crash_diagnostics` | bool | `on`, `off` | `on` | uses `on` |

### `config/shutdown.ini`

Writer `auto_core_settings.exe --config --area shutdown`. Section `[shutdown]`.

| Key | Type | Allowed | Default | Invalid or missing |
| --- | --- | --- | --- | --- |
| `delayed_shutdown_prompt` | enum | `popup`, `console` | `popup` | uses `popup` |
| `shutdown_timeout_ms` | integer | milliseconds | `2000` | uses `2000` |

### `config/logging_policy.ini`

Writer `auto_core_settings.exe --config --area logging-policy`. Section `[logging]`. This is the shared logging policy for processes that link `auto_core.dll`. Sink and merge behavior is in [STRATEGY.md](STRATEGY.md) and [main.md](main.md).

| Key | Type | Allowed | Default | Invalid or missing |
| --- | --- | --- | --- | --- |
| `disable_all` | bool | `on`, `off` | `off` | uses `off`. On suppresses diagnostic logging and the `print()` / `printnl()` file copy. Console output stays available. This is the logging-disable control |
| `directory` | path | relative to the installation root, or absolute | `data\logs` | empty uses `<installation_root>/data/logs` |
| `max_directory_mb` | integer | non-negative | `25` | uses `25`. `0` keeps logs named with today's local date and does not disable logging. Other `.log` files are deleted oldest-first until the directory is within the cap. Protected logs still count, so the cap can remain exceeded |
| `retention_days` | integer | non-negative | `10` | uses `10`. Elapsed UTC days. A file is eligible when its last write time is strictly earlier than now minus this many 24-hour periods. `0` uses now as the cutoff. Logs named with today's local date are kept either way |
| `level` | enum | `trace`,`debug`, `info`, `alert` | `info` | uses `info` |
| `log_console_output` | bool | `on`, `off` | `on` | uses `on`. Allows `print()` and `printnl()` to participate in the component file sink |
| `write_logs_to_files` | bool | `on`, `off` | `on` | uses `on`. Master file-output gate for component logging, including eligible `print()` / `printnl()` copies |
| `write_logs_to_console` | bool | `on`, `off` | `off` | uses `off`. Mirrors enabled diagnostic logs to the console |
| `write_alerts_to_console` | bool | `on`, `off` | `on` | uses `on`. Shows enabled alerts on the console. Alerts already mirrored to the console are written once |
| `component_logging_default` | bool | `on`, `off` | `on` | uses `on` |

`auto_core.exe` reads `directory`, `max_directory_mb`, and `retention_days`
from the logging configuration resolved at host startup. Edits to
`logging_policy.ini` apply after the host restarts. Cleanup runs on a
background thread after Main reaches ready state, then again at each local
midnight. One megabyte is 1,048,576 bytes. Retention runs before size cleanup.
Summaries are `log_debug` records on the `auto_core` component, so
`[logging] level` must be `trace` or `debug` for them to be written. Directory
and delete failures are `log_alert`.

`[log_merge]` follows `[logging]` in the same file.

| Key | Type | Allowed | Default | Invalid or missing |
| --- | --- | --- | --- | --- |
| `merge_interval_seconds` | integer | non-negative | `5` | uses `5` when the section or key is missing or invalid. `0` disables the periodic merge. An explicit value, including `0` and `60`, is kept when the file is rewritten |
| `merge_logs_on_shutdown` | bool | `on`, `off` | `on` | uses `on`. Independent of the interval. Both keys apply only when `components.list` enables `log_merger` |

`auto_core_settings.exe --config --area logging-policy` writes both sections.
It does not write `log_merger`. Periodic merging runs in `auto_core.exe` when
that list entry is on, blank, or absent. Off skips the periodic worker and the
detached shutdown merge and leaves these values saved. `auto_core_logger.exe --refresh` and
`--merge` perform on-demand merges in either list state. Bytes written after a pass snapshots its
inputs are published on a later pass.

### `config/warnings.ini`

Writer `auto_core_settings.exe --config --area warnings`. Section `[warnings]`.

| Key | Type | Allowed | Default | Invalid or missing |
| --- | --- | --- | --- | --- |
| `missing_winkey_mapping` | bool | `on`, `off` | `on` | uses `on`. Only exact `off` disables the warning |
| `unmapped_key` | bool | `on`, `off` | `off` | uses `off`. Only exact `on` enables the warning |

## Components

Each configurable component owns `config/components/<component>.ini` and writes it with `<component>_ac.exe --config`. Component keys are documented in the component manuals linked from [configuration.md](../docs/manual/configuration.md#component-configuration).

## Editable runtime data

`components.list` and `keymap.map` are not INI files. `auto_core_components.exe --edit --seed` creates a missing `components.list` and leaves an existing list unchanged. List updates take an exclusive byte-range lock on the persistent `<installation root>/data/auto_core/components.list.lock` file, from the authoritative read through publication. A missing `data/auto_core` directory is created before that file is opened. If the directory cannot be created, the update fails before the list is read or published. Prompts release that lock and the update is recomputed after it is acquired again. The new bytes are written to a uniquely named temporary file in the same directory with `CREATE_NEW`, flushed, and closed. An existing list is published with `ReplaceFileW` and flags 0. A missing list is published with `MoveFileExW` and flags 0. `REPLACEFILE_WRITE_THROUGH` is not used. A failure before publication removes that unfinished temporary file and leaves `components.list` unmodified. A publication failure keeps the temporary file, does not delete `components.list`, and reports both paths. It does not guarantee that the original pathname still names the previous file. There is no automatic recovery, regeneration, or power-loss guarantee. Readers of `components.list` open it with delete sharing. A failed read is not an empty catalog. `auto_core_keymap.exe --seed` creates a missing `<installation root>/keymap.map` with `CREATE_NEW`, including a blank line and an empty `[commands]` section, and leaves an existing regular file unchanged. `auto_core_keymap.exe --build` updates only the final `[commands]` name list, inserting one blank line before it when the preserved text does not already end with one, or creates the short template plus that section when `keymap.map` does not exist. A no-argument launch seeds a missing map, runs that build once, and then opens a rebuild-or-exit menu. An empty choice exits. A directory or other non-file at that path is an error. List format and keymap grammar are in [main.md](main.md) and [persistent-state.md](persistent-state.md).
