# Persistent state

Phase 1A discovery of every file that survives process exit. INI keys are in [configuration.md](configuration.md). This ledger is what the general sole-writer rule (C9, **Proposed canonical**) has to match. It is not that rule made normative.

Paths are relative to the installation root (`dist/` after a normal build) unless the row says otherwise. A Component data directory comes from that Component's `directory` key. Compiled defaults are in the configuration ledger.

Shared missing-file behavior for non-INI files is **Proposed canonical** and not yet a conformance rule: the owner's `--seed` creates a missing file, and readers use a documented in-memory default or fail the operation. Rows below record what the code does now (**Observed**).

Logs and crash reports are written by whichever process produced them. That is many writers of one directory pattern, and one writer of each resulting file. It does not fit "one executable owns the pattern" without an exception. Disposition of that exception: decision required.

## Installation root

| Path | Purpose | Sole writer | Readers | Created | Missing | Portable | Secrets | First-run | Legacy | Disposition |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `config/*.ini` | settings | the matching `_config.exe` | the family that names the file | `--seed` or `--init` when missing | compiled defaults, no create | yes, if paths stay relative | no | yes, through the orchestrators | unknown keys drop out on rewrite. No `true`/`false` spellings in production parsers | keep. See the INI ledger. |
| `components.list` | enablement | `components_editor.exe` | Main, every `<name>_star.exe` | `--seed`, no-arg rebuild, or `--component` | runtime enables every valid `*_ac.exe` and does not create the file | yes | no | `components_editor.exe --seed` | none. Lines are `name`, `name on`, or `name off` | keep. Finalized. |
| `keymap.map` | key bindings | `keymap_editor.exe` | `auto_core.exe` | `--seed` when missing | in-memory emergency map, no create | yes | no | `keymap_editor.exe` during first-run | `parse_legacy_line` accepts a brace form and has no production caller | keep the file. Brace parser is legacy. |
| `keymap/keymap_commands.txt` | autocomplete catalog | `auto_core.exe` during keymap initialization | keymap editor consumers | rewritten when bytes differ | editor autocomplete lacks the generated list | yes | no | written on the next successful keymap init | the two exported getters have no caller; the file write does | keep. Disposition of the unused getters is pending on the module row. |
| `keymap/components/itunes.keymap_commands.txt` | unused build list | MSBuild `WriteLinesToFile` | none | build | none; hello is the command authority | yes | no | build output | none | leftover. Not read at runtime. |
| `keymap/components/journal.keymap_commands.txt` | unused build list | `journal_ac.exe` at build | none | build | none; hello is the command authority | yes | no | build output | none | leftover. Not read at runtime. |
| `keymap/components/spotify.keymap_commands.txt` | unused build list | MSBuild `WriteLinesToFile` | none | build | none; hello is the command authority | yes | no | build output | none | leftover. Not read at runtime. |
| `keymap/components/taskbar.keymap_commands.txt` | unused builder list | `taskbar_builder.exe` | none | `--seed` if missing; `--refresh-manifest` replaces | none; hello and Main registration are the authority | yes | no | `taskbar_config.exe` may launch the builder | none | leftover. Not read at runtime. |
| `keymap/components/writer.keymap_commands.txt` | unused build list | `writer_ac.exe` at build | none | build | none; hello is the command authority | yes | no | build output | none | leftover. Not read at runtime. |

Logger, Server, and Wake do not generate a `keymap/components` catalog.

## Logs, crashes, and errors

| Path | Purpose | Writer | Readers | Missing | Portable | Secrets | Legacy |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `logs/components/<identity>/{date}_{name}.log` | comprehensive log | that executable's component logger | people, `logger_ac.exe` does not merge this file | no file until the first gated write | no | may contain local paths | none |
| `logs/components/<identity>/{date}_{name}.main.log` | main-log subset | same executable | `logger_ac.exe` merges these | same | no | may contain local paths | none |
| `logs/{date}_main.log` | merged daily main log | `logger_ac.exe` (hosted interval, or `--once`) | people | rebuilt from sources | no | same as the sources | none |
| `logs/merge.state` | consumed offsets | `logger_ac.exe` only. Hosted mode and `--once` are the same executable | `logger_ac.exe` | merge starts from the beginning | no | no | none |
| `crash/.crash` | Main recovery marker | Main | next `auto_core.exe` start | no prompt | no | no | none |
| `crash/<event>/crash.txt` and `crash.dmp` | crash report | the process that crashed, except `auto_core_shell_launcher.exe` | people | directory created on the event | no | dumps can contain process data | none. Not deleted automatically. |
| `errors/errors.log` | best-effort stderr companion | `auto_core.core.error` in whichever process called it | people | created on the first report | no | may contain local detail | none |

Family logging does not gate Wake history. Crash reports are independent of `logging.ini`, including `disable_all = on`.

## Dash

| Path | Purpose | Writer | Readers | Missing | Portable | Secrets |
| --- | --- | --- | --- | --- | --- | --- |
| `%LOCALAPPDATA%\Auto Core\dash.vault` | DPAPI vault | `dash_editor.exe` | `dash_ac.exe` reads and does not mutate | retrieval fails | no | yes |

## iTunes

Data directory default `components/itunes`.

| Path | Purpose | Writer | Readers | Missing | Invalid | Portable | Secrets | Legacy |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `history.db` | listening history | `itunes_db.exe` only | none. `itunes_ac.exe` uses the pipe | `--seed` creates an empty file. Runtime does not | open failure is reported | yes, as a SQLite file | no | `user_version` 0 is stamped to 1. **Legacy.** |
| `song.format` | COM song line | `itunes_formatter.exe` | `itunes_ac.exe` | compiled template in memory | compiled template, file not rewritten | yes | no | none |
| `library.format` | library row layout. Keys `format` (default `[column]`) and `column_count` (default `4`) | `itunes_formatter.exe` | `itunes_ac.exe` | each missing field uses its compiled default | that field only falls back | yes | no | none |

## Spotify

Data directory default `components/spotify`.

| Path | Purpose | Writer | Readers | Missing | Invalid | Portable | Secrets | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `history.db` | play history | `spotify_db.exe` only | none. `spotify_ac.exe` uses the pipe | `--seed` creates an empty file | open failure is reported | yes | no | no schema migrator |
| `song.format` | song line | `spotify_formatter.exe` | `spotify_ac.exe` | compiled template | compiled template, file not replaced | yes | no | |
| `client.id` | Spotify application client id | `spotify_editor.exe` | `spotify_ac.exe`, `spotify_oauth.exe`, `spotify_config.exe` | authorization cannot finish | blank is treated as missing | no | yes | `--seed` can write a blank file |
| `devices.list` | device id lines | `spotify_editor.exe` | `spotify_ac.exe` | no stored devices | unusable lines are skipped | no | device ids | written after `ac.spotify.devices.v1` |
| `tokens.map` | `access_token`, `refresh_token`, `authorized_at`, `refresh_expires_at` | **two writers:** `spotify_oauth.exe` for the initial grant, `spotify_ac.exe` on refresh | both of those, under `Local\AutoCoreSpotifyTokens` | interactive authorization required | incomplete file is not a usable session | no | yes | C9 proposed sole-writer does not hold. Disposition decision required. |

`spotify_config.exe --seed` does not create `tokens.map`.

## Journal

Data directory default `components\journal`.

| Path | Purpose | Writer | Readers | Missing | Invalid | Portable | Secrets | Legacy |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `series.db` | series counters | `journal_db.exe` only | none. Clients use `ac.journal.db.v2` | `--seed` creates it and can add `Auto Core` with padding `2` only on that create | rejected or migrated | yes | no | in-place `ALTER` from user_version 1 to 2. **Legacy.** |
| `series.map` | `active = Name` plus a snapshot | `journal_series.exe` | `journal_ac.exe` reads `active` and does not write | allocate uses an empty key | malformed `active` is the same | yes | no | snapshot can lag the database |
| `firebase.id` | Firebase endpoint | `journal_cloud.exe` | `journal_cloud.exe` | remote sync cannot run | rejected URL | no | yes, it is an account locator | |
| `extended_hour.clock` | one line `extended_hours = <token>` | `journal_clock.exe` | `journal_ac.exe` | behaves as `extended_hours = 0` | same | yes | no | `--seed` writes `+0` |
| `make_print_choice.list` | aliases | `journal_builder.exe` | `journal_ac.exe` | skipped at startup, not created by `journal_ac.exe` | bad lines skipped | yes | no | factory arguments accept `true`/`false` as bool shorthand. That is alias text, not an INI spelling. |
| `print_and_insert_into_journal.list` | aliases | `journal_builder.exe` | `journal_ac.exe` | same | same | yes | no | same |
| `roll_dice.list` | aliases | `journal_builder.exe` | `journal_ac.exe` | same | same | yes | no | same |

`journal_builder.exe` interactive edit is the same owner. `--seed` does not replace an existing alias file.

## Server

Data directory default `components\server`.

| Path | Purpose | Writer | Readers | Missing | Portable | Secrets |
| --- | --- | --- | --- | --- | --- | --- |
| `port.id` | one integer, seed `8585` | `server_editor.exe` | `server_ac.exe` | startup fails. No invented port | yes | no |
| `document_root.id` | one path, seed `site` | `server_editor.exe` | `server_ac.exe`, `server_builder.exe` | startup fails | relative yes; absolute is this machine | no |
| `<document root>/index.html` | starter page | `server_builder.exe`, and only under a relative root inside the data directory | the HTTP listener | `server_ac.exe` does not create it | yes | no |
| `<document root>/styles.css` | starter stylesheet | `server_builder.exe`, independently of `index.html` | the HTTP listener | same | yes | no |

An existing starter file is left unchanged. An absolute `document_root.id` is not modified by the builder.

## Taskbar

Data directory default `taskbar`.

| Path | Purpose | Writer | Readers | Missing | Portable | Secrets |
| --- | --- | --- | --- | --- | --- | --- |
| `applications/*.map` | per-application sections `application`, `window`, `activation`, `commands`, `fallback` | `taskbar_builder.exe` | `auto_core.taskbar` | that application is absent from discovery configuration | only if those programs exist on the machine | no |
| `winkey_map.cache` | Win+1 through Win+10 cache. Section `[taskbar]` | `taskbar_ac.exe`, including `--refresh-cache` | `auto_core.taskbar` | `mode = cache` discovers live; `mode = live` replaces the cache during authority startup | no | no |

## Wake

Data directory default `components\wake`. Written even when family logging is off.

| Path | Purpose | Writer | Readers | Missing | Portable | Secrets |
| --- | --- | --- | --- | --- | --- | --- |
| `current.event` | latest wake record | `wake_ac.exe` | `wake_ac.exe` | `--snapshot` from config when any of the three is missing | no | no |
| `previous.event` | previous record | `wake_ac.exe` | `wake_ac.exe` | same | no | no |
| `wake_events.log` | appended history | `wake_ac.exe` | people | same | no | no |

An existing history file is not replaced by config. An identical capture does not append.

## Writer

Data directory default `writer`. Notes subdirectory default `notes`.

| Path | Purpose | Writer | Readers | Missing | Portable | Secrets | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `session_prompts.list` | prompt lines | `writer_editor.exe` | `writer_ac.exe` opens the file | editor `--seed` / `--init` creates it. `writer_ac.exe` does not | yes | no | |
| `task_list.txt` | task lines | `writer_editor.exe` | `writer_ac.exe` opens the file | same | yes | no | |
| `notes/YYYY-MM-DD.txt` | daily note | `writer_editor.exe` (`--daily-note` and the editor menu) | the user, in Notepad | that command creates the day's file by appending | yes | notes may be private | `writer_ac.exe` and `writer_star.exe` launch `writer_editor.exe --daily-note`. They do not write the file. |

The notes directory is created by `writer_editor.exe` when it resolves today's note.

## Extensions in use

**Observed**, against C19 **Proposed canonical**: `.ini`, `.list`, `.map`, `.db`, `.format`, `.id`, `.clock`, `.log`, `.cache`, `.event`, `.txt`, `.html`, `.css`, `.dmp`, `.vault`, `.state`. `.tmp` is a replace-file scratch file and does not survive a successful replace.

No production parser looks for `spotify_history.db`, `spotify_tokens.ini`, or `spotify_codes.ini`.
