# Component command contract

Every component, including a third-party component, selects an operation from an explicit first argument of `<name>_ac.exe`. There is no allowlist and no separate `<name>_config.exe` or `<name>_settings.exe`. Auto Core discovers `*_ac.exe` in the component executable directory and launches `--run`, `--settings`, or `--config` for every valid component name. That directory is `<installation root>\components`, including `bin\components`.

## Logger

| Invocation | Behavior |
| --- | --- |
| `logger_ac.exe` | If `config/components/logger.ini` exists, or that check fails, open the settings menu. If the file is missing, start `logger_ac.exe --config --init`, wait, and open settings only when that process exits 0. A non-zero initialization exit code is returned as-is. |
| `logger_ac.exe --settings` | Open the settings menu, including Initialize when the INI is absent. This does not run initialization by itself. |
| `logger_ac.exe --config` | The configuration tool. `--init`, `--initialize`, `--seed`, both flags, and no further arguments keep their previous meanings. This command exits. It does not open settings. |
| `logger_ac.exe --run` | The previous runtime. Further arguments are forwarded unchanged: `--once`, `--shutdown` (a label only), and `--export-keymap-commands`. A bare `--run` connects to `ac_logger_pipe` when Main created it, and otherwise performs one merge. A missing INI still uses the built-in merge interval and does not create the file. |

The operation is only the first argument. `--once` without `--run` is rejected. An unknown first argument prints a usage line and exits 1.

Settings and configuration keep their log identities, `logger_settings` and `logger_config`. The runtime identity remains `logger`.

## Dash

| Invocation | Behavior |
| --- | --- |
| `dash_ac.exe` | If `config/components/dash.ini` exists, or that check fails, open the settings menu. If the file is missing, start `dash_ac.exe --config --init`, wait, and open settings only when that process exits 0. |
| `dash_ac.exe --settings` | Open the settings menu. This does not run initialization by itself. Manage secrets starts a separate `dash_ac.exe --editor`. |
| `dash_ac.exe --editor` | The secret editor. It keeps `Local\AutoCoreDashEditor` and ignores extra arguments. It does not take the runtime mutex. |
| `dash_ac.exe --config` | The configuration tool. `--init`, `--initialize`, `--seed`, both flags, and no further arguments keep their previous meanings. Logging stays off unless the file changes it. This command exits. It does not open settings or the editor. |
| `dash_ac.exe --run` | The secret menu. Wide arguments are preserved: `--target`, `--parent-pid`, and `--export-keymap-commands`. A missing INI is reported and is not created. The runtime mutex stays `Local\AutoCoreDashInstance`. |

A no-argument launch does not open the secret menu. Main still registers `launch_dash` and starts `dash_ac.exe --run --target <hwnd> --parent-pid <pid>` in a new console. Log identities stay `dash`, `dash_settings`, `dash_config`, and `dash_editor`.

## Slash

| Invocation | Behavior |
| --- | --- |
| `slash_ac.exe` | If `config/components/slash.ini` exists, or that check fails, open the settings menu. If the file is missing, start `slash_ac.exe --config --init`, wait, and open settings only when that process exits 0. |
| `slash_ac.exe --settings` | Open the settings menu. This does not run initialization by itself. |
| `slash_ac.exe --config` | The configuration tool. `--init`, `--initialize`, `--seed`, both flags, and no further arguments keep their previous meanings. This command exits. It does not open settings. |
| `slash_ac.exe --run` | The previous runtime. A bare `--run` runs `report_and_empty_recycle_bin`. One further argument is a command name, or `--export-keymap-commands`. A missing INI stays verbose and is not created. |

A no-argument launch does not empty the Recycle Bin. Main's descriptor command remains `report_and_empty_recycle_bin`, and that launch is `slash_ac.exe --run report_and_empty_recycle_bin`. Log identities stay `slash`, `slash_settings`, and `slash_config`.

## Wake

| Invocation | Behavior |
| --- | --- |
| `wake_ac.exe` | If `config/components/wake.ini` exists, or that check fails, open the settings menu. If the file is missing, start `wake_ac.exe --config --init`, wait, and open settings only when that process exits 0. |
| `wake_ac.exe --settings` | Open the settings menu. This does not run initialization by itself. |
| `wake_ac.exe --config` | The configuration tool. `--init`, `--initialize`, `--seed`, both flags, and no further arguments keep their previous meanings. When history files are missing, this still starts a separate `wake_ac.exe --run --snapshot` and waits. |
| `wake_ac.exe --run` | The previous runtime. `--snapshot` captures once and exits. `--export-keymap-commands` returns 0 with no output. A bare `--run` captures, then stays on `ac_wake_pipe`. Any other tail returns 1. A missing INI is reported and is not created. |

Log identities stay `wake`, `wake_settings`, and `wake_config`.

## Server

| Invocation | Behavior |
| --- | --- |
| `server_ac.exe` | If `config/components/server.ini` exists, or that check fails, open the settings menu. If the file is missing, start `server_ac.exe --config --init`, wait, and open settings only when that process exits 0. |
| `server_ac.exe --settings` | Open the settings menu. This does not run initialization by itself. The port editor is `server_ac.exe --editor`. Starter-site creation is `server_ac.exe --builder --seed`. The baseline probe still checks `port.id` and `document_root.id`. |
| `server_ac.exe --editor` | The port and document-root editor. `--init` and `--seed` keep their previous meanings. A separate process still opens the document-root folder. |
| `server_ac.exe --builder` | Requires `--seed`. Any other invocation prints usage and exits 1. |
| `server_ac.exe --config` | The configuration tool. `--init`, `--initialize`, `--seed`, both flags, and no further arguments keep their previous meanings. It starts `server_ac.exe --editor --seed` or `--editor --init`, then `server_ac.exe --builder --seed`, as separate processes. `--init` still asks before the builder. This command exits. It does not open settings. |
| `server_ac.exe --run` | The hosted HTTP listener. `--export-keymap-commands` returns 0 with no output. A missing INI uses `<installation>/data/server` and is not created. Missing or invalid `port.id` or `document_root.id`, or a missing document root, still exits 1 and does not launch the editor or builder. |

Log identities stay `server`, `server_settings`, `server_config`, `server_editor`, and `server_builder`.

## Writer

| Invocation | Behavior |
| --- | --- |
| `writer_ac.exe` | If `config/components/writer.ini` exists, or that check fails, open the settings menu. If the file is missing, start `writer_ac.exe --config --init`, wait, and open settings only when that process exits 0. |
| `writer_ac.exe --settings` | Open the settings menu. This does not run initialization by itself. Delegated actions are `writer_ac.exe --editor --add-task`, `--editor --add-session-prompt`, and `--editor --daily-note`. The baseline probe still checks `session_prompts.list` and `task_list.txt`. |
| `writer_ac.exe --editor` | The editor. `--init`, `--seed`, `--add-task`, `--add-session-prompt`, and `--daily-note` keep their previous meanings. A no-argument editor that needs initialization tells the user to run `writer_ac.exe --config --init`. |
| `writer_ac.exe --builder` | Rebuilds the text catalog. Any further argument prints usage and exits 1. |
| `writer_ac.exe --config` | The configuration tool. It starts `writer_ac.exe --editor --seed` or `--editor --init` as a separate process. This command exits. It does not open settings. |
| `writer_ac.exe --run` | The hosted session. `--export-keymap-commands` returns 0 with no output. A missing INI uses `<installation>/writer` and `<writer>/notes` and is not created. The daily-note action starts a separate `writer_ac.exe --editor --daily-note` and waits. Notepad still goes through `auto_core_shell_launcher.exe`. |

Log identities stay `writer`, `writer_settings`, `writer_config`, `writer_editor`, and `writer_builder`.

## Taskbar

| Invocation | Behavior |
| --- | --- |
| `auto_core_taskbar.exe` | If `config/taskbar_policy.ini` exists, or that check fails, open the settings menu. If the file is missing, start `auto_core_taskbar.exe --config --init`, wait, and open settings only when that process exits 0. |
| `auto_core_taskbar.exe --settings` | Open the settings menu. This does not run initialization by itself. Discovery is `auto_core_taskbar.exe --builder`. |
| `auto_core_taskbar.exe --builder` | Discovers applications. `--seed` stays noninteractive. After maps are written it starts a separate `auto_core_taskbar.exe --run --refresh-cache` and waits. |
| `auto_core_taskbar.exe --config` | The configuration tool. It starts `auto_core_taskbar.exe --builder` or `--builder --seed` as a separate process. This command exits. It does not open settings. It owns `config/taskbar_policy.ini`. |
| `auto_core_taskbar.exe --run` | The authority when the tail is empty. `--refresh-cache` uses the cache mutex only and does not take the authority mutex or start the control pipe. `--inspect-snapshot <application>` keeps exit codes 0, 1, and 2. `--export-keymap-commands` returns 0 with no output. A missing policy file stays `mode = live` and uses `<installation>/data/taskbar`. |

Main starts `app\auto_core_taskbar.exe --run` itself. It is not a `components.list` entry. `launch_taskbar_config` starts `auto_core_taskbar.exe --config` in a new console. Log identities stay `taskbar`, `taskbar_settings`, `taskbar_config`, and `taskbar_builder`.

`auto_core_keymap.exe --build` and startup resolve a shared `activate_<program>` name with the same rules. Ordinary duplicate names are resolved the same way from `keymap/shared_commands.map`, with a blank initial value and a `<provider>_<command>` alias. Alias collisions are checked against the command names and the aliases accepted by either map. `--build` is the only writer of `keymap/activate_commands.map` and `keymap/shared_commands.map`. It rebuilds that file from the successful exports and `data/taskbar/applications/*.map` when it writes `keymap/keymap_commands.txt`. A failed export leaves both maps unchanged. After every configured provider exports successfully, `shared_commands.map` drops a saved command that none of them registers and keeps the empty `[shared_commands]` section. A disabled component is still exported. `activate_commands.map` keeps a saved command that the discovery no longer offers. Startup reads the map and registers only actions it can invoke. One provider with no saved line keeps `activate_<program>` and gets no alias. A saved provider that is available runs the shared name, and each available provider is also published under its explicit alias. A saved provider that is unavailable is preserved, reported, and left unresolved. `refresh_taskbar_positions` is a hello command from the running authority and is not part of the `--build` disk catalog. WinKey dispatch stays in `auto_core.dll` after `connect()`.

## iTunes

| Invocation | Behavior |
| --- | --- |
| `itunes_ac.exe` | If `config/components/itunes.ini` exists, or that check fails, open the settings menu. If the file is missing, start `itunes_ac.exe --config --init`, wait, and open settings only when that process exits 0. |
| `itunes_ac.exe --settings` | Open the settings menu. This does not run initialization by itself. Song formatting is `itunes_ac.exe --formatter`. |
| `itunes_ac.exe --formatter` | Edits `song.format` and `library.format`. `--seed` and `--init` require a readable INI and do not launch configuration. A no-argument formatter with a missing INI starts `itunes_ac.exe --config --init`, waits, and then edits. |
| `itunes_ac.exe --config` | The configuration tool. `--seed` is checked before `--init`. It starts `itunes_ac.exe --run --seed`, then `itunes_ac.exe --formatter --seed` or `--formatter --init`, as separate processes. This command exits. It does not open settings. |
| `itunes_ac.exe --run` | The hosted session. `--seed` creates a missing `history.db` and requires a readable INI. `--database` is the history menu. `--export-keymap-commands` lists commands. COM initialization stays on the hosted path. A missing INI is reported and is not created. The hosted open still creates `history.db`. |

Log identities stay `itunes`, `itunes_settings`, `itunes_config`, and `itunes_formatter`.

## Spotify

| Invocation | Behavior |
| --- | --- |
| `spotify_ac.exe` | If `config/components/spotify.ini` exists, or that check fails, open the settings menu. If the file is missing, start `spotify_ac.exe --config --init`, wait, and open settings only when that process exits 0. |
| `spotify_ac.exe --settings` | Open the settings menu. This does not run initialization by itself. Delegated actions are `--editor`, `--oauth` in a new console, and `--formatter`. |
| `spotify_ac.exe --editor` | Edits `client.id` and `devices.list`. `--seed` and `--client-id` keep their previous meanings. |
| `spotify_ac.exe --formatter` | Edits `song.format`. `--seed` and `--init` require a readable INI and do not launch configuration. A no-argument formatter with a missing INI starts `spotify_ac.exe --config --init` and waits. |
| `spotify_ac.exe --oauth` | Authorization in its own console. Extra arguments are ignored. The token mutex stays `Local\AutoCoreSpotifyTokens`. |
| `spotify_ac.exe --config` | The configuration tool. When both `--seed` and `--init` are present, seed runs first and authorization still continues. Children are `spotify_ac.exe --run --seed`, `--formatter --seed` or `--formatter --init`, `--editor --seed` or `--editor --client-id`, and `--oauth` with `CREATE_NEW_CONSOLE`. This command exits. It does not open settings. |
| `spotify_ac.exe --run` | The hosted session, including the devices pipe thread. `--seed` creates a missing `history.db` and requires a readable INI. `--database` is the history menu. `--export-keymap-commands` lists commands. A missing INI is reported and is not created. A missing `song.format` keeps the compiled template. Runtime auto-launch of OAuth stays a separate console process and waits so the runtime can reload tokens. |

`--run --seed` only creates `history.db`. `--config --seed` is the configuration orchestrator. `--formatter --seed` and `--editor --seed` keep their narrower writes. Log identities stay `spotify`, `spotify_settings`, `spotify_config`, `spotify_editor`, `spotify_formatter`, and `spotify_oauth`.

## Journal

| Invocation | Behavior |
| --- | --- |
| `journal_ac.exe` | If `config/components/journal.ini` exists, or that check fails, open the settings menu. If the file is missing, start `journal_ac.exe --config --init`, wait, and open settings only when that process exits 0. |
| `journal_ac.exe --settings` | Open the settings menu. This does not run initialization by itself. Delegated actions are `--builder`, `--series`, `--cloud`, and `--clock`. |
| `journal_ac.exe --builder` | Alias lists. `--seed` and `--init` keep their previous meanings. |
| `journal_ac.exe --clock` | `extended_hour.clock`. A missing INI still starts `journal_ac.exe --config` with the same `--seed` and `--init` tokens and waits. `--clock --seed` does not launch configuration again after that INI exists. |
| `journal_ac.exe --cloud` | Firebase configuration, or `--serve` with optional `--owner-handle`. `--cloud --serve` stays a separate process. |
| `journal_ac.exe --series` | The series menu. It opens its own `series.db`. A hosted `--run` and this menu are two processes. |
| `journal_ac.exe --config` | The configuration tool. `--seed` is checked before `--init`. Children, in order, are `--builder`, `--cloud`, `--clock`, and `--series`, each with that mode. A no-argument configuration still runs `--builder --seed` before the menu. This command exits. It does not open settings. |
| `journal_ac.exe --run` | The hosted session. `--export-keymap-commands` lists commands. A missing INI is reported and is not created, and cloud is not started. A missing `series.db` is created empty, without the seed series row. Alias lists and `extended_hour.clock` stay optional. |

Log identities stay `journal`, `journal_settings`, `journal_config`, `journal_builder`, `journal_clock`, `journal_cloud`, and `journal_series`.

## What Auto Core launches

Every discovered component receives these arguments. A third-party `<name>_ac.exe` must implement the same contract.

- Main's hosted start passes `--run`.
- `auto_core_settings.exe` discovers `*_ac.exe` and launches each one as `--settings`. A `*_settings.exe` is not a settings row. Enablement still comes from `components.list`.
- `auto_core_components.exe --initialize` launches `*_ac.exe --config` plus its existing `--init` or `--init --seed` tail.
- `auto_core_keymap.exe --build` passes `--run --export-keymap-commands`.
- A launch descriptor is `{name}_ac.exe --run` plus the descriptor arguments. Slash's registered command name stays `report_and_empty_recycle_bin`. Dash still receives `--target` and `--parent-pid`.

`components.list` is still built from installed `*_ac.exe` names that already have `config/components/<name>.ini`. That discovery is unchanged.

## Third-party components

Ship one `example_ac.exe` and place it in `<installation root>\components`. `scripts/update-dist.ps1` does not remove a third-party executable that it did not publish. A no-argument launch opens settings when `config/components/example.ini` exists or that check fails, and otherwise runs `example_ac.exe --config --init`, waits, and opens settings only after exit code 0. `--settings` opens settings without that initialization. `--config` owns the INI and exits. `--run` is the hosted session. The pipe name remains `ac_example_pipe`. Do not ship `example_config.exe` or `example_settings.exe`.

The executable must delay-load `auto_core.dll` and any other shared DLL it imports from `app\`. Compile [`app/shared/component_dll_search.cxx`](../app/shared/component_dll_search.cxx), link `delayimp.lib`, and pass `/DELAYLOAD` for each of those DLLs. The startup file configures sibling `app\` with `SetDefaultDllDirectories` and `AddDllDirectory` before normal C++ constructors run. An ordinary import of `auto_core.dll` is resolved before that code and cannot see `app\`. Do not copy the shared DLLs beside the component executable, and do not change `PATH`. `msbuild/ComponentRuntime.props` is the in-tree form of those linker settings. A DLL that the executable imports as data cannot be delay-loaded; keep those references as function imports.
