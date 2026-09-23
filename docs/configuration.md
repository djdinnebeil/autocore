# Runtime configuration

Runtime configuration files belong in `dist/config`. **Only `_config.exe` programs create or rewrite `.ini` files.** Runtime programs (`auto_core.exe` and `<name>_ac.exe`) parse their INIs and never write them. If a required INI is missing, unreadable, missing a section or key, or has an invalid value, the process uses the documented default in memory and reports the condition with `log_and_print`. Typed defaults for children live in `app/components/<name>/shared/defaults.ixx`. Main helper defaults live in [`app/main/shared/defaults.ixx`](../app/main/shared/defaults.ixx). Tracked samples under [`defaults/`](../defaults/) must match those defaults; Auto Core never reads `defaults/`.

If a component INI is missing or malformed, `<name>_ac.exe` uses `shared/defaults.ixx` in memory and `Component::report_ini_unavailable` (`log_and_print`): run `<name>_config.exe`; defaults are in use; the file will not be created. Invalid, empty, or unknown **keys** in a readable file keep that key's default and do not rewrite the file. Defaults apply per key: a bad `port` does not discard a valid `document_root`.

Relative `directory` and `document_root` values resolve against the **installation root** (`dist/` after a normal build), never against `dist/bin/` or the process current working directory. `auto_core.exe` lives at `dist/bin/auto_core.exe`. `bin_directory` is that folder; `installation_root` is its parent.

`auto_core.exe` checks that `config/auto_core.ini` exists. If it is missing, Main launches `auto_core_config.exe` and waits. That helper runs `main_config.exe`, `components_config.exe`, `keymap_config.exe`, `keymap_editor.exe`, `shutdown_config.exe`, and `crash_recovery_config.exe`, then writes `auto_core.ini` only after all six succeed. If `auto_core.ini` already exists, `auto_core_config.exe` opens a menu instead of repeating first-time initialization. If `components.list` is missing, Main launches `components_editor.exe` with no arguments and waits. Runtime never writes that file. If the helper fails or the list is still missing, Main exits `1`.

| File | Purpose |
| --- | --- |
| `auto_core.ini` | Existence sentinel written by `auto_core_config.exe`. Not parsed. Contents are `[auto_core] initialized = true` plus comments. |
| `main.ini` | `[main]` `warn_without_winkey_mapping` (default `true`). When true, `taskbar_ac.exe` warns if Auto Core is not in taskbar positions 1–10. Booleans are lowercase `true`/`false` only; any other value keeps the default. Set `false` to silence the warning. Written only by `main_config.exe`. Missing or malformed: in-memory default `true`, `log_and_print`, no file create. |
| `components.ini` | Component-system `[settings]` only (`new_components` = `prompt`/`on`/`off`, `sort_components` = `on`/`off`, `remove_missing_components` = `on`/`off`; defaults `on` / `on` / `off`). Exclusive writer: `components_config.exe`. That helper also launches `bin/<name>_config.exe` when `config/<name>.ini` is missing (existence check only; no schema validation) and may offer a no-arg `components_editor.exe` afterward. Settings are read case-insensitively and written lowercase. Missing INI during sync: compiled defaults in memory, no file create. |
| `../components.list` | Installation-root component registry. Exclusive writer: `components_editor.exe`. `[components]` lines are `name`, `name on`, or `name off` (blank defaults to on; any other token is malformed and disables that name). A readable list is authoritative: a missing name is disabled. Names are case-sensitive and lowercase-only (`^[a-z][a-z0-9_]*$`); invalid names are not normalized. Duplicate names use last-wins. Known specials (`logger`, `dash`, `slash`) use the same rules but are not v1 session children. The portable seed lists the ten known names with blank (on) values. A missing list is reconstructible: no-arg `components_editor.exe` rebuilds it from installed `*_ac.exe` names that already have `config/<name>.ini`, applying `new_components`, `remove_missing_components`, and `sort_components` from `components.ini` or compiled defaults. `--component <name>` requires `config/<name>.ini` to exist, applies `new_components` to that name only, and does not prune. There is no legacy catalog migration. `<name>_config.exe` must not edit this file; after a successful INI write it launches `components_editor.exe --component <name>`. If Main cannot produce a readable list, it exits `1`. An existing unreadable list: `log_and_print`, enable every valid `*_ac.exe` in `bin_directory`, no file create. |
| `itunes.ini` | Whether iTunes starts automatically (`[itunes] auto_start`, default `false`) and the final tab copied when reading track data (`tab_end`, default `3`). Booleans are lowercase `true`/`false` only; any other value keeps the default. Invalid `tab_end` keeps `3`. Written only by `itunes_config.exe`. Missing or malformed: in-memory defaults, no file create. See [iTunes](itunes.md). |
| `journal.ini` | `[journal]` `directory` (portable default `components\journal`, which is `dist/components/journal` when Auto Core runs from `dist`). Relative paths resolve against the installation root. A missing or empty `directory` uses `<installation_root>/components/journal`. Written only by `journal_config.exe`. Blank `[journal] series` uses the last series table added in `journals.db`; a non-empty name that is not a table is reported. Also remote sync (`disable`) and `[timestamp]` `day_rollover_hour` (`0`). |
| `crash_recovery.ini` | `[dialog]` `default_response` (`yes` or `no`). Missing file or any other value defaults to No. Written only by `crash_recovery_config.exe`. Missing or malformed: in-memory default No, `log_and_print`, no file create. |
| `shutdown.ini` | `[shutdown] delayed_shutdown_prompt` (`popup` or `console`, default `popup`) and one shared `shutdown_timeout_ms` (default `2000`) used for generic children and `logger_ac.exe`. Invalid values keep their defaults. Console mode keeps Main alive at its recovery prompt until input arrives. Written only by `shutdown_config.exe`. Missing or malformed: in-memory defaults, `log_and_print`, no file create. |
| `logger.ini` | `[logger]` `directory` (portable default `logs`, which is `dist/logs` when Auto Core runs from `dist`) and `write_to_console` (portable default `false`). Relative `directory` paths resolve against the installation root. A missing or empty `directory` uses `<installation_root>/logs`. Documented booleans are lowercase `on`/`off`. `true` is an alias for `on`, and `false` is an alias for `off`. Any other value keeps the default. Central logging is enabled from `components.list`, not this file. Written only by `logger_config.exe`. |
| `keymap.ini` | `[keymap] trace_enabled` and `silence_nonset_warning` (both default `false`). Must be exactly `true` to turn a flag on. Written only by `keymap_config.exe`. Missing or malformed: in-memory defaults, `log_and_print`, no file create. |
| `server.ini` | `[server]` `port` (default `8585`) and `document_root` (default `components\server`, which is `dist/components/server` when Auto Core runs from `dist`). Relative `document_root` paths resolve against the installation root. Written only by `server_config.exe` (prompts for both keys; menu can rewrite either without discarding the other). Missing or malformed: in-memory defaults (`8585` / `components\server`), no file create. See [Server](server.md). |
| `spotify.ini` | `[spotify]` `directory` (portable default `components\spotify`, which is `dist/components/spotify` when Auto Core runs from `dist`). Relative paths resolve against the installation root. A missing or empty `directory` uses `<installation_root>/components/spotify`. Written only by `spotify_config.exe`. See [Spotify](spotify.md). |
| `taskbar.ini` | `[taskbar]` `directory` (portable default `taskbar`, which is `dist/taskbar` when Auto Core runs from `dist`) and discovery `mode` (`live` or `cache`; default `live`). Relative `directory` paths resolve against the installation root. A missing or empty `directory` uses `<installation_root>/taskbar`. Written only by `taskbar_config.exe`. |
| `writer.ini` | `[writer]` `directory` (portable default `writer`, which is `dist/writer` when Auto Core runs from `dist`) and `notes_directory` (portable default `notes`, which is `dist/notes`). Relative paths resolve against the installation root. A missing or empty `directory` uses `<installation_root>/writer`; a missing or empty `notes_directory` uses `<installation_root>/notes`. Written only by `writer_config.exe`. See [Writer](writer.md). |
| `dash.ini` | `[dash]` with no keys yet. The vault stays under `%LOCALAPPDATA%\Auto Core`. Written only by `dash_config.exe`. Missing or malformed: in-memory defaults, no file create. See [Dash](dash.md). |
| `wake.ini` | `[wake]` with no keys yet. Written only by `wake_config.exe`. See [Wake](wake.md). |
| `slash.ini` | `[slash]` with no keys yet. Written only by `slash_config.exe`. See [Slash](slash.md). |

The `dist/crash/` directory is created only when Auto Core records a crash. The marker file is `crash/.crash`. Dated `crash/<date>_crash` folders hold `crash.log` plus copies of `auto_core.exe` and `auto_core.pdb`; a same-day collision uses `<date>_crash_N`. If the marker exists at the next start, Auto Core asks whether to continue. Yes removes the marker and continues; No exits and leaves the marker. Recovery stays disabled until that prompt completes. See [main.md](main.md).

## Logging

Runtime logs default to `<installation root>/logs` (`ac::paths::log_directory()`). The portable INI sets `[logger] directory = logs`, which is `dist/logs` when Auto Core runs from `dist`. A relative path is resolved against the installation root; an absolute path is used as-is. A missing or empty `directory` keeps `<installation_root>/logs`.

The log root holds dated `YYYY-MM-DD_main.log` files from `logger_ac.exe`. Every executable has its own logging identity and its own directory under `logs/components/`. An `ac::Component` named `<name>` writes a comprehensive `{date}_{name}.log` and a central-subset `{date}_{name}.main.log` under `components/<name>/`. Every `.main.log` event is also present in the comprehensive log with the same source-generated millisecond timestamp. Session start/end/`***` markers go to `{date}_{name}.log`.

`log()` writes only `{date}_{name}.log`. `log_and_log()` also writes `{date}_{name}.main.log` and the central logger. `log_and_print()` adds console. A utility does not write into another executable's directory: `spotify_config.exe` uses `ac::Component{"spotify_config"}` and logs under `components/spotify_config/`, and `spotify_oauth.exe` logs under `components/spotify_oauth/`. Main helpers use their own identities (`auto_core_config`, `main_config`, `components_config`, `keymap_config`, `shutdown_config`, `crash_recovery_config`, `keymap_editor`, `components_editor`), not subdirectories of `components/auto_core/`.

Wake extras (`wake_latest.log`, `wake_previous.log`, `wake_master.log`) live in `components/wake/` next to the daily wake log. Old root-level `<name>/` folders are not moved.

Component records use `[YYYY-MM-DD HH:MM:SS.mmm] Message`. Live centralized records use `[YYYY-MM-DD HH:MM:SS.mmm] [component] Message`; `logger_ac.exe` preserves the timestamp generated by the originating component.

`logger_ac.exe` is feature-complete for the Auto Core pre-release and is now in field-testing/maintenance status. Its current API integration, timestamp transport, component `.main.log` integration, centralized formatting, synchronization, rollover, and lifecycle behavior should remain stable. Further implementation changes are limited to concrete defects, regressions, reliability issues, or unmet pre-release requirements found during field testing.

A future `log_merger_ac.exe` is planned to merge the daily component `.main.log` files chronologically into the daily centralized `main.log`. It is not implemented yet. During migration, the live named-pipe logger remains authoritative so its output can later be compared with merger output for coverage. Incremental scanning, reconciliation, and merge scheduling are intentionally deferred with the merger.

`logger` enabled in `components.list` gates whether Main starts `logger_ac.exe` and calls `Component::connect_to_logger()`. Local `{date}_{name}.log` and `{date}_{name}.main.log` files still run when central logging is off. Main creates the log directory during logger init either way.

Do not commit machine-specific log output.

## Runtime data

Treat the following as local runtime state rather than portable project source:

- `dist/bin/` — runtime executables and DLLs
- `dist/components.list` — component enablement list
- `dist/keymap.map` — user key-to-command map
- `dist/components/spotify/` — Spotify tokens, device codes, and `spotify_history.db`
- `dist/components/journal/` — `journals.db` and `journal_choices.ini`. The tracked sample is [`defaults/components/journal/journal_choices.ini`](../defaults/components/journal/journal_choices.ini). Override the folder with `[journal] directory` in `config/journal.ini`.
- `dist/taskbar/` — generated per-program INIs in `applications/` (`taskbar_config.exe`) and `cached_positions.ini` (Win+1 through Win+10 cache from `taskbar_ac.exe`). Override the folder with `[taskbar] directory` in `config/taskbar.ini`.
- `dist/components/server/` — local document root. Build copies [`defaults/server/`](../defaults/server/) (`index.html`, `styles.css`) here; extra files stay local (see [Server](server.md)).
- `dist/writer/` — `gpt_prompts.txt` and `task_list.txt`. Override the folder with `[writer] directory` in `config/writer.ini`.
- `dist/notes/` — dated `YYYY-MM-DD.txt` notes. Override the folder with `[writer] notes_directory` in `config/writer.ini`.
- `dist/logs/` — log output
- `dist/crash/` — crash marker and dated diagnostic copies
- `dist/errors/` — `errors.log` from best-effort error reporting
- `dist/symbols/` — debug symbol files (`.pdb`)

`dist/` is gitignored. Tracked samples live under [`defaults/`](../defaults/) (`defaults/config/`, [`defaults/keymap.map`](../defaults/keymap.map), [`defaults/components.list`](../defaults/components.list), [`defaults/components/journal/journal_choices.ini`](../defaults/components/journal/journal_choices.ini), [`defaults/server/`](../defaults/server/)). Auto Core does not load `defaults/` or `*.local.ini`.

[`defaults/keymap.map`](../defaults/keymap.map) is the sample map shipped in the repository. Copy it to `dist/keymap.map` if you want that starting point, or run `keymap_editor.exe` to write a seed if the live file is missing. An existing `keymap.map` is never overwritten.

[`defaults/components/journal/journal_choices.ini`](../defaults/components/journal/journal_choices.ini) is the sample print-choice alias table. `journal_config.exe` writes a missing live `journal_choices.ini` from those portable defaults. An existing file is never overwritten. Auto Core does not read repo `defaults/`. Alias lines are `name = factory_expression` (`;` / `#` comments; optional `[journal_choices]` header ignored). Main expands an alias only when `keymap.map` uses that name.

## Copying `dist/` to another Windows 11 PC

Copy the built `dist/` folder, then retune machine-specific files. There is no installer.

Usually keep: `bin/` executables and DLLs; live `config/` files when paths are relative; `keymap.map`; `components.list`; journal `journal_choices.ini` and `journals.db`; writer and notes files; `taskbar/applications/*.ini` when those programs exist on the new PC; `components/server/` content.

Retune on the new PC:

- Install the matching **MSVC v145** redistributable if that PC did not build the binaries.
- **Spotify:** do not copy `components/spotify/spotify_tokens.ini`. Run `spotify_oauth.exe` on the new machine. See [Spotify](spotify.md).
- **`config/logger.ini`:** if `directory` is an absolute path, point it at a path that exists on the new PC, or reset to `logs`.
- **`taskbar_config.exe`:** re-run when pinned programs or exe paths differ.
- **Dash:** the vault is not portable between Windows installs.
- Optional: `journal_config.exe`, `writer_config.exe`, or `server_config.exe` only if those paths should change.

## Keymap

The user map is `dist/keymap.map`. Supporting catalogs stay under `dist/keymap/`. `keymap_editor.exe` writes a seed `keymap.map` if that file is missing (`numpad_0` / `numpad_1` filled, others `key = primary | secondary`). An existing `keymap.map` is not overwritten. `keymap_config.exe` writes `config/keymap.ini` only. Runtime never writes `keymap.map`. `keymap_commands.txt` at the keymap root is not a component catalog.

| File | Purpose |
| --- | --- |
| `../keymap.map` | The key map. Valid key names are `key_codes::keys`; `enter` is not a name. |
| `keymap_commands.txt` | Generated list of registered command expressions for editor autocomplete, including parsed `journal_choices.ini` alias names. Rewritten on start; skipped if the bytes already match. |
| `components/<component>.keymap_commands.txt` | Per-component command catalogs (generated). `journal.keymap_commands.txt` is rewritten on every successful workspace init (factories, protocol names, parsed alias names). |

Component catalogs keep the basename `component_name.keymap_commands.txt` inside `keymap/components/`.

`keymap.map` lines are `key = primary | secondary`. Canonical writers use spaces around `=` and `|`. `key =`, `key = |`, and `key = primary | secondary` are the same unset pair, and the canonical form written back is `key = primary | secondary`. The word `primary` is unset only on the primary side, and `secondary` only on the secondary side. A swapped pair such as `key = secondary | primary` is an invalid line. A blank side (`activate_word |`) stays blank. `[...]` headers and `;` / `#` comments are ignored. A `|` inside `()` or quotes is not the action split. A second top-level `|` is invalid. Each side is resolved on its own: a valid name still binds when the other is empty or unknown. Empty both sides leaves the key unbound and is not logged as invalid. A missing line is the same as unbound. After a successful load, each `key_codes` name not in `active_keymap` prints `numpad 2 hasn't been set` (underscore becomes a space), unless `silence_nonset_warning` is exactly `true`. Unbound keys are not in `active_keymap`, so the hook calls `CallNextHookEx` and the physical key keeps its Windows behavior. A key with at least one filled side is in the map and eats the keystroke (`return 1`); an empty side is a no-op. Unknown command expressions print `numpad 2 is set to an incorrect value: …` at load and again on press of that side. Unknown keys and malformed lines are logged and skipped. Zero usable rows (no resolved command on either side of any key), a file that cannot be opened, or workspace creation failure installs an in-memory emergency map (`numpad_0` F-lock, `numpad_1` activate Auto Core / close) and does not rewrite `keymap.map`. Unset messages are not printed after emergency fallback.

See [development.md](development.md) for registering commands, and [Why keymap.map is the only map](main.md#why-keymapmap-is-the-only-map) for the measured cost of the dropped compiled table.
