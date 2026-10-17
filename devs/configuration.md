# Configuration

Component configuration is `config/components/<name>.ini`, written only by `<name>_config.exe`. Main host files stay directly under `config/` and are written only by their own `_config.exe`. `auto_core.exe` and `<name>_ac.exe` read configuration and do not create or rewrite it.

A launch with no arguments opens that owner's settings menu. `--seed` writes compiled defaults only when the file is missing. `--init` is interactive initialization. `--initialize` is an alias of `--init`. When both flags are set, seed runs first. Owners then return without prompting, except `spotify_config.exe`, which continues into client ID and OAuth. `logging_policy_config.exe --disable` writes a missing file with `disable_all = on` and does not change an existing file.

A missing or unreadable file, a missing key, or an invalid value uses that key's compiled default in memory. Another valid key in the same file still applies. Readers do not repair the file.

`auto_core.ini` is created during initialization and retained as the sentinel only if the required initialization sequence succeeds. There is no `initialized` key. The startup sequence is in [main.md](main.md#startup-order).

Boolean switches are `on` and `off`. `default_response` uses `yes` and `no` because it is an answer. `components_policy.ini` compares `new_components`, `sort_components`, and `remove_missing_components` case-insensitively. Other boolean parsers require exact `on` or `off`.

A relative path resolves from the installation root (`dist/` for a normal build). An absolute path is used directly. The process working directory and `bin/` are not the base.

Compiled default text for a component lives in that child's `shared/defaults.ixx`. Main host default text lives in [`app/main/shared/defaults.ixx`](../app/main/shared/defaults.ixx).

## Main configuration

### `config/auto_core.ini`

Writer `auto_core_config.exe`. Section `[auto_core]`.

| Key | Type | Allowed | Default | Invalid or missing |
| --- | --- | --- | --- | --- |
| `logging` | bool | `on`, `off` | `on` | uses `component_logging_default` |

### `config/components_policy.ini`

Writer `components_policy_config.exe`. Section `[settings]`. This file is component-system policy. It is not `components.list`.

| Key | Type | Allowed | Default | Invalid or missing |
| --- | --- | --- | --- | --- |
| `new_components` | enum | `prompt`, `on`, `off` | `on` | an unrecognized value becomes `prompt` |
| `sort_components` | bool | `on`, `off` | `on` | uses the current or compiled default |
| `remove_missing_components` | bool | `on`, `off` | `off` | uses the current or compiled default |

### `config/crash_recovery.ini`

Writer `crash_recovery_config.exe`. Section `[crash_recovery]`.

| Key | Type | Allowed | Default | Invalid or missing |
| --- | --- | --- | --- | --- |
| `default_response` | answer | `yes`, `no` | `no` | uses `no` |
| `crash_diagnostics` | bool | `on`, `off` | `on` | uses `on` |

### `config/shutdown.ini`

Writer `shutdown_config.exe`. Section `[shutdown]`.

| Key | Type | Allowed | Default | Invalid or missing |
| --- | --- | --- | --- | --- |
| `delayed_shutdown_prompt` | enum | `popup`, `console` | `popup` | uses `popup` |
| `shutdown_timeout_ms` | integer | milliseconds | `2000` | uses `2000` |

### `config/logging_policy.ini`

Writer `logging_policy_config.exe`. Section `[logging]`. This is the shared logging policy for processes that link `auto_core.dll`. Sink and merge behavior is in [STRATEGY.md](STRATEGY.md) and [main.md](main.md).

| Key | Type | Allowed | Default | Invalid or missing |
| --- | --- | --- | --- | --- |
| `disable_all` | bool | `on`, `off` | `off` | uses `off`. On suppresses diagnostic logging and the `print()` / `printnl()` file copy. Console output stays available. This is the logging-disable control |
| `directory` | path | relative to the installation root, or absolute | `logs` | empty uses `<installation_root>/logs` |
| `max_directory_mb` | integer | non-negative | `100` | uses `100`. `0` is valid and does not disable logging |
| `retention_days` | integer | non-negative | `30` | uses `30`. `0` is valid and does not disable logging |
| `level` | enum | `trace`,`debug`, `info`, `alert` | `debug` | uses `debug` |
| `log_console_output` | bool | `on`, `off` | `on` | uses `on`. Allows `print()` and `printnl()` to participate in the component file sink |
| `write_logs_to_files` | bool | `on`, `off` | `on` | uses `on`. Master file-output gate for component logging, including eligible `print()` / `printnl()` copies |
| `write_logs_to_console` | bool | `on`, `off` | `off` | uses `off`. Mirrors enabled diagnostic logs to the console |
| `write_alerts_to_console` | bool | `on`, `off` | `on` | uses `on`. Shows enabled alerts on the console. Alerts already mirrored to the console are written once |
| `component_logging_default` | bool | `on`, `off` | `on` | uses `on` |
| `experimental` | bool | `on`, `off` | `on` | uses `on`. Temporarily selects the cleanup utility's visible startup report (`on`) or debug-log report (`off`) |

`auto_core_logging_cleanup.exe` currently reads and reports `experimental`,
`max_directory_mb`, and `retention_days` after Main reaches ready state; it
does not yet perform cleanup.

### `config/warnings.ini`

Writer `warnings_config.exe`. Section `[warnings]`.

| Key | Type | Allowed | Default | Invalid or missing |
| --- | --- | --- | --- | --- |
| `missing_winkey_mapping` | bool | `on`, `off` | `on` | uses `on`. Only exact `off` disables the warning |
| `unmapped_key` | bool | `on`, `off` | `on` | uses `on`. Only exact `off` disables the warning |

## Components

Each configurable component owns `config/components/<component>.ini` through `<component>_config.exe`. Component keys are documented in the component manuals linked from [configuration.md](../docs/manual/configuration.md#component-configuration).

## Editable runtime data

`components.list` and `keymap.map` are not INI files. `components_editor.exe --seed` creates a missing `components.list` and leaves an existing list unchanged. `keymap_editor.exe` creates a missing seed `keymap.map` and leaves an existing map unchanged. List format and keymap grammar are in [main.md](main.md) and [persistent-state.md](persistent-state.md).
