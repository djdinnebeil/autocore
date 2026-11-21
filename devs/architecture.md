# Architecture inventory

Phase 1A discovery. This file records the live tree: glossary words, every production executable, the subsystem groups, and every internal protocol. Decision states are on each item. **Finalized** rows restate locks that already exist. **Proposed canonical** and **Observed** rows are not conformance rules. Nothing in this file is the frozen prerelease standard.

Test executables under `obj\*_tests\` are build tools. They are outside the taxonomy. `auto_core.dll` is the shared library. It is listed with the host because the stem `auto_core` is reserved for it and for `auto_core.exe`.

The module ledger is [modules.md](modules.md). INI keys are in [configuration.md](configuration.md). Non-INI files are in [persistent-state.md](persistent-state.md).

## Glossary

**Proposed canonical**, except the sentences that restate an existing lock. Those sentences are **Finalized**. Accept or edit the new words in the vocabulary decision before they are published as normative.

The failure this glossary exists to stop is "the database component of the iTunes component." iTunes history is a module inside `itunes_ac.exe`, not a separate component.

| Term | Use | State |
| --- | --- | --- |
| Auto Core | The platform: source tree, `auto_core.dll`, Main, and every Component. Also the installation, whose root is the parent of the directory that contains the running image (`dist/` after a normal build). | Proposed canonical. The installation-root sentence is **Finalized**. |
| Main | The host family in `app/main/`: `auto_core.exe`, init orchestrators, host config executables, and the editors of `components.list` and `keymap.map`. | Proposed canonical. |
| Core | `auto_core.dll` and `app/core/`. | Proposed canonical. |
| Component | A top-level capability enrolled in `components.list`, implemented by exactly one `<name>_ac.exe`. The ten current Components are dash, itunes, journal, logger, server, slash, spotify, taskbar, wake, and writer. | Proposed canonical for the word. Enablement by `components.list` is **Finalized**. |
| Subsystem | A specialized capability that belongs to one Component or to Main and is not itself a Component. The iTunes history store is a subsystem of iTunes. | Proposed canonical. |
| Service | A long-running subsystem process whose job is to accept a private protocol, almost always via `--serve`. Current service: `journal_ac.exe --cloud --serve`. | Proposed canonical. `server_ac.exe` is a Component. |
| Executable | A concrete `.exe`, classified with the taxonomy. | Proposed canonical. |
| Runtime | (1) the `runtime/` project that builds `<name>_ac.exe`; (2) that process; (3) behavior of a running process. | Proposed canonical. Do not mix the three in one sentence. |
| Module | A C++20/23 `export module` in a `.ixx`. | Proposed canonical. |
| Protocol | A named wire: protocol id, transport, and a closed set of messages. | Proposed canonical. |
| Command catalog | A `*_protocol.ixx` that only lists keymap command strings. Not a pipe. | Proposed canonical. |
| Owner | The sole executable allowed to create or rewrite a persistent file. | Proposed canonical as the general word. **Finalized** for INI, `components.list`, and `keymap.map`. |
| Process owner | The parent whose handle is passed as `--owner-handle`. | Proposed canonical. |
| Editor | An executable whose job is operational data that is not an INI. | Proposed canonical. |
| Builder | An executable that materializes starter or generated artifacts from a compiled recipe. | Proposed canonical. |
| Formatter | An executable that owns a human-edited format definition. The Component reads the file and formats. | Proposed canonical. |
| Configuration | INI settings, compiled defaults, and the configuration owner. A component's owner is `<name>_ac.exe --config`. Main configuration is `auto_core_settings.exe --config`. | Proposed canonical. **Finalized** that runtimes do not create the INI. |
| Initialization | First-run, recognized by `config/auto_core.ini`, or the `--init` mode of an owner. Init orchestrators launch owners. They do not write the files. | **Finalized**. |
| Host | Main, when it starts Components. Also a service, when it accepts `--serve`. | Proposed canonical. |
| Client | The process that connects to a protocol. Name the protocol or API. | Proposed canonical. |
| Persistent state | Any file that survives process exit. Name the owner in the same sentence. | Proposed canonical. |

## Executable taxonomy

**Proposed canonical** for the suffix chart. Member lists are **Observed** and are complete for the live `app/` tree. Each production executable appears once in the table below, together with `auto_core.dll`.

| Executable | Project | Class | Lifetime | CLI | State |
| --- | --- | --- | --- | --- | --- |
| `auto_core.exe` | `app/main/runtime` | component host | long-running | no config menu | Finalized as the host. Not a Component. Log cleanup runs on a background thread inside this process after ready state and at each local midnight. It uses the logging configuration resolved at startup. |
| `auto_core.dll` | `app/core` | shared library | in-process | none | Finalized stem reservation. Not an executable. |
| `auto_core_shell_launcher.exe` | `app/core/shell_launcher` | shell trampoline | one-shot | none | Observed. Not a Component. Does not link the DLL. |
| `auto_core_settings.exe` | `app/main/settings` | settings, configuration, and initialization | one-shot | no arguments opens the settings menu; `--config` is the seven-area menu or `--area <route>` with `--seed`, `--init`, and logging `--disable`; `--initialize` is first-run orchestration | The settings menu launches another `auto_core_settings.exe --config` and every discovered `<name>_ac.exe --settings`. A `<name>_settings.exe` is not a row. The menu writes nothing. `--initialize` does not write files. It seeds a missing `logging_policy.ini` with logging enabled before the Main menu. Configure asks `Enable Auto Core logging? [Y/n]:` only when this run created that file. No persists `disable_all = on` without changing other logging settings, then logs `Auto Core logging has been disabled.` and suppresses further logging in that process. It removes `auto_core.ini` if a required step fails after this run created it. Skipping components is not a failure. Log identities stay `auto_core_settings`, `auto_core_config`, and `auto_core_init`. |
| `auto_core_components.exe` | `app/main/editors/components` | component list editor and initialization orchestrator | one-shot | no arguments opens a menu for list synchronization, component initialization, or exit; `--edit` full-syncs `components.list`; `--edit --seed` creates a missing list; `--enable <name>` and `--disable <name>` set one listed state; `--initialize` prompts per component; `--initialize --seed` launches each `<name>_ac.exe --config --init --seed`; `--init` aliases `--initialize` | Sole writer of `components.list`. Initialization does not write files. Blank menu input exits without changing files. Log identities stay `components_editor` and `components_init`. |
| `auto_core_keymap.exe` | `app/main/editors/keymap` | operational editor | one-shot | no arguments print `Keymap commands`, seed a missing `keymap.map`, run `--build` once, then offer `1. Rebuild keymap` or `2. Exit`; empty input exits; a failed seed or build prints `Keymap commands could not be built.` and the menu still opens; `--seed` creates a missing `keymap.map`; `--build` publishes the command catalogs and updates the final `[commands]` section; `--help` prints usage | Sole writer of `keymap.map`. Log identity stays `keymap_editor`. Seed and build stay in-process and do not invoke each other. The no-argument workflow calls seed only when the map is missing, then build, so the build lock is acquired once. `--build` holds an exclusive byte-range lock on `<installation root>/data/auto_core/keymap_build.lock` for the whole build and exits if that lock is already held. `[commands]` stays command names only. A blank line separates the preserved map text from `[commands]`. |
| `dash_ac.exe` | `app/components/dash/runtime` | Component, settings, INI owner, and editor | `--run` is the secret menu; no arguments opens settings, or `--config --init` then settings; `--settings`; `--config`; `--editor` | `--run` keeps wide `--target`, `--parent-pid`, and `--export-keymap-commands`. Main launches `dash_ac.exe --run --target <hwnd> --parent-pid <pid>` with `CREATE_NEW_CONSOLE`. `--editor` is a separate process and keeps `Local\AutoCoreDashEditor`. `--run` keeps `Local\AutoCoreDashInstance`. Logging default stays off. Log identities stay `dash`, `dash_settings`, `dash_config`, and `dash_editor`. |
| `itunes_ac.exe` | `app/components/itunes/runtime` | Component, settings, INI owner, and formatter | `--run` is hosted or `--seed` / `--database`; no arguments opens settings, or `--config --init` then settings; `--settings`; `--config`; `--formatter` | `--run --seed` creates a missing `history.db`. `--config --seed` is the orchestrator and starts `--run --seed` and `--formatter --seed`. COM stays on the hosted path. Log identities stay `itunes`, `itunes_settings`, `itunes_config`, and `itunes_formatter`. |
| `journal_ac.exe` | `app/components/journal/runtime` | Component, settings, INI owner, builder, clock, cloud, and series | `--run` is hosted; no arguments opens settings, or `--config --init` then settings; `--settings`; `--config`; `--builder`; `--clock`; `--cloud`; `--series` | `--config` owns `config/components/journal.ini` and starts builder, cloud, clock, then series. `--cloud --serve` stays a separate process with `Local\AutoCoreJournalCloudServe`, `ac_journal_cloud_pipe`, and `--owner-handle`. `--series` opens its own `series.db`. Log identities stay `journal`, `journal_settings`, `journal_config`, `journal_builder`, `journal_clock`, `journal_cloud`, and `journal_series`. |
| `logger_ac.exe` | `app/components/logger/runtime` | Component, settings, and INI owner | `--run` is hosted or one-shot; no arguments opens settings, or `--config --init` then settings; `--settings`; `--config` | `--run` forwards `--once`, `--shutdown`, and `--export-keymap-commands`. `--config` forwards `--seed`, `--init`, and the menu. Log identities stay `logger`, `logger_settings`, and `logger_config`. |
| `server_ac.exe` | `app/components/server/runtime` | Component, settings, INI owner, editor, and builder | `--run` is hosted; no arguments opens settings, or `--config --init` then settings; `--settings`; `--config`; `--editor`; `--builder --seed` | HTTP server. Not a service in the glossary sense. `--config` owns `config/components/server.ini` and starts `server_ac.exe --editor` and `server_ac.exe --builder --seed` as separate processes. `--editor` owns `port.id` and `document_root.id`. `--builder --seed` owns the starter site. Log identities stay `server`, `server_settings`, `server_config`, `server_editor`, and `server_builder`. |
| `slash_ac.exe` | `app/components/slash/runtime` | Component, settings, and INI owner | `--run` is one command; no arguments opens settings, or `--config --init` then settings; `--settings`; `--config` | `--run` forwards one command name or `--export-keymap-commands`. A bare `--run` runs `report_and_empty_recycle_bin`. Main's descriptor keeps that command name and launches `slash_ac.exe --run report_and_empty_recycle_bin`. Log identities stay `slash`, `slash_settings`, and `slash_config`. |
| `spotify_ac.exe` | `app/components/spotify/runtime` | Component, settings, INI owner, editor, formatter, and OAuth | `--run` is hosted or `--seed` / `--database`; no arguments opens settings, or `--config --init` then settings; `--settings`; `--config`; `--editor`; `--formatter`; `--oauth` | `--run --seed` creates a missing `history.db`. `--config --seed` orchestrates database, formatter, and editor seeds, and both flags still continue into authorization. `--oauth` is a separate console. The devices pipe stays a thread inside `--run`. `Local\AutoCoreSpotifyTokens` stays cross-process. Log identities stay `spotify`, `spotify_settings`, `spotify_config`, `spotify_editor`, `spotify_formatter`, and `spotify_oauth`. |
| `auto_core_taskbar.exe` | `app/main/taskbar` | Main subsystem, settings, policy owner, and builder | `--run` is hosted, `--refresh-cache`, or `--inspect-snapshot`; no arguments set the app user model id, run `--builder` once, then open settings, or `--config --init` then that discovery and settings when init exits 0; `--settings` opens the menu only; `--config`; `--builder` | `--run --refresh-cache` uses the cache mutex only. It does not take the authority mutex or start the control pipe. A no-argument launch prints `Discovering taskbar applications`, then uses the same `--builder` process as menu option 2. Exit 0 prints `Taskbar application discovery complete.` A failed discovery keeps the existing alerts and still opens the menu. A failed `--config --init` returns that exit code without discovery or the menu. `--builder` writes missing maps, then starts a separate `auto_core_taskbar.exe --run --refresh-cache`. `--config` owns `config/taskbar_policy.ini`. Data is `<installation>/data/taskbar`. Log identities stay `taskbar`, `taskbar_settings`, `taskbar_config`, and `taskbar_builder`. |
| `wake_ac.exe` | `app/components/wake/runtime` | Component, settings, and INI owner | `--run` is hosted or `--snapshot`; no arguments opens settings, or `--config --init` then settings; `--settings`; `--config` | `--run` forwards `--snapshot` and `--export-keymap-commands`. `--config` still starts a separate `wake_ac.exe --run --snapshot` when history files are missing. Log identities stay `wake`, `wake_settings`, and `wake_config`. |
| `writer_ac.exe` | `app/components/writer/runtime` | Component, settings, INI owner, editor, and builder | `--run` is hosted; no arguments opens settings, or `--config --init` then settings; `--settings`; `--config`; `--editor`; `--builder` | The daily-note command starts a separate `writer_ac.exe --editor --daily-note` and waits. Notepad still goes through `auto_core_shell_launcher.exe`. `--editor` owns `task_list.txt`, `session_prompts.list`, and the daily note. `--builder` owns `insert_text.list`. `--config` owns `config/components/writer.ini`. Log identities stay `writer`, `writer_settings`, `writer_config`, `writer_editor`, and `writer_builder`. |
`--initialize` is a compatibility alias of `--init` in `parse_config_launch`. **Legacy** alias. It is not a second mode. Production INI parsers that use that function accept it.

A no-argument component configuration menu uses `app/shared/config_menu.ixx` to list that owner's settings. That menu is `<name>_ac.exe --config`. The module is presentation only. The owner still defines each key, default, and validation rule, and it is still the only writer of `config/components/<name>.ini`. Closed-value settings are also written as a leading `# key = a | b` comment. `auto_core_settings.exe --config` keeps its hub of the Main configuration areas and leaves that hub with `0. Exit`. OAuth, editors, builders, and database initialization stay outside the setting menu. Ownership, launch flags, and Main keys are in [configuration.md](configuration.md).

## Subsystems

These sit under Components or under Main. Grouping is **Observed**. The word "subsystem" is **Proposed canonical**.

| Group | Members |
| --- | --- |
| Host session | `auto_core.exe`, `auto_core_settings.exe`, `auto_core_components.exe`, `auto_core_keymap.exe`, `auto_core_taskbar.exe` |
| Component Settings | `<name>_ac.exe --settings` on `component_settings` |
| Configuration owners | `<name>_ac.exe --config` for a component. Main configuration routes are `auto_core_settings.exe --config --area <route>` |
| Operational editors | `auto_core_components.exe`, `auto_core_keymap.exe`, `writer_editor`, `server_editor`, `dash_ac.exe --editor`, `spotify_ac.exe --editor`, `journal_ac.exe --series`, `journal_ac.exe --builder`, `journal_ac.exe --clock` |
| Other long-running pipe host | `journal_ac.exe --cloud --serve`. Spotify devices is a private pipe from `spotify_ac.exe --editor` to `spotify_ac.exe`, not a separate process. Component databases are in-process SQLite modules, not services. |
| One-shot utilities | `spotify_ac.exe --oauth`, `itunes_ac.exe --formatter`, `spotify_ac.exe --formatter`, `server_ac.exe --builder --seed`, `auto_core_taskbar.exe --builder`, `writer_ac.exe --builder`, `logger_ac.exe --run --once`, `wake_ac.exe --run --snapshot`, `auto_core_taskbar.exe --run --refresh-cache` |
| Platform substrates in the DLL | taskbar snapshot authority, pipes, process, paths, logging, INI read, clipboard, keyboard, crash diagnostics |
| Non-hosted commands | Dash and Slash, launched from `AC_LAUNCH_DESCRIPTOR` rather than hello/invoke |
| Installation trampoline | `auto_core_shell_launcher.exe` |

No production executable is in the wrong family folder. The live folders are `runtime/`, `config/`, `settings/`, `setup/`, `shared/`, plus `editor/`, `builder/`, `db/`, `formatter/`, `oauth/`, `clock/`, `cloud/`, and `series/` where that executable or module exists. `db/` holds the SQLite module for Journal, iTunes, and Spotify. It is not an executable directory. Docs that still say component `main/` for `runtime/` are **Legacy** (C2 is Finalized).

## Internal protocols

Versioning policy for every internal wire is **Finalized** as policy: the prerelease ships one version of each protocol; a version change deletes the old id in the same change. The spellings below are **Observed** except `ac.component.v2`, whose wire ids are **Finalized**. There is no second taskbar pipe and no second keymap wire.

No protocol id parser was found that still accepts a superseded id. After hello, `ac.component.v2` does not allow unsolicited child messages. The private cloud pipe is request/response. Journal, iTunes, and Spotify history calls are direct method calls on one persistent SQLite connection in the runtime. The taskbar snapshot pipe is the exception: a connected client receives later generations.

### `ac.component.v2`

| Field | Value |
| --- | --- |
| State | Finalized wire ids |
| Transport | named pipe `ac_<name>_pipe`, length-prefixed frames |
| Host | the Component `<name>_ac.exe` |
| Client | Main |
| Cardinality | one Main-to-child session |
| Handshake | child sends one hello: protocol id, `termination_policy` (`graceful` or `force_allowed`), then catalog lines. A line is a command name or `name(...)`, optionally followed by ` - description` after that complete identity |
| Requests | `invoke = 0`, `shutdown = 1` |
| After hello | the child does not send unsolicited runtime messages |
| Owning module | `app/shared/protocols/component_protocol.ixx` |
| Tests | iTunes pipe tests cover hello rejection of a bad id and a bad termination policy. Main component-list tests cover enablement, not this frame layout. |

Hosted Components: itunes, journal, logger, server, spotify, taskbar, wake, writer. Dash and Slash do not speak this protocol.

### Component databases

Journal, iTunes, and Spotify each keep SQL in `db/*_sqlite`. The runtime opens one connection for its lifetime and calls the store directly. A process mutex covers every call when more than one thread uses that connection. Callers are joined before the final write and `sqlite3_close`. There is no database pipe.

`journal_ac.exe --series` opens a second connection for its own lifetime. Overlapping writes use SQLite's lock and the existing 3-second busy timeout. Journal read-modify-write work uses `BEGIN IMMEDIATE` before reading the values it updates. The on-disk Journal `user_version` 1 `ALTER` remains **Legacy** and is not a protocol id. `journal_schema_tests` lock the database file.

### `ac.journal.cloud.v1`

| Field | Value |
| --- | --- |
| State | Observed |
| Transport | `ac_journal_cloud_pipe` |
| Host | `journal_ac.exe --cloud --serve` |
| Client | `journal_ac.exe` |
| Session | sticky |
| Requests | `push = 1`, `shutdown = 2` |
| Parent | `journal_ac.exe` |
| Owning module | `app/components/journal/shared/journal_cloud_protocol.ixx` |

### `ac.spotify.devices.v1`

| Field | Value |
| --- | --- |
| State | Observed |
| Transport | `ac_spotify_devices_pipe` |
| Host | `spotify_ac.exe` (the Component, not a service process) |
| Client | `spotify_ac.exe --editor` |
| Requests | `list_devices = 1` |
| Shutdown | no shutdown request on this pipe |
| Owning module | `app/components/spotify/shared/devices_protocol.ixx` |

### `AutoCore.Taskbar.v1`

| Field | Value |
| --- | --- |
| State | Observed. Does not use the `ac.<scope>.<role>.vN` spelling. **C6 Proposed canonical** would have to accept this or change it. |
| Transport | `\\.\pipe\AutoCore.Taskbar.v1`. Binary frames. Magic `ACTB`, `protocol_version` 7, `snapshot_schema_version` 7. |
| Host | `auto_core_taskbar.exe` snapshot authority. Mutex `Local\AutoCore.Taskbar.Authority.v1`. Cache mutex `Local\AutoCore.Taskbar.WinkeyCache.v1`. |
| Clients | processes that call `ac::taskbar::connect`. Found in Main, `journal_ac.exe`, `spotify_ac.exe`, `writer_ac.exe`, and `auto_core_taskbar.exe` itself. |
| Unsolicited | the authority publishes later snapshot generations to a connected listener. That push is the snapshot contract, not a Component hello. |
| Owning module | `app/core/taskbar/core_taskbar.ixx` (`auto_core.taskbar`). The pipe path and version constants are in `taskbar.cxx`. |
| Tests | none found that lock the magic or version constants |

## Command registration

Hosted components advertise commands in the `ac.component.v2` hello catalog. On-demand components list commands in the `AC_LAUNCH_DESCRIPTOR` resource inside the executable. Main registers both without compiling the component name. Name-specific protocol modules are for the component itself. They are not a Main command table. `auto_core.exe` rewrites `keymap/keymap_commands.txt` from that registry.

## External APIs

Classified only. They are platform or vendor compatibility, not Auto Core protocol versions.

| API | Where |
| --- | --- |
| iTunes COM | `itunes_ac.exe` |
| Spotify Web API | `spotify_ac.exe`, `spotify_ac.exe --oauth` |
| Firebase HTTP | `journal_ac.exe --cloud` |
| Loopback HTTP | `server_ac.exe` |
| DPAPI and Windows Hello | Dash vault |

## Convention index

Status is the status already assigned. This pass did not promote any Proposed canonical rule to Finalized. Verification notes:

| Id | Status | 1A note |
| --- | --- | --- |
| C1 | Proposed canonical. `_ac` is Finalized. | Taxonomy table is the member list. |
| C2 | Finalized | Live folders are `runtime/`. |
| C3 | Proposed canonical | Stem differences are in [modules.md](modules.md). |
| C4 | Finalized for location and ownership | Section-equals-stem and `snake_case` stay proposed. |
| C5 | Finalized | No production INI parser accepts `true`/`false`. `components_policy.ini` lowercases values before compare. |
| C6 | Proposed spelling. Finalized single-version policy. | Taskbar wire is the spelling exception. No second wire found. |
| C7 | Proposed canonical | Catalog table above. |
| C8 | Finalized for `--seed`, `--init`, init orchestrators, and `--component`. Proposed for `--serve` and the extra flags. | `--init --seed` is two flags. Seed wins, except Spotify continues. Not a third mode. |
| C9 | Finalized for INI, `components.list`, and `keymap.map`. Proposed as the general rule. | The persistent-state ledger is the check. `tokens.map` has two writers (`spotify_ac.exe --oauth` and `spotify_ac.exe`). Daily notes have one writer (`writer_ac.exe --editor`). |
| C10 | Finalized for INI | `new_components` invalid values become `prompt`, not the compiled default `on`. **Inconsistent.** |
| C11 | Finalized for the editor/config split | |
| C12 | Inconsistent. Proposed canonical is `configured_directory`. | Writer and Taskbar paths are still DLL exports. |
| C13 | Finalized for the component host. Proposed for services. | |
| C14 | Observed two session models. Proposed canonical: both stay legal. | `journal_ac.exe --cloud` is sticky. Component databases are in-process and have no session. |
| C15 | Finalized | |
| C16 | Proposed canonical | Journal v1 `ALTER` and the iTunes `user_version` 0 stamp are **Legacy**. |
| C17 | Finalized | Dash and Slash are the listed exception. |
| C18 | Finalized | |
| C19 | Proposed canonical | Extensions in use are listed in [persistent-state.md](persistent-state.md). |
| C20 | Proposed canonical | |
| C21 | Finalized where the lock record already says so. Proposed: no shared database process. | |
