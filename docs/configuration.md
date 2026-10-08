# Runtime configuration

Runtime configuration files belong in `dist/config`. **Only `_config.exe` programs create or rewrite `.ini` files.** Runtime programs (`auto_core.exe` and `<name>_ac.exe`) parse their INIs and never write them. If a required INI is missing, unreadable, missing a section or key, or has an invalid value, the process uses the documented default in memory and reports the condition with `log_print`. Typed defaults for children live in `app/components/<name>/shared/defaults.ixx`. Main helper defaults live in [`app/main/shared/defaults.ixx`](../app/main/shared/defaults.ixx). `<name>_config.exe --seed` writes a missing live INI from that text.

If a component INI is missing or malformed, `<name>_ac.exe` uses `shared/defaults.ixx` in memory and `Component::report_ini_unavailable` (`log_print`): run `<name>_config.exe`; defaults are in use; the file will not be created. Invalid, empty, or unknown **keys** in a readable file keep that key's default and do not rewrite the file. Defaults apply per key: an invalid value keeps that key's default and does not discard another valid key.

Relative `directory` values resolve against the **installation root** (`dist/` after a normal build), never against `dist/bin/` or the process current working directory. `auto_core.exe` lives at `dist/bin/auto_core.exe`. `bin_directory` is that folder; `installation_root` is its parent. A relative Server `document_root.id` resolves under the Server data directory. An absolute `document_root.id` is used as-is.

`auto_core.exe` checks that `config/auto_core.ini` exists. The presence of that file means Auto Core is initialized. There is no `initialized` key. If it is missing, the installation is new. Main launches `auto_core_init.exe` and waits. That orchestrator asks only about Auto Core configuration: `1. Use defaults` runs `auto_core_config.exe --seed`, and `2. Configure` runs `auto_core_config.exe --init`. It then launches `logger_init.exe` and `components_init.exe`, each with its own menu, then seeds `keymap_config.exe`, `components_config.exe`, `shutdown_config.exe`, and `crash_recovery_config.exe`, then runs `components_editor.exe --seed` and `keymap_editor.exe`. `logger_init.exe` and `components_init.exe` do not write INI files. A failed or cancelled Logger or Components stage stops initialization. Main resolves the shared logging policy after initialization returns. `auto_core_config.exe` is the only writer of `auto_core.ini`. If a required step fails or the user cancels that step, and this run created `auto_core.ini`, `auto_core_init.exe` removes the file before it exits and Main exits `1`. Leaving the init menu before the sequence starts does not delete an existing file. If `auto_core.ini` already exists and `logging.ini` is missing, Main uses built-in logging defaults, warns, and does not create the file. Startup does not check `components.list` or `keymap.map` to decide first-run. A missing list or map after startup uses the existing in-memory fallback and is not created by `auto_core.exe`. If `auto_core.ini` already exists, `auto_core_config.exe` opens the Main configuration menu and does not repeat first-run orchestration.

Each `_config.exe` accepts `--seed`, `--init`, and both together in either order. `--initialize` is a compatibility alias of `--init`. The parser in `components_editor_request` sets both flags and does not invent a third mode. **Observed:** when both are set, `launch.seed` is handled first. Every INI owner except `spotify_config.exe` then returns, so `--init` does not prompt and does not rewrite an existing file. `--seed` still creates the file only when it is missing. `spotify_config.exe` seeds the same way, then continues into client-id and OAuth prompts because `launch.init` stays true. `components_init.exe` menu choice 1 launches every discovered `<name>_config.exe --init --seed`. `journal_clock.exe` forwards `--seed --init` to `journal_config.exe` when `journal.ini` is missing; `journal_config.exe` still takes the seed branch and ignores `--init`. **C8** combined flags stay **Observed** until the CLI decision. They are not a third mode.

No-argument launch remains normal configuration of the owned file. The exhaustive key list is [the INI key ledger](#ini-key-ledger). It is Phase 1A discovery. Finalized INI rules (C4 location and ownership, C5 `on`/`off`, C10 missing and invalid keys) stay in force. Proposed names are not renames.

| File | Purpose |
| --- | --- |
| `auto_core.ini` | Presence of the file means Auto Core is initialized. There is no `initialized` key. `[auto_core] warn_without_winkey_mapping` (default `on`) and `logging` (default `on`). When the warning is on, `taskbar_ac.exe` warns if Auto Core is not in taskbar positions 1–10. Warning values are lowercase `on` or `off` only; any other value keeps the default. Set `off` to silence the warning. `logging` is the Auto Core Main family switch for `auto_core.exe`. A missing or invalid `logging` value uses `component_logging_default`. Written only by `auto_core_config.exe`, before Components initialization. A failed required init step removes the file when that run created it. Missing or malformed warning: in-memory default `on`, `log_print`, no file create. |
| `components.ini` | Component-system `[settings]` only (`new_components` = `prompt`/`on`/`off`, `sort_components` = `on`/`off`, `remove_missing_components` = `on`/`off`; defaults `on` / `on` / `off`). Exclusive writer: `components_config.exe`. It does not seed `<name>.ini`. `components_init.exe` discovers `*_ac.exe` names and launches `<name>_config.exe`. After the user changes `components.ini`, the config helper may offer `components_editor.exe`. Settings are read case-insensitively and written lowercase. Missing INI during sync: compiled defaults in memory, no file create. |
| `../components.list` | Installation-root component registry. Exclusive writer: `components_editor.exe`. `[components]` lines are `name`, `name on`, or `name off` (blank defaults to on; any other token is malformed and disables that name). A readable list is authoritative: a missing name is disabled. Names are case-sensitive and lowercase-only (`^[a-z][a-z0-9_]*$`); invalid names are not normalized. Duplicate names use last-wins. `dash` and `slash` use the same rules. Main does not start them as v1 session children. The portable seed lists the ten known names with blank (on) values. A missing list is reconstructible: no-arg `components_editor.exe` rebuilds it from installed `*_ac.exe` names that already have `config/<name>.ini`, applying `new_components`, `remove_missing_components`, and `sort_components` from `components.ini` or compiled defaults. `--component <name>` requires `config/<name>.ini` to exist, applies `new_components` to that name only, and does not prune. `--component <name> --on` or `--off` rewrites that entry to explicit `on` or `off` and does not rebuild the list. `<name>_star.exe` reads the current state and launches that command; it does not edit the file. There is no legacy catalog migration. `<name>_config.exe` must not edit this file. `<name>_config.exe` does not launch `components_editor.exe`. `--seed` and `--init` do not update the list. `--seed` on `components_editor.exe` creates a missing list and leaves an existing list unchanged. A missing or unreadable list at runtime: `log_print`, enable every valid `*_ac.exe` in `bin_directory`, no file create. Main does not exit because the list is missing. |
| `itunes.ini` | `[itunes]` `directory` (portable default `components/itunes`, which is `dist/components/itunes` when Auto Core runs from `dist`) and `auto_start` (default `on`). A relative `directory` resolves against the installation root. An absolute path is used as-is. A missing or empty `directory` uses `<installation_root>/components/itunes`. `itunes_config.exe` does not create that directory. `auto_start` is lowercase `on` or `off` only; any other value keeps the default. Written only by `itunes_config.exe`. `--seed` and `--init --seed` write the compiled text when the file is missing, leave an existing file unchanged, then run `itunes_db.exe --seed` and `itunes_formatter.exe --seed`. `--init` prompts only when the file is missing, skips an existing file, then runs `itunes_db.exe --seed` and `itunes_formatter.exe --init`. Initialization does not rewrite an existing store. No-argument launch prompts for `directory`, `auto_start`, and `logging` and rewrites an existing file. It does not launch those children. A leftover `tab_end` key is ignored. `song.format` and `library.format` under the iTunes directory are written only by `itunes_formatter.exe`. `library.format` stores `format` (compiled default `[column]`) and `column_count` (compiled default `4`). `column_count` is how many left-to-right columns are retained from tab-delimited iTunes Library rows copied through the Clipboard. `itunes_formatter.exe --seed` creates a missing format file and leaves an existing file unchanged. `itunes_formatter.exe --init` prompts only for a missing format file. Both require a readable `itunes.ini`. `itunes_db.exe --seed` creates a missing empty `history.db` and does not open an existing database. `format` is either one surrounding symbol pair, such as `[column]`, or one separator between columns, such as `column - column`. `itunes_ac.exe` reads both files and does not rewrite them. A missing `library.format`, or a missing assignment, uses that field's compiled default in memory. An unreadable file is logged and uses both defaults. An invalid `format` or `column_count` is logged and falls back only that field. `itunes_db.exe` owns `history.db` in that directory. `itunes_ac.exe` does not open it. Missing or malformed INI: in-memory defaults, no file create. See [iTunes](itunes.md). |
| `journal.ini` | `[journal]` `directory` (portable default `components\journal`, which is `dist/components/journal` when Auto Core runs from `dist`), `auto_select_new_series` (`on`), `remote_sync` (`off`), and `logging` (`on`). `auto_select_new_series` accepts `on` or `off`. `on` makes a series created in `journal_series.exe` the active series, using the name stored by `journal_db.exe`. `off` keeps the current `active = Name`. The snapshot still refreshes. A failed add does not change it. A missing or unrecognized value stays `on`. Relative paths resolve against the installation root. A missing or empty `directory` uses `<installation_root>/components/journal`. Written only by `journal_config.exe`. `--seed` and `--init --seed` write the compiled text when `journal.ini` is missing, leave an existing file unchanged, then run `journal_builder.exe --seed`, `journal_cloud.exe --seed`, `journal_clock.exe --seed`, `journal_db.exe --seed`, and `journal_series.exe --seed`. `--init` prompts only when `journal.ini` is missing (`Journal directory`, `Auto-select new series`, `Remote sync`, `Enable logging`), skips an existing file, then runs those executables with `--init`. Each owner creates only its own missing file. `journal_db.exe --seed` adds `Auto Core` with padding `2` only when it creates a missing `series.db`. An existing database, including an empty one created by `journal_db.exe --serve`, is left unchanged. Add a series to that database with `journal_series.exe`. `journal_series.exe` writes a missing `series.map` from that database and does not run if database initialization failed. No-argument launch rewrites the INI from the menu and seeds missing alias files only. `remote_sync` accepts `on` or `off`. `on` starts `journal_cloud.exe --serve`; `off` and every other value leave it off. The active series is `active = Name` in `series.map`, owned by `journal_series.exe`. `journal_db.exe` owns `series.db`. `journal_cloud.exe` owns `firebase.id`. `journal_ac.exe` does not open SQLite or write those files. `extended_hour.clock` lives in that journal data directory and is written only by `journal_clock.exe`. |
| `crash_recovery.ini` | `[crash_recovery] default_response` (`yes` or `no`, default `no`) controls Main's existing previous-crash prompt. `crash_diagnostics` (`on` or `off`, default `on`) independently controls shared crash reports for every Auto Core executable. Missing, unreadable, missing-key, or invalid diagnostics configuration uses `on` in memory. Written only by `crash_recovery_config.exe`; runtime never rewrites it. |
| `shutdown.ini` | `[shutdown] delayed_shutdown_prompt` (`popup` or `console`, default `popup`) and one shared `shutdown_timeout_ms` (default `2000`) used for generic children. Invalid values keep their defaults. Console mode keeps Main alive at its recovery prompt until input arrives. Written only by `shutdown_config.exe`. Missing or malformed: in-memory defaults, `log_print`, no file create. |
| `logging.ini` | Shared logging policy for every executable. `[logging]` keys are written in this order: `disable_all` (default `off`), `directory` (portable default `logs`, which is `dist/logs` when Auto Core runs from `dist`), `write_logs_to_files` (default `on`), `write_logs_to_console` (default `off`), `log_print_mode` (`log` or `print`, default `print`), and `component_logging_default` (default `on`). Booleans are exactly `on` or `off`. Any other text is invalid. `disable_all = on` stops logging-controlled file and console output and does not emit the disabled-family notice. Menus, prompts, `print`, and `report_error` stay on the console. `log_print_mode` classifies `log_print` before the sinks. A relative `directory` resolves against the installation root. A missing or empty value uses `<installation_root>/logs`. Written only by `logging_config.exe` ([`app/main/config/logging`](../app/main/config/logging)). `--seed` and `--init --seed` write the compiled text when the file is missing and leave an existing file unchanged. `--init` prompts those six keys in that order when the file is missing. `--disable` writes the compiled text with `disable_all = on` when the file is missing and leaves an existing file unchanged. No-argument launch remains the settings menu and prompts for a missing file. On a new installation, `logger_init.exe` chooses `--seed`, `--init`, or `--disable` for this owner and does not write the file itself. On an established installation, a missing file uses in-memory defaults, Main warns, and the file is not created. |
| `logger.ini` | Merger timing and the Logger family `logging` key. `[logger]` `merge_interval_seconds` (default `60`; `0` disables periodic merging while an enabled Logger stays hosted; invalid values use `60`) and `merge_logs_on_shutdown` (default `on`). Booleans are exactly `on` or `off`. Any other text is invalid and keeps the default `on`. The shutdown merge is independent of the periodic interval. `logger_config.exe` ([`app/components/logger/config`](../app/components/logger/config)) is the only writer. `logger_ac.exe` merges `.main.log` files into `YYYY-MM-DD_main.log` under the shared `logging.ini` directory, writes `merge.state` there, and does not write this file. The hosted process exits without a final merge. After it has exited, shutdown `on` starts a detached `logger_ac.exe --once --shutdown`. `--shutdown` only labels that launch. A missing file uses in-memory defaults and is not created; Main prints that `logger_config.exe` should be run. A malformed file uses those defaults and does not get that alert. |
| `keymap.ini` | `[keymap] silence_nonset_warning` (default `off`). Must be exactly `on` to skip the load-time unset-key messages. Written only by `keymap_config.exe`. Missing or malformed: in-memory default `off`, `log_print`, no file create. |
| `server.ini` | `[server]` `directory` (portable default `components\server`, which is `dist/components/server` when Auto Core runs from `dist`) and `logging` (`on`). A relative `directory` resolves against the installation root. A missing or empty `directory` uses `<installation_root>\components\server`. Written only by `server_config.exe`. `--seed` and `--init --seed` write the compiled text when the file is missing, leave an existing file unchanged, then run `server_editor.exe --seed` and `server_builder.exe --seed`. `--init` prompts `Server directory [components\server]:` and `Enable logging [on]:` only when the file is missing, then runs `server_editor.exe --init`. It then asks `Create default site files in the document root? [Y/n]:`. Enter runs `server_builder.exe --seed`; `n` succeeds without creating starter files. Initialization never rewrites an existing store. No-argument launch is the settings menu and does not launch the editor or the builder. Changing `directory` rewrites the INI only and does not seed the new directory. Legacy `port` and `document_root` keys are ignored and drop out on the next rewrite. `port.id` (seed `8585`) and `document_root.id` (seed `site`) live in the Server data directory and are written only by `server_editor.exe`. A relative `document_root.id` resolves under that directory; an absolute path is used as-is. `server_ac.exe` requires both scalars and does not invent them. A missing document-root directory fails startup and does not launch the builder. `server_builder.exe --seed` writes missing `index.html` and `styles.css` independently, only under a relative document root inside the data directory, and leaves an existing file unchanged. The listener is loopback-only (`127.0.0.1`). See [Server](server.md). |
| `spotify.ini` | `[spotify]` `directory` (portable default `components\spotify`, which is `dist/components/spotify` when Auto Core runs from `dist`), `auto_launch_oauth` (default `off`), and `logging` (default `on`). Relative paths resolve against the installation root. A missing or empty `directory` uses `<installation_root>/components/spotify`. `on` enables automatic OAuth and `off` disables it. Any other value stays `off`. The setting is read when `spotify_ac.exe` starts and applies only when interactive reauthorization is already required. It does not launch OAuth at startup. `off` reports that authorization is required and does not start `spotify_oauth.exe` or a browser. An already authorized session still refreshes through `tokens.map`. `--seed` is noninteractive: it writes the compiled INI when that file is missing, then runs `spotify_db.exe --seed`, `spotify_formatter.exe --seed`, and `spotify_editor.exe --seed`. Those create a missing `history.db`, a missing `song.format` using `[{name}] [{duration}] [{artist}] [{album}]`, and a missing blank `client.id`. Existing files are left unchanged. `--seed` never creates `tokens.map` and never launches OAuth, the editor UI, or a browser. `--init` prompts for a missing INI. Enter on `auto_launch_oauth` stores `off`. It asks before creating a missing `history.db` and runs `spotify_formatter.exe --init` when `song.format` is missing. `--init --seed` seeds those defaults first, then continues with client-ID configuration and OAuth. A missing or blank `client.id` explains that the Spotify component requires user authorization and offers `spotify_editor.exe --client-id`. Leaving the ID blank does not launch OAuth. A usable `client.id` with no `tokens.map` starts `spotify_oauth.exe` in its own console and waits. A usable `client.id` together with an existing `tokens.map` does not prompt for the client ID and does not launch OAuth. `spotify_config.exe` returns 0 when authorization is left incomplete so later component initialization can continue. That 0 means the config executable finished. The log line is either `Spotify initialization complete.` or `Spotify initialization incomplete: User Authorization has not been completed.` A filesystem failure or a missing helper still returns 1. No-argument config edits the INI only. Written only by `spotify_config.exe`. `song.format` is written only by `spotify_formatter.exe`. `spotify_ac.exe` reads `song.format` and does not rewrite it. A missing or invalid file uses the compiled default template in memory. See [Spotify](spotify.md). |
| `taskbar.ini` | `[taskbar]` `directory` (portable default `taskbar`, which is `dist/taskbar` when Auto Core runs from `dist`), discovery `mode` (`live` or `cache`; default `live`), and `logging` (`on`). Relative `directory` paths resolve against the installation root. A missing or empty `directory` uses `<installation_root>/taskbar`. Written only by `taskbar_config.exe`. `--seed` and `--init --seed` write that compiled text when the file is missing, leave an existing file unchanged, and run `taskbar_builder.exe --seed`. `--init` prompts `Taskbar directory [taskbar]:`, `Taskbar mode [live]:`, and `Enable logging [on]:` only when the file is missing, then asks `Run Taskbar builder now? [Y/n]:`. Enter runs `taskbar_builder.exe`; `n` succeeds without it. The builder owns `applications/*.map` and creates only missing files. `taskbar_ac.exe` owns `winkey_map.cache` and replaces it from live discovery. `mode = live` does that during normal runtime. `mode = cache` uses a usable cache and discovers only when the cache is missing or empty. `taskbar_ac.exe --refresh-cache` forces live discovery and replacement regardless of mode. An unavailable `taskbar.ini` uses the compiled default `directory = taskbar`, the same fallback as normal `taskbar_ac.exe`. A failed refresh is reported by `taskbar_builder.exe` as `Unable to refresh winkey_map.cache.` Restart `taskbar_ac.exe` before a new `directory` or `mode` applies. The interactive UI accepts only `live` or `cache`. See [Taskbar](taskbar.md). |
| `writer.ini` | `[writer]` `directory` (portable default `writer`, which is `dist/writer` when Auto Core runs from `dist`), `notes_subdirectory` (portable default `notes`, which is `dist/writer/notes`), and `logging` (`on`). `directory` resolves against the installation root. `notes_subdirectory` must be relative and resolves against the Writer directory. A missing, empty, or non-relative `notes_subdirectory` uses `<writer directory>/notes`. Written only by `writer_config.exe`. `--seed` and `--init --seed` write that compiled text when the file is missing, leave an existing file unchanged, then run `writer_editor.exe --seed`. `--init` prompts `Writer directory [writer]:`, `Notes subdirectory [notes]:`, and `Enable logging [on]:` only when the file is missing, then runs `writer_editor.exe --init`. The editor creates missing `session_prompts.list`, `task_list.txt`, and the notes directory. `--init` asks before creating the notes directory; `n` succeeds and leaves it absent. An existing store is not replaced. A custom `directory` is followed. No-argument launch edits `directory` and `notes_subdirectory` and does not launch the editor. See [Writer](writer.md). |
| `dash.ini` | `[dash] logging = off`. Written only by `dash_config.exe`. The vault `%LOCALAPPDATA%\Auto Core\dash.vault` is written only by `dash_editor.exe`. `dash_ac.exe` reads and uses that vault and does not mutate it. Missing or malformed INI: in-memory defaults, no file create. See [Dash](dash.md). |
| `wake.ini` | `[wake] directory` (portable default `components\wake`, which is `dist/components/wake` when Auto Core runs from `dist`) and `logging` (`on`). Relative paths resolve against the installation root. A missing or empty `directory` uses `<installation_root>/components/wake`. Written only by `wake_config.exe`. `logging` is the ordinary component-log switch and does not gate Wake history. `--seed` and `--init --seed` write the compiled text when the file is missing and leave an existing file unchanged. `--init` prompts for the directory and logging only when the file is missing. All three then run `wake_ac.exe --snapshot` only when `current.event`, `previous.event`, or `wake_events.log` is missing. An existing history file, including an empty file, is not opened or replaced by config. No-argument launch edits the INI and does not snapshot. See [Wake](wake.md). |
| `slash.ini` | `[slash] mode` (`verbose`, `concise`, or `silent`; default `verbose`) and `logging` (default `on`). `verbose` is the categorized report. `concise` reports `Recycle Bin emptied: N items`. `silent` prints `Recycle bin emptied` on the console and does not insert text. An empty bin prints `Recycle bin is empty`. Any other `mode` keeps `verbose`. `logging` is the family switch. Written only by `slash_config.exe`. `--seed` writes the compiled text when the file is missing. `--init` prompts when the file is missing. Both flags together take the seed branch and do not prompt. No-argument launch edits the file. Missing or invalid: in-memory defaults, `log_print`, no file create or rewrite. See [Slash](slash.md). |

The `dist/crash/` directory is created only when an enabled Auto Core process records a crash. Each event uses a collision-safe UTC/executable/PID folder containing `crash.txt` and, when DbgHelp succeeds, `crash.dmp`. Existing reports are never overwritten or automatically deleted. No executable, DLL, or PDB is copied into a report. Main's recovery marker remains `crash/.crash`: if it exists at the next start, Auto Core asks whether to continue. Yes removes the marker and continues; No exits and leaves it. Recovery stays disabled until that prompt completes. Crash diagnostics are independent of normal logging, including `[logging] disable_all = on`. Dumps can contain diagnostic process information and are never uploaded automatically. For support, open the Auto Core installation folder, open `crash`, and send only the requested crash-report folder. See [main.md](main.md).

Every production executable that links `auto_core.dll` initializes the shared
handler through the common Shell startup call. `auto_core_shell_launcher.exe` is the sole
production exception: it is a minimal breakaway trampoline intentionally built
without `auto_core.dll`, immediately delegates to `ShellExecuteEx`, and exits.
Test runners likewise do not install product crash handling automatically.

## Logging

Runtime logs default to `<installation root>/logs` (`ac::paths::log_directory()`). The portable INI sets `[logging] directory = logs`, which is `dist/logs` when Auto Core runs from `dist`. A relative path is resolved against the installation root; an absolute path is used as-is. A missing or empty `directory` keeps `<installation_root>/logs`. `directory()` is the shared `logging.ini` policy. `logger_ac.exe` uses that same directory for local logs and merged output. `logger.ini` controls only when merging occurs. Changing `directory` does not move existing files.

Every executable has its own logging identity and its own directory under `logs/components/`. An `ac::Component` named `<name>` writes a comprehensive `{date}_{name}.log` and a main-log subset `{date}_{name}.main.log` under `components/<name>/`. `<name>.main.log` is an exact subset of `<name>.log`: the same timestamp, the same text, and the same open-line boundaries. The subset is not restamped. Session start/end/`***` markers go only to `{date}_{name}.log`.

`disable_all`, `write_logs_to_files`, and `write_logs_to_console` stay distinct. File writes happen only when `disable_all` is off, that executable's family logging is on, and `write_logs_to_files` is on. Ordinary `log` and `log_main` reach the console only when `write_logs_to_console` is on. `log_print` follows `log_print_mode`: `log` is logging-controlled, and `print` is user-facing, so it still reaches the console when the console sink is off. `print` and `report_error` are user-facing. They keep the console, and they keep file and main-log output when the file sink is on. `log` and `lognl` write only the comprehensive log. `log_main` and `lognl_main` also write the main-log subset. The `nl` names follow the same routes and differ only by leaving the record open. Family logging is `[<scope>] logging` in `config/<scope>.ini`. A normal missing or invalid key uses `component_logging_default`. Dash uses scope `dash` with fallback off. `auto_core.exe` uses identity `auto_core` and scope `auto_core`, so `[auto_core] logging` controls `logs/components/auto_core/`. A `Component` with no scope does not open a component INI. When family logging is off and `disable_all` is off, the first routing decision prints `Logging is disabled for <family>.` once. Constructing `ac::Component` does not open log files; the first file write that passes the file gate does. A utility does not write into another executable's directory: `spotify_config.exe` uses `ac::Component{"spotify_config"}` with scope `spotify` and logs under `components/spotify_config/`, and `spotify_oauth.exe` logs under `components/spotify_oauth/`. `spotify_star.exe` logs under `components/spotify_star/`, and `itunes_star.exe` logs under `components/itunes_star/`. Main helpers use their own identities (`auto_core_init`, `components_init`, `auto_core_config`, `components_config`, `keymap_config`, `shutdown_config`, `crash_recovery_config`, `keymap_editor`, `components_editor`), not subdirectories of `components/auto_core/`.

Wake history (`current.event`, `previous.event`, `wake_events.log`) lives under `[wake] directory` (default `components/wake` at the installation root). Those files are written even when family logging is off. Ordinary `{date}_wake.log` files stay under `logs/components/wake/`. Old root-level `<name>/` folders are not moved.

Component records use `[YYYY-MM-DD HH:MM:SS.mmm] Message`.

`logger_ac.exe` merges component `.main.log` files into `YYYY-MM-DD_main.log` under the shared log directory and records consumed offsets in `merge.state`. Both files are runtime output owned by `logger_ac.exe`. Source `.main.log` files stay owned by the shared logging path. When `components.list` enables `logger`, Main hosts `logger_ac.exe`. A positive interval merges once at startup and again on each interval. Interval `0` disables periodic merging and leaves that hosted process waiting for shutdown. On shutdown the hosted logger exits immediately without another merge. `merge_logs_on_shutdown` then starts a detached `logger_ac.exe --once --shutdown` after that process has exited. The shutdown merge is independent of the periodic interval. Local logs are still written when `logger` is off. Main does not host `dash` or `slash` as v1 children. `logger` is an ordinary v1 name.

Do not commit machine-specific log output.

## Runtime data

Treat the following as local runtime state rather than portable project source:

- `dist/bin/` — runtime executables and DLLs
- `dist/components.list` — component enablement list
- `dist/keymap.map` — user key-to-command map
- `dist/components/spotify/` — `tokens.map`, `client.id`, `devices.list`, `history.db`, and `song.format`. Owners are in [persistent-state.md](persistent-state.md). Override the folder with `[spotify] directory` in `config/spotify.ini`. `spotify_formatter.exe` writes `song.format`. `spotify_ac.exe` reads that file and does not rewrite it. A missing or invalid format uses the compiled template in memory. `spotify_ac.exe` does replace `tokens.map` on refresh.
- `dist/components/itunes/` — `song.format` and `library.format`, both written only by `itunes_formatter.exe`, and `history.db`, the listening-history database owned only by `itunes_db.exe`. `song.format` is the COM song-line template. `library.format` stores `format` (compiled default `[column]`) and `column_count` (compiled default `4`). `format` is either one surrounding symbol pair, such as `[column]`, or one separator between columns, such as `column - column`. Override the folder with `[itunes] directory` in `config/itunes.ini`. `itunes_ac.exe` reads both format files and does not rewrite them. A missing or invalid `song.format` uses the compiled default template in memory. A missing `library.format`, or a missing assignment, uses that field's compiled default. An unreadable `library.format` is logged and uses both defaults. An invalid `format` or `column_count` is logged and falls back only that field. `itunes_ac.exe` does not open `history.db`.
- `dist/components/journal/` — `series.db`, `series.map`, `firebase.id`, `extended_hour.clock`, and one `<factory_name>.list` alias file per compiled factory (`make_print_choice.list`, `print_and_insert_into_journal.list`, `roll_dice.list`). `journal_db.exe` is the only writer of `series.db`. `journal_series.exe` is the only writer of `series.map`. `journal_cloud.exe` is the only writer of `firebase.id`. `journal_builder.exe` writes the alias files. `journal_builder.exe --seed` creates a missing starter file and does not change an existing one. The starters are one demonstration alias each. `journal_clock.exe` is the only writer of `extended_hour.clock`. `--seed` writes `extended_hours = +0` when that file is missing. `journal_ac.exe` reads the clock file and `active = Name` and does not rewrite them. A missing or malformed clock file behaves as `extended_hours = 0`. The snapshot in `series.map` updates only from `journal_series.exe` and can lag behind allocation. If `config/journal.ini` is missing, `journal_clock.exe` runs `journal_config.exe` before resolving the directory. Override the folder with `[journal] directory` in `config/journal.ini`. `journal_config.exe --init` and `--seed` create a missing `firebase.id` through `journal_cloud.exe` and a missing `series.map` through `journal_series.exe`. An existing file is not replaced.
- `dist/components/wake/` — `current.event`, `previous.event`, and `wake_events.log` from `wake_ac.exe`. `wake_ac.exe --snapshot` captures once and exits. `wake_config.exe --init` and `--seed` run that command only when one of the three files is missing. An existing file is not replaced. Override the folder with `[wake] directory` in `config/wake.ini`.
- `dist/taskbar/` — generated per-program `.map` definitions in `applications/` (`taskbar_builder.exe`) and `winkey_map.cache` (Win+1 through Win+10 cache from `taskbar_ac.exe`). `.map` v1 uses section/key syntax and is Taskbar application data, not configuration. Override the folder with `[taskbar] directory` in `config/taskbar.ini`.
- `dist/components/server/` — default Server data directory (`port.id`, `document_root.id`, and the generated `site\` when `document_root.id` is `site`). Override the folder with `[server] directory` in `config/server.ini`. A directory change does not copy the old tree. `server_config.exe --init` and `--seed` create a missing `server.ini`, then delegate missing `port.id` and `document_root.id` to `server_editor.exe` and missing starter files to `server_builder.exe`. An existing file is not replaced. The builder follows the stored `document_root.id` (`public` is `<server directory>\public`, not `site`). `server_ac.exe` does not create these files. See [Server](server.md).
- `dist/writer/` — `session_prompts.list`, `task_list.txt`, and `notes/` (`YYYY-MM-DD.txt`). Override the folder with `[writer] directory` and the notes folder with `[writer] notes_subdirectory` in `config/writer.ini`. A directory change does not copy the old tree. `writer_config.exe --init` and `--seed` create a missing `writer.ini`, then delegate missing prompt and task files to `writer_editor.exe`. An existing file is not replaced. `writer_editor.exe` creates those two files and today's note. `writer_ac.exe` launches `writer_editor.exe --daily-note` and does not write the note. See [persistent-state.md](persistent-state.md).
- `dist/logs/` — log output
- `dist/crash/` — Main recovery marker and per-process `crash.txt` / `crash.dmp` event folders
- `dist/errors/` — `errors.log` from best-effort error reporting

`dist/` is gitignored. Auto Core does not load `*.local.ini`.

`keymap_editor.exe` writes a seed `keymap.map` when that file is missing (`numpad_0` / `numpad_1` filled, every other `key_codes` name blank). An existing `keymap.map` is never overwritten. `components_editor.exe --seed` creates a missing `components.list` from installed `*_ac.exe` names that already have `config/<name>.ini`.

`journal_config.exe` owns only `config/journal.ini`. `--seed` and `--init` delegate every other Journal store to the executable that writes it. The no-argument menu still runs `journal_builder.exe --seed` after the INI is present. Alias lines are `name = factory(arguments)`. Blank lines are ignored. `journal_ac.exe` loads them at startup. Restart Journal after an alias change. Main expands an alias only when `keymap.map` uses that name. A missing alias file is skipped at startup and is not created by `journal_ac.exe`.

## Copying `dist/` to another Windows 11 PC

Copy the built `dist/` folder, then retune machine-specific files. There is no installer.

Usually keep: `bin/` executables and DLLs; live `config/` files when paths are relative; `keymap.map`; `components.list`; journal `<factory_name>.list` alias files and `series.db`; writer files, including notes; `taskbar/applications/*.map` when those programs exist on the new PC; `components/server/` when `document_root.id` stays relative. An absolute `document_root.id` points at a path on that PC.

Retune on the new PC:

- Install the matching **MSVC v145** redistributable if that PC did not build the binaries.
- **Spotify:** do not copy `components/spotify/tokens.map`. Run `spotify_oauth.exe` on the new machine. `spotify_editor.exe` writes `client.id` and `devices.list`. `spotify_db.exe` creates `history.db`. See [Spotify](spotify.md).
- **`config/logging.ini`:** if `directory` is an absolute path, point it at a path that exists on the new PC, or reset to `logs`.
- **`taskbar_builder.exe`:** re-run Discover taskbar applications when pinned programs or exe paths differ. `taskbar_config.exe` changes only `config/taskbar.ini`.
- **Dash:** the vault is not portable between Windows installs.
- Optional: `journal_config.exe`, `writer_config.exe`, or `server_config.exe` only if those paths should change.

## Keymap

The user map is `dist/keymap.map`. `keymap/keymap_commands.txt` is the editor autocomplete list. `keymap_editor.exe` writes a seed `keymap.map` if that file is missing (`numpad_0` / `numpad_1` filled, other keys left blank as `key =`). An existing `keymap.map` is not overwritten. `keymap_config.exe` writes `config/keymap.ini` only. Runtime never writes `keymap.map`.

| File | Purpose |
| --- | --- |
| `../keymap.map` | The key map. Valid key names are `key_codes::keys`; `enter` is not a name. |
| `keymap_commands.txt` | Generated list of registered command expressions for editor autocomplete, including alias names loaded from Journal's per-factory `.list` files. Rewritten on start; skipped if the bytes already match. |

`auto_core.exe` rewrites `keymap/keymap_commands.txt` during keymap initialization when the bytes differ.

`keymap.map` lines are `key = primary | secondary`. Canonical writers use spaces around `=` and `|`. A fully unset binding is written as `key =`. `key =`, `key = |`, and `key = primary | secondary` are the same unset pair. The word `primary` is unset only on the primary side, and `secondary` only on the secondary side. A swapped pair such as `key = secondary | primary` is an invalid line. A blank side (`activate_word |`) stays blank. `[...]` headers and `;` / `#` comments are ignored. A `|` inside `()` or quotes is not the action split. A second top-level `|` is invalid. Each side is resolved on its own: a valid name still binds when the other is empty or unknown. Empty both sides leaves the key unbound and is not logged as invalid. A missing line is the same as unbound. After a successful load, each `key_codes` name not in `active_keymap` prints `numpad 2 hasn't been set` (underscore becomes a space), unless `silence_nonset_warning` is exactly `on`. Unbound keys are not in `active_keymap`, so the hook calls `CallNextHookEx` and the physical key keeps its Windows behavior. A key with at least one filled side is in the map and eats the keystroke (`return 1`); an empty side is a no-op. Unknown command expressions print `numpad 2 is set to an incorrect value: …` at load and again on press of that side. Unknown keys and malformed lines are logged and skipped. Zero usable rows (no resolved command on either side of any key), a file that cannot be opened, or workspace creation failure installs an in-memory emergency map (`numpad_0` F-lock, `numpad_1` activate Auto Core / close) and does not rewrite `keymap.map`. Unset messages are not printed after emergency fallback.

See [development.md](development.md) for registering commands, and [Why keymap.map is the only map](main.md#why-keymapmap-is-the-only-map) for the measured cost of the dropped compiled table.

## INI key ledger

Phase 1A discovery. Every supported key is listed. This ledger is not a rename and not the frozen schema. **Disposition** is `keep` when the key already matches a Finalized rule, and `decision required` when the name or the invalid-value behavior is still a Phase 1B question. Readers do not rewrite INI files.

Shared rules, already **Finalized** (C4, C5, C10), unless a row says otherwise:

- The file is `config/<stem>.ini`. Only the matching `_config.exe` writes it. `--seed` creates it only when missing. A missing or unreadable file uses compiled defaults in memory and does not create the file. `auto_core.ini` is the exception that means "initialized": its absence starts first-run, and a failed required step may delete a file that same run created.
- A missing key or an invalid value keeps that key's compiled default and does not rewrite the file.
- Boolean switches are lowercase `on` and `off`. Production INI parsers do not accept `true`, `false`, `yes`, or `no` as switches. `crash_recovery.ini` `default_response` uses `yes` and `no` because it is an answer. Interactive prompts may accept `y`/`n`; those are not stored spellings.
- `logging` on a component or host family file is the family switch. It is not the shared policy. The shared policy is `logging.ini`. Dash's compiled default and missing-key fallback are `off` (**Exception**, Finalized). Every other family falls back to `component_logging_default` (compiled `on`).
- Unknown keys are not read. A later rewrite by the owner writes only the keys that owner serializes, so a leftover key disappears on rewrite. That is not a dedicated legacy parser. `tab_end`, `port`, and `document_root` are not keys.

`components.ini` is the exception to exact lowercase booleans: `new_components`, `sort_components`, and `remove_missing_components` are compared after ASCII lowercasing, so `ON` matches `on`. Other bool parsers require exact `on` or `off`. **Inconsistent** case folding. Disposition of the folding: decision required. The key names stay `keep`.

### Cross-file names

| Concept | Keys | State |
| --- | --- | --- |
| Data directory | `directory` in itunes, journal, server, spotify, taskbar, wake, writer, and logging | Observed. Same word, section selects which tree. Proposed canonical keeps `directory`. Disposition keep. |
| Notes folder | `notes_subdirectory` | Observed. Relative to the Writer directory. Disposition keep. |
| Family logging | `logging` | Observed on every family INI, including `logger.ini` and `slash.ini`. Disposition keep. |
| Shared log policy | `disable_all`, `write_logs_to_files`, `write_logs_to_console`, `log_print_mode`, `component_logging_default` | Finalized with C15. Disposition keep. |
| Discovery mode vs report mode | `taskbar.ini` `mode` (`live`/`cache`) and `slash.ini` `mode` (`verbose`/`concise`/`silent`) | Observed. Same key, different enums. Disposition decision required. Do not rename in this pass. |
| Start-like booleans | `auto_start`, `auto_launch_oauth`, `auto_select_new_series` | Observed. Different behaviors. Disposition keep the three names apart. |
| Negative or verb keys | `disable_all`, `silence_nonset_warning`, `remove_missing_components`, `warn_without_winkey_mapping` | Observed. Disposition decision required for vocabulary only. |
| Units in the name | `shutdown_timeout_ms`, `merge_interval_seconds` | Observed. Timeout and interval stay distinct. Disposition keep. |
| Enum that is not a switch | `new_components` (`prompt`/`on`/`off`), `delayed_shutdown_prompt`, `log_print_mode`, `default_response` | Observed. Disposition keep the enums. `new_components` invalid-value behavior is separate below. |

### `config/auto_core.ini`

Writer `auto_core_config.exe`. Section `[auto_core]`. Readers: Main startup (presence), `auto_core.core.config` (`warn_without_winkey_mapping`), family logging for `auto_core.exe`.

| Key | Type | Allowed | Default | Invalid or missing | Disposition |
| --- | --- | --- | --- | --- | --- |
| `warn_without_winkey_mapping` | bool | `on`, `off` | `on` | default `on`, no rewrite | keep |
| `logging` | bool | `on`, `off` | `on` | `component_logging_default` | keep |

### `config/components.ini`

Writer `components_config.exe`. Section `[settings]`. This file is not `components.list`. Readers: component discovery and `components_editor.exe`.

| Key | Type | Allowed | Default | Invalid or missing | Disposition |
| --- | --- | --- | --- | --- | --- |
| `new_components` | enum | `prompt`, `on`, `off` | `on` | **Inconsistent** with C10: an unrecognized value becomes `prompt`, not `on` | decision required |
| `sort_components` | bool | `on`, `off` | `on` | keeps the previous or compiled default | keep |
| `remove_missing_components` | bool | `on`, `off` | `off` | keeps the previous or compiled default | keep |

### `config/crash_recovery.ini`

Writer `crash_recovery_config.exe`. Section `[crash_recovery]`. Readers: Main crash prompt, shared crash diagnostics.

| Key | Type | Allowed | Default | Invalid or missing | Disposition |
| --- | --- | --- | --- | --- | --- |
| `default_response` | answer | `yes`, `no` | `no` | keeps `no` | keep. Not a boolean switch. |
| `crash_diagnostics` | bool | `on`, `off` | `on` | keeps `on` | keep |

### `config/shutdown.ini`

Writer `shutdown_config.exe`. Section `[shutdown]`. Readers: Main shutdown.

| Key | Type | Allowed | Default | Invalid or missing | Disposition |
| --- | --- | --- | --- | --- | --- |
| `delayed_shutdown_prompt` | enum | `popup`, `console` | `popup` | keeps `popup` | keep |
| `shutdown_timeout_ms` | integer | milliseconds, shared generic-child wait | `2000` | keeps `2000` | keep |

### `config/logging.ini`

Writer `logging_config.exe`. Section `[logging]`. Readers: every process that links the DLL logging policy. `--disable` writes the compiled text with `disable_all = on` when the file is missing and leaves an existing file unchanged.

| Key | Type | Allowed | Default | Invalid or missing | Disposition |
| --- | --- | --- | --- | --- | --- |
| `disable_all` | bool | `on`, `off` | `off` | keeps `off` | keep |
| `directory` | path | relative to the installation root, or absolute | `logs` | empty uses `<installation_root>/logs` | keep |
| `write_logs_to_files` | bool | `on`, `off` | `on` | keeps `on` | keep |
| `write_logs_to_console` | bool | `on`, `off` | `off` | keeps `off` | keep |
| `log_print_mode` | enum | `log`, `print` | `print` | keeps `print` | keep |
| `component_logging_default` | bool | `on`, `off` | `on` | keeps `on` | keep |

### `config/logger.ini`

Writer `logger_config.exe`. Section `[logger]`. Merge keys are read by `logger_merge_config`. `logging` is the Logger family switch, read by the component logger, not by the merge resolver.

| Key | Type | Allowed | Default | Invalid or missing | Disposition |
| --- | --- | --- | --- | --- | --- |
| `merge_interval_seconds` | integer | seconds; `0` disables the periodic merge | `60` | keeps `60` | keep |
| `merge_logs_on_shutdown` | bool | `on`, `off` | `on` | keeps `on` | keep |
| `logging` | bool | `on`, `off` | `on` | `component_logging_default` | keep |

### `config/keymap.ini`

Writer `keymap_config.exe`. Section `[keymap]`. Reader: `auto_core.exe` keymap load. Exact `on` or `off`.

| Key | Type | Allowed | Default | Invalid or missing | Disposition |
| --- | --- | --- | --- | --- | --- |
| `silence_nonset_warning` | bool | `on`, `off` | `off` | keeps `off`. Only exact `on` silences | keep |

### `config/dash.ini`

Writer `dash_config.exe`. Section `[dash]`.

| Key | Type | Allowed | Default | Invalid or missing | Disposition |
| --- | --- | --- | --- | --- | --- |
| `logging` | bool | `on`, `off` | `off` | fallback `off`, not `component_logging_default` | keep. Finalized exception. |

### `config/itunes.ini`

Writer `itunes_config.exe`. Section `[itunes]`. Readers: `itunes_ac.exe` and the iTunes subsystems that resolve the data directory.

| Key | Type | Allowed | Default | Invalid or missing | Disposition |
| --- | --- | --- | --- | --- | --- |
| `directory` | path | relative to the installation root, or absolute | `components/itunes` | empty uses that default | keep |
| `auto_start` | bool | `on`, `off` | `on` | keeps `on` | keep |
| `logging` | bool | `on`, `off` | `on` | `component_logging_default` | keep |

### `config/journal.ini`

Writer `journal_config.exe`. Section `[journal]`.

| Key | Type | Allowed | Default | Invalid or missing | Disposition |
| --- | --- | --- | --- | --- | --- |
| `directory` | path | relative to the installation root, or absolute | `components\journal` | empty uses that default | keep |
| `auto_select_new_series` | bool | `on`, `off` | `on` | keeps `on` | keep |
| `remote_sync` | bool | `on`, `off` | `off` | keeps `off`. `true` is not accepted | keep |
| `logging` | bool | `on`, `off` | `on` | `component_logging_default` | keep |

### `config/server.ini`

Writer `server_config.exe`. Section `[server]`. `port` and `document_root` are editor seed values for `port.id` and `document_root.id`. They are not keys.

| Key | Type | Allowed | Default | Invalid or missing | Disposition |
| --- | --- | --- | --- | --- | --- |
| `directory` | path | relative to the installation root, or absolute | `components\server` | empty uses that default | keep |
| `logging` | bool | `on`, `off` | `on` | `component_logging_default` | keep |

### `config/slash.ini`

Writer `slash_config.exe`. Section `[slash]`.

| Key | Type | Allowed | Default | Invalid or missing | Disposition |
| --- | --- | --- | --- | --- | --- |
| `mode` | enum | `verbose`, `concise`, `silent` | `verbose` | keeps `verbose` | decision required. Same key name as Taskbar `mode`. |
| `logging` | bool | `on`, `off` | `on` | `component_logging_default` | keep |

### `config/spotify.ini`

Writer `spotify_config.exe`. Section `[spotify]`. `auto_launch_enabled` returns false for every value other than `on`, which matches the compiled default `off`. The comment above that function still names `true` and `false`. The function does not treat them as spellings of `on`. Disposition of the comment: legacy wording, not a parser.

| Key | Type | Allowed | Default | Invalid or missing | Disposition |
| --- | --- | --- | --- | --- | --- |
| `directory` | path | relative to the installation root, or absolute | `components\spotify` | empty uses that default | keep |
| `auto_launch_oauth` | bool | `on`, `off` | `off` | stays `off` | keep |
| `logging` | bool | `on`, `off` | `on` | `component_logging_default` | keep |

### `config/taskbar.ini`

Writer `taskbar_config.exe`. Section `[taskbar]`.

| Key | Type | Allowed | Default | Invalid or missing | Disposition |
| --- | --- | --- | --- | --- | --- |
| `directory` | path | relative to the installation root, or absolute | `taskbar` | empty uses that default | keep |
| `mode` | enum | `live`, `cache` | `live` | keeps `live` | decision required. Same key name as Slash `mode`. |
| `logging` | bool | `on`, `off` | `on` | `component_logging_default` | keep |

### `config/wake.ini`

Writer `wake_config.exe`. Section `[wake]`. `logging` does not gate `current.event`, `previous.event`, or `wake_events.log`.

| Key | Type | Allowed | Default | Invalid or missing | Disposition |
| --- | --- | --- | --- | --- | --- |
| `directory` | path | relative to the installation root, or absolute | `components\wake` | empty uses that default | keep |
| `logging` | bool | `on`, `off` | `on` | `component_logging_default` | keep |

### `config/writer.ini`

Writer `writer_config.exe`. Section `[writer]`. `notes_subdirectory` must be relative.

| Key | Type | Allowed | Default | Invalid or missing | Disposition |
| --- | --- | --- | --- | --- | --- |
| `directory` | path | relative to the installation root, or absolute | `writer` | empty uses that default | keep. DLL export of this path is a C12 finding, not a second key. |
| `notes_subdirectory` | relative path | one relative segment path | `notes` | empty or non-relative uses `<writer directory>/notes` | keep |
| `logging` | bool | `on`, `off` | `on` | `component_logging_default` | keep |

No other `config/*.ini` stem is written by a production executable. `*.local.ini` is not loaded (**Finalized**, C18).
