# Runtime configuration

Runtime configuration files belong in `dist/config`. These files control runtime behavior, logging, taskbar discovery settings, local server settings, journaling settings, key mapping, the Spotify data directory, and the Writer data directories. `dist/` is gitignored. Tracked portable defaults live in [`defaults/`](../defaults/) (repo `defaults/X` is the sample for runtime `dist/X`). Auto Core never reads `defaults/`. If a live file is missing, it is written once from the same defaults that live in code. Invalid, empty, or unknown values use those defaults in memory and do not rewrite an existing file. Defaults apply per key: a bad `port` does not discard a valid `document_root`.

| File | Purpose |
| --- | --- |
| `auto_core.ini` | `[auto_core]` `warn_without_winkey_mapping` (default `true`). When true, `taskbar_ac.exe` warns if Auto Core is not in taskbar positions 1–10. Booleans are lowercase `true`/`false` only; any other value keeps the default. Set `false` to silence the warning. A missing file is written once from the portable default and is not fatal. |
| `itunes.ini` | Whether iTunes starts automatically (`[itunes] auto_start`, default `false`) and the final tab copied when reading track data (`tab_end`, default `3`). Booleans are lowercase `true`/`false` only; any other value keeps the default. Invalid `tab_end` keeps `3`. A missing live file is written once from those defaults. An existing file is never overwritten. |
| `journal.ini` | `[journal]` `directory` (portable default `journal`, which is `dist/journal` when Auto Core runs from `dist`). Relative paths resolve against the executable directory, same as Spotify `directory`. A missing or empty `directory` uses `<exe>/journal`. Shared by `journal_ac.exe` and `journal_config.exe`. `journal_config.exe` prompts for `directory` before writing a missing live file. Blank `[journal] series` uses the last series table added in `journals.db`; a non-empty name that is not a table is reported. Also remote sync (`disable`) and `[timestamp]` `day_rollover_hour` (`0`). |
| `crash_recovery.ini` | `[dialog]` `default_response` (`yes` or `no`). Missing file or any other value defaults to No. |
| `logger.ini` | `[logger]` `enabled` (default `true`), `write_to_console` (default `false`), and `directory` (portable default `logs`, which is `dist/logs` when Auto Core runs from `dist`). Relative `directory` paths resolve against the executable directory, same as server `document_root`. A missing or empty `directory` uses `<exe>/logs`. Booleans are lowercase `true`/`false` only; any other value keeps the default. |
| `keymap.ini` | `[keymap] trace_enabled` and `silence_nonset_warning` (both default `false`). Must be exactly `true` to turn a flag on. |
| `server.ini` | `[server]` `port` (default `8585`) and `document_root` (default `server`, which is `dist/server` when Auto Core runs from `dist`). Relative `document_root` paths resolve against the executable directory. A missing live file is written once from those defaults (Main or any seeder). `server_config.exe` also writes it if missing (prompting for both keys) and can rewrite `port` or `document_root` from its menu without discarding the other key. Runtime never rewrites a bad `port` in the file; invalid values keep `8585` in memory. Shared by `server_ac.exe` and `server_config.exe`. See [Server](server.md) and [Server config](server_config.md). |
| `spotify.ini` | `[spotify]` `directory` (portable default `spotify`, which is `dist/spotify` when Auto Core runs from `dist`). Relative paths resolve against the executable directory, same as server `document_root`. A missing or empty `directory` uses `<exe>/spotify`. Shared by `spotify_ac.exe` and `spotify_oauth.exe`. See [Spotify](spotify.md). |
| `taskbar.ini` | `[taskbar]` `directory` (portable default `taskbar`, which is `dist/taskbar` when Auto Core runs from `dist`) and discovery `mode` (`live` or `cache`; default `live`). Relative `directory` paths resolve against the executable directory, same as journal and Spotify `directory`. A missing or empty `directory` uses `<exe>/taskbar`. Shared by `taskbar_ac.exe` and `taskbar_config.exe`. `taskbar_config.exe` prompts for `directory` before writing a missing live file. |
| `writer.ini` | `[writer]` `directory` (portable default `writer`, which is `dist/writer` when Auto Core runs from `dist`) and `notes_directory` (portable default `notes`, which is `dist/notes`). Relative paths resolve against the executable directory, same as journal and Spotify `directory`. A missing or empty `directory` uses `<exe>/writer`; a missing or empty `notes_directory` uses `<exe>/notes`. Shared by `writer_ac.exe`, Main, and `writer_config.exe`. A missing live file is written once from those defaults (Main or any seeder). `writer_config.exe` also writes it if missing (prompting for `directory`; blank stores `writer`) and always stores `notes_directory = notes` on that first write. Nest notes under the writer root by setting `notes_directory = writer/notes` (or an absolute path). See [Writer config](writer_config.md). |

The `dist/crash/` directory is created only when Auto Core records a crash. The marker file is `crash/.crash`. Dated `crash/<date>_crash` folders hold `crash.log` plus copies of `auto_core.exe` and `auto_core.pdb`; a same-day collision uses `<date>_crash_N`. If the marker exists at the next start, Auto Core asks whether to continue. Yes removes the marker and continues; No exits and leaves the marker. Recovery stays disabled until that prompt completes. See [main.md](main.md).

## Logging

Runtime logs default to `<executable directory>/logs` (`ac::paths::log_directory()`). The portable INI sets `[logger] directory = logs`, which is `dist/logs` when Auto Core runs from `dist`. A relative path is resolved against the executable directory; an absolute path is used as-is. A missing or empty `directory` keeps `<exe>/logs`.

The log root holds dated `YYYY-MM-DD_main.log` files from `logger_ac.exe`. Each `ac::Component` writes `{date}_{name}.log` under `components/<name>/`. Wake extras (`wake_latest.log`, `wake_previous.log`, `wake_master.log`) live in `components/wake/` next to the daily wake log. Old root-level `<name>/` folders are not moved.

`[logger] enabled` gates whether Main starts `logger_ac.exe` and calls `Component::connect_to_logger()`. Local `{date}_{name}.log` files still run when central logging is off. Main creates the log directory during logger init either way.

Do not commit machine-specific log output.

## Runtime data

Treat the following as local runtime state rather than portable project source:

- `dist/spotify/` — Spotify tokens, device codes, and `spotify_history.db`
- `dist/journal/` — `journals.db` and `journal_choices.ini`. The tracked sample is [`defaults/journal/journal_choices.ini`](../defaults/journal/journal_choices.ini). Override the folder with `[journal] directory` in `config/journal.ini`.
- `dist/taskbar/` — generated per-program INIs in `applications/` (`taskbar_config.exe`) and `cached_positions.ini` (Win+1 through Win+10 cache from `taskbar_ac.exe`). Override the folder with `[taskbar] directory` in `config/taskbar.ini`.
- `dist/server/` — local document root. Build copies [`defaults/server/`](../defaults/server/) (`index.html`, `styles.css`) here; extra files stay local (see [Server](server.md)).
- `dist/writer/` — `gpt_prompts.txt` and `task_list.txt`. Override the folder with `[writer] directory` in `config/writer.ini`.
- `dist/notes/` — dated `YYYY-MM-DD.txt` notes. Override the folder with `[writer] notes_directory` in `config/writer.ini`.
- `dist/logs/` — log output
- `dist/crash/` — crash marker and dated diagnostic copies
- `dist/errors/` — `errors.log` from best-effort error reporting
- `dist/symbols/` — debug symbol files (`.pdb`)

`dist/` is gitignored. Tracked samples live under [`defaults/`](../defaults/) (`defaults/config/*.ini`, [`defaults/keymap/bindings.ini`](../defaults/keymap/bindings.ini), [`defaults/journal/journal_choices.ini`](../defaults/journal/journal_choices.ini), [`defaults/server/`](../defaults/server/)). Auto Core does not load `defaults/` or `*.local.ini`.

[`defaults/keymap/bindings.ini`](../defaults/keymap/bindings.ini) is the sample map shipped in the repository. Copy it to `dist/keymap/bindings.ini` if you want that starting point, or let Auto Core write a seed if the live file is missing. An existing `bindings.ini` is never overwritten.

[`defaults/journal/journal_choices.ini`](../defaults/journal/journal_choices.ini) is the sample print-choice alias table. A missing live `journal_choices.ini` is written once from those portable defaults. An existing file is never overwritten. Auto Core does not read repo `defaults/`. Alias lines are `name = factory_expression` (`;` / `#` comments; optional `[journal_choices]` header ignored). Main expands an alias only when `bindings.ini` uses that name.

## Copying `dist/` to another Windows 11 PC

Copy the built `dist/` folder, then retune machine-specific files. There is no installer.

Usually keep: executables, `auto_core.dll`, vendor DLLs; live `config/*.ini` when paths are relative; `keymap/bindings.ini`; `journal/journal_choices.ini` and `journals.db`; writer and notes files; `taskbar/applications/*.ini` when those programs exist on the new PC; `server/` content.

Retune on the new PC:

- Install the matching **MSVC v145** redistributable if that PC did not build the binaries.
- **Spotify:** do not copy `spotify/spotify_tokens.ini`. Run `spotify_oauth.exe` on the new machine. See [Spotify](spotify.md).
- **`config/logger.ini`:** if `directory` is an absolute path, point it at a path that exists on the new PC, or reset to `logs`.
- **`taskbar_config.exe`:** re-run when pinned programs or exe paths differ.
- **Dash:** the vault is not portable between Windows installs.
- Optional: `journal_config.exe`, `writer_config.exe`, or `server_config.exe` only if those paths should change.

## Keymap

The `dist/keymap` folder contains the keymap workspace. Auto Core creates `keymap/` and `keymap/components/`. If `bindings.ini` is missing, it writes a seed of every `key_codes` name (`numpad_0` / `numpad_1` filled, others `{, }`). An existing `bindings.ini` is not overwritten. `keymap_commands.txt` at the keymap root is not a component catalog.

| File | Purpose |
| --- | --- |
| `bindings.ini` | The key map. Valid key names are `key_codes::keys`; `enter` is not a name. |
| `keymap_commands.txt` | Generated list of registered command expressions for editor autocomplete, including parsed `journal_choices.ini` alias names. Rewritten on start; skipped if the bytes already match. |
| `components/<component>.keymap_commands.txt` | Per-component command catalogs (generated). `journal.keymap_commands.txt` is rewritten on every successful workspace init (factories, protocol names, parsed alias names). |

Component catalogs keep the basename `component_name.keymap_commands.txt` inside `components/`.

`bindings.ini` lines are `key = {primary, secondary}`. An optional `[keymap]` header and `;` / `#` comments are ignored. A comma inside `()` or quotes is not the action split. Each side is resolved on its own: a valid name still binds when the other is empty or unknown. `{, }` (both actions empty) leaves the key unbound and is not logged as invalid. A missing line is the same as `{, }`. After a successful load, each `key_codes` name not in `active_keymap` prints `numpad 2 hasn't been set` (underscore becomes a space), unless `silence_nonset_warning` is exactly `true`. Unbound keys are not in `active_keymap`, so the hook calls `CallNextHookEx` and the physical key keeps its Windows behavior. A key with at least one filled side is in the map and eats the keystroke (`return 1`); an empty side is a no-op. Unknown command expressions print `numpad 2 is set to an incorrect value: …` at load and again on press of that side. Unknown keys and malformed lines are logged and skipped. Zero usable rows (no resolved command on either side of any key), a file that cannot be opened, or workspace creation failure installs an in-memory emergency map (`numpad_0` F-lock, `numpad_1` activate Auto Core / close) and does not rewrite `bindings.ini`. Unset messages are not printed after emergency fallback.

See [development.md](development.md) for registering commands, and [Why bindings.ini is the only map](main.md#why-bindingsini-is-the-only-map) for the measured cost of the dropped compiled table.
