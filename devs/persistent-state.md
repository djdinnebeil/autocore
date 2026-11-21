# Persistent state

Phase 1A discovery of every file that survives process exit. INI keys are in [configuration.md](configuration.md). This ledger is what the general sole-writer rule (C9, **Proposed canonical**) has to match. It is not that rule made normative.

Paths are relative to the installation root (`dist/` after a normal build) unless the row says otherwise. A Component data directory comes from that Component's `directory` key. Compiled defaults are in [configuration.md](configuration.md).

Shared missing-file behavior for non-INI files is **Proposed canonical** and not yet a conformance rule: the owner's `--seed` creates a missing file, and readers use a documented in-memory default or fail the operation. Rows below record what the code does now (**Observed**).

Logs and crash reports are written by whichever process produced them. That is many writers of one directory pattern, and one writer of each resulting file. It does not fit "one executable owns the pattern" without an exception. Disposition of that exception: decision required.

## Installation root

| Path | Purpose | Sole writer | Readers | Created | Missing | Portable | Secrets | First-run | Legacy | Disposition |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `config/*.ini` | settings | `<name>_ac.exe --config` for a component; `auto_core_settings.exe --config` for a Main host file | the family that names the file | `--seed` or `--init` when missing | compiled defaults, no create | yes, if paths stay relative | no | yes, through the orchestrators | unknown keys drop out on rewrite. No `true`/`false` spellings in production parsers | keep. See the INI ledger. |
| `components.list` | enablement | `auto_core_components.exe` | Main and each component settings menu | `--edit`, or `--edit --seed` when the file is missing | runtime enables every valid `*_ac.exe` and does not create the file | yes | no | `auto_core_components.exe --edit --seed` | none. Lines are `name`, `name on`, or `name off`. Updates are published from a same-directory temporary file. A failed publication can leave that temporary file. | keep. Finalized. |
| `data/auto_core/components.list.lock` | exclusive update lock for `components.list` | `auto_core_components.exe` creates a missing `data/auto_core` and the lock file on the first update and does not delete or replace the lock file | none. It is not opened with delete sharing. | first list update. A missing `data/auto_core` is created automatically | if `data/auto_core` cannot be created, or the lock file cannot be opened or locked, the update stops before the list is read or published and does not create `components.list` | yes | no | created when the first list update runs | empty coordination file. Not a backup or a recovery log. | keep. |
| `keymap.map` | key bindings and the generated `[commands]` name list | `auto_core_keymap.exe` | `auto_core.exe` | `--seed` creates a missing regular file with `CREATE_NEW`, ending in a blank line and an empty `[commands]` section. `--build` creates a missing file from the short template, a blank line, and `[commands]` filled from the command names, without replacing a file that appears first. A directory or other non-file is not replaced | in-memory emergency map, no create | yes | no | `auto_core_keymap.exe --seed` during first-run | `--seed` leaves an existing regular file unchanged. `--build` preserves the bytes before one final `[commands]` section and replaces that section with command names. It inserts one blank line before `[commands]` when the preserved text does not already end with one, and keeps blank lines that are already there. It leaves the file unchanged when the section is duplicated, another section follows it, a binding is inside it, the read fails, or the file contains a NUL byte. A failed publication removes only that temporary file. Catalog files and `keymap.map` are not one transaction. `[commands]` stays names only | keep. |
| `data/auto_core/keymap_build.lock` | exclusive lock for one `--build` | `auto_core_keymap.exe` creates it on the first build and does not delete or replace it | none. It is not opened with delete sharing | first `--build` | that build fails and does not export or replace catalogs, `keymap.map`, `keymap/activate_commands.map`, or `keymap/shared_commands.map` | yes | no | not created by first-run | empty coordination file. The byte-range lock is released when the process exits | keep. |
| `keymap/keymap_commands.txt` | autocomplete catalog | `auto_core.exe` during keymap initialization, and `auto_core_keymap.exe --build` | keymap editor consumers | rewritten when bytes differ. Each publication uses a uniquely named temporary file beside the destination | editor autocomplete lacks the generated list | yes | no | written on the next keymap init | `name [Provider] - description` when the description is non-empty, or `name [Provider]` when it is empty. `[Unset]` is used only for an explicit blank that still has two or more providers, and that row is not added to `keymap.map` `[commands]`. A command nobody still offers is omitted. `[commands]` stays the resolved names, published aliases, and unique commands. The description is not copied into `[commands]`. The last successful publication wins. A failed publication removes only that invocation's temporary file. Earlier files from the same `--build` are not rolled back | keep. |

## Logs, crashes, and errors

| Path | Purpose | Writer | Readers | Missing | Portable | Secrets | Legacy |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `logs/components/<identity>/{date}_{name}.log` | component log | that executable's component logger | people, `logger_ac.exe` merges these | no file until the first gated write | no | may contain local paths | none |
| `logs/{date}_main.log` | merged daily main log | `logger_ac.exe --run` (hosted interval, or `--run --once`) | people | rebuilt from sources | no | same as the sources | none. Same deletion policy as other `.log` files. A name starting with the current local `YYYY-MM-DD_` is kept |
| `logs/merge.state` | consumed offsets | `logger_ac.exe --run` only. Hosted mode and `--run --once` are the same executable | `logger_ac.exe` | merge starts from the beginning | no | no | none |
| `data/crash_reports/.crash` | Main recovery marker | Main | next `auto_core.exe` start | no prompt | no | no | none |
| `data/crash_reports/<event>/crash.txt` and `crash.dmp` | crash report | the process that crashed, except `auto_core_shell_launcher.exe` | people | directory created on the event | no | dumps can contain process data | none. Not deleted automatically. Outside the log directory |
| `errors/errors.log` | best-effort stderr companion | `auto_core.core.error` in whichever process called it | people | created on the first report | no | may contain local detail | none |

Family logging does not gate Wake history. Crash reports are independent of `logging_policy.ini`, including `disable_all = on`.

`auto_core.exe` deletes `.log` files under the resolved log directory from a background thread. The directory and limits are the logging configuration resolved when the host started, so edits to `logging_policy.ini` apply after the host restarts. Retention uses a strict elapsed-UTC last-write cutoff, then size cleanup deletes the oldest remaining files. A filename that starts with the current local `YYYY-MM-DD_` is kept and still counts toward `max_directory_mb`. `merge.state` is not deleted.

## Dash

| Path | Purpose | Writer | Readers | Missing | Portable | Secrets |
| --- | --- | --- | --- | --- | --- | --- |
| `%LOCALAPPDATA%\Auto Core\dash.vault` | DPAPI vault | `dash_ac.exe --editor` | `dash_ac.exe --run` reads and does not mutate | retrieval fails | no | yes |

## iTunes

Data directory default `data/itunes`.

| Path | Purpose | Writer | Readers | Missing | Invalid | Portable | Secrets | Legacy |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `history.db` | listening history | `itunes_ac.exe`. `--database` reads it | none | `--seed` creates an empty file. Hosted open also creates a missing file | open failure is reported | yes, as a SQLite file | no | `user_version` 0 is stamped to 1. **Legacy.** |
| `song.format` | COM song line | `itunes_ac.exe --formatter` | `itunes_ac.exe` | compiled template in memory | compiled template, file not rewritten | yes | no | none |
| `library.format` | library row layout. Keys `format` (default `[column]`) and `column_count` (default `4`) | `itunes_ac.exe --formatter` | `itunes_ac.exe` | each missing field uses its compiled default | that field only falls back | yes | no | none |

## Spotify

Data directory default `data/spotify`.

| Path | Purpose | Writer | Readers | Missing | Invalid | Portable | Secrets | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `history.db` | play history | `spotify_ac.exe`. `--database` reads it | none | `--seed` creates an empty file. Hosted open also creates a missing file | open failure is reported | yes | no | no schema migrator |
| `song.format` | song line | `spotify_ac.exe --formatter` | `spotify_ac.exe` | compiled template | compiled template, file not replaced | yes | no | |
| `client.id` | Spotify application client id | `spotify_ac.exe --editor` | `spotify_ac.exe`, `spotify_ac.exe --oauth`, `spotify_ac.exe --config` | authorization cannot finish | blank is treated as missing | no | yes | `--seed` can write a blank file |
| `devices.list` | device id lines | `spotify_ac.exe --editor` | `spotify_ac.exe` | no stored devices | unusable lines are skipped | no | device ids | written after `ac.spotify.devices.v1` |
| `tokens.map` | `access_token`, `refresh_token`, `authorized_at`, `refresh_expires_at` | **two writers:** `spotify_ac.exe --oauth` for the initial grant, `spotify_ac.exe` on refresh | both of those, under `Local\AutoCoreSpotifyTokens` | interactive authorization required | incomplete file is not a usable session | no | yes | C9 proposed sole-writer does not hold. Disposition decision required. |

`spotify_ac.exe --config --seed` does not create `tokens.map`.

## Journal

Data directory default `data\journal`.

| Path | Purpose | Writer | Readers | Missing | Invalid | Portable | Secrets | Legacy |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `series.db` | series counters | `journal_ac.exe` allocates. `journal_ac.exe --series` edits, seeds, and initializes. SQLite locking coordinates them | both of those | `--seed` creates it and can add `Auto Core` with padding `2` only on that create. Hosted open creates a missing schema without that row | rejected or migrated | yes | no | in-place `ALTER` from user_version 1 to 2. **Legacy.** |
| `series.map` | `active = Name` plus a snapshot | `journal_ac.exe --series` | `journal_ac.exe` reads `active` and does not write | allocate uses an empty key | malformed `active` is the same | yes | no | snapshot can lag the database |
| `firebase.id` | Firebase endpoint | `journal_ac.exe --cloud` | `journal_ac.exe --cloud` | remote sync cannot run | rejected URL | no | yes, it is an account locator | |
| `extended_hour.clock` | one line `extended_hours = <token>` | `journal_ac.exe --clock` | `journal_ac.exe` | behaves as `extended_hours = 0` | same | yes | no | `--seed` writes `+0` |
| `make_print_choice.list` | aliases | `journal_ac.exe --builder` | `journal_ac.exe` | skipped at startup, not created by `journal_ac.exe` | bad lines skipped | yes | no | factory arguments accept `true`/`false` as bool shorthand. That is alias text, not an INI spelling. |
| `print_and_insert_into_journal.list` | aliases | `journal_ac.exe --builder` | `journal_ac.exe` | same | same | yes | no | same |
| `roll_dice.list` | aliases | `journal_ac.exe --builder` | `journal_ac.exe` | same | same | yes | no | same |

`journal_ac.exe --builder` interactive edit is the same owner. `--seed` does not replace an existing alias file.

## Server

Data directory default `data\server`.

| Path | Purpose | Writer | Readers | Missing | Portable | Secrets |
| --- | --- | --- | --- | --- | --- | --- |
| `port.id` | one integer, seed `8585` | `server_ac.exe --editor` | `server_ac.exe` | startup fails. No invented port | yes | no |
| `document_root.id` | one path, seed `site` | `server_ac.exe --editor` | `server_ac.exe`, `server_ac.exe --builder` | startup fails | relative yes; absolute is this machine | no |
| `<document root>/index.html` | starter page | `server_ac.exe --builder`, and only under a relative root inside the data directory | the HTTP listener | `server_ac.exe` does not create it | yes | no |
| `<document root>/styles.css` | starter stylesheet | `server_ac.exe --builder`, independently of `index.html` | the HTTP listener | same | yes | no |

An existing starter file is left unchanged. An absolute `document_root.id` is not modified by the builder.

## Taskbar

Data directory `<installation>\data\taskbar`. `keymap\activate_commands.map` is not in that directory. Policy is `config\taskbar_policy.ini` (`mode`, `logging`).

| Path | Purpose | Writer | Readers | Missing | Portable | Secrets |
| --- | --- | --- | --- | --- | --- | --- |
| `applications/*.map` | per-application sections `application`, `window`, `activation`, `commands`, `fallback`. A newly created map may include `[application] display_name`. An existing map is not rewritten | `auto_core_taskbar.exe --builder` | `auto_core.taskbar` | that application is absent from discovery configuration | only if those programs exist on the machine | no |
| `winkey_map.cache` | Win+1 through Win+10 cache. Section `[taskbar]` | `auto_core_taskbar.exe`, including `--refresh-cache` | `auto_core.taskbar` | `mode = cache` discovers live; `mode = live` replaces the cache during authority startup | no | no |
| `keymap/activate_commands.map` | shared `activate_<program>` provider selections. Path is from the installation root. Section `[activation_map]`. `# Choice:` names the one provider discovered for that command. `# Choices:` lists any other count, separated by a vertical bar | `auto_core_keymap.exe --build`, and only after every provider export succeeds | `auto_core.exe` during keymap initialization | one provider with no saved line keeps `activate_<program>` and gets no alias. The file is not created | yes | no |
| `keymap/shared_commands.map` | provider selections for ordinary duplicated commands. Section `[shared_commands]`. A new collision is blank. The alias is `<provider>_<command>`. A successful discovery removes a saved command that no configured provider still registers, including its generated choice comment, and keeps an empty section | `auto_core_keymap.exe --build`, and only after every provider export succeeds | `auto_core.exe` during keymap initialization | one provider with no saved line keeps the raw name and gets no alias. The file is not created. A failed or partial discovery does not remove a saved line | yes | no |

## Wake

Data directory default `data\wake`. Written even when family logging is off.

| Path | Purpose | Writer | Readers | Missing | Portable | Secrets |
| --- | --- | --- | --- | --- | --- | --- |
| `current.event` | latest wake record | `wake_ac.exe` | `wake_ac.exe` | `--run --snapshot` from config when any of the three is missing | no | no |
| `previous.event` | previous record | `wake_ac.exe` | `wake_ac.exe` | same | no | no |
| `wake_events.log` | appended history | `wake_ac.exe` | people | same | no | no |

An existing history file is not replaced by config. An identical capture does not append.

## Writer

Data directory default `writer`. Notes subdirectory default `notes`.

| Path | Purpose | Writer | Readers | Missing | Portable | Secrets | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `session_prompts.list` | prompt lines | `writer_ac.exe --editor` | `writer_ac.exe` opens the file | editor `--seed` / `--init` creates it. `writer_ac.exe` does not | yes | no | |
| `task_list.txt` | task lines | `writer_ac.exe --editor` | `writer_ac.exe` opens the file | same | yes | no | |
| `notes/YYYY-MM-DD.txt` | daily note | `writer_ac.exe --editor` (`--daily-note` and the editor menu) | the user, in Notepad | that command creates the day's file by appending | yes | notes may be private | `writer_ac.exe` and `writer_ac.exe --settings` launch `writer_ac.exe --editor --daily-note`. They do not write the file. |
| `text/*.txt` | text inserted by the file menu and by generated `insert_*` commands | the user | `writer_ac.exe` reads the chosen file | the directory is not created | yes | text may be private | `select_and_insert_text` lists filenames when the menu opens and pastes the file unchanged, including trailing newlines. An `insert_*` command reads its file at execution and pastes those bytes unchanged. An empty file inserts nothing. |
| `insert_text.list` | `insert_*` command catalog | `writer_ac.exe --builder` | `writer_ac.exe` | skipped at startup, not created by `writer_ac.exe` | yes | filenames only | Replaced in full. A missing or unreadable `text` directory leaves the existing file unchanged. Colliding names are omitted and reported. |
| `profile_urls.txt` | `name = url` lines. The menu shows the name and the command inserts the URL | the user | `writer_ac.exe` | the file is not created | yes | URLs may be personal | `select_and_insert_profile_url` reads the file each time the menu opens. Blank lines are skipped. A line without `=` or with an empty name or URL is logged and skipped. A later `=` stays in the URL. |

The notes directory is created by `writer_ac.exe --editor` when it resolves today's note. `writer_ac.exe` does not create `text` or `profile_urls.txt`.

## Extensions in use

**Observed**, against C19 **Proposed canonical**: `.ini`, `.list`, `.map`, `.db`, `.format`, `.id`, `.clock`, `.log`, `.cache`, `.event`, `.txt`, `.html`, `.css`, `.dmp`, `.vault`, `.state`. `.tmp` is a replace-file scratch file and does not survive a successful replace.

No production parser looks for `spotify_history.db`, `spotify_tokens.ini`, or `spotify_codes.ini`.
