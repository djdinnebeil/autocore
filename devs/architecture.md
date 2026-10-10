# Architecture inventory

Phase 1A discovery. This file records the live tree: glossary words, every production executable, the subsystem groups, and every internal protocol. Decision states are on each item. **Finalized** rows restate locks that already exist. **Proposed canonical** and **Observed** rows are not conformance rules. Nothing in this file is the frozen prerelease standard.

Test executables under `obj\*_tests\` and `scripts/stamp_auto_core_shortcut` are build tools. They are outside the taxonomy. `auto_core.dll` is the shared library. It is listed with the host because the stem `auto_core` is reserved for it and for `auto_core.exe`.

The module ledger is [modules.md](modules.md). INI keys are in [configuration.md](../docs/manual/configuration.md). Non-INI files are in [persistent-state.md](persistent-state.md).

## Glossary

**Proposed canonical**, except the sentences that restate an existing lock. Those sentences are **Finalized**. Accept or edit the new words in the vocabulary decision before they are published as normative.

The failure this glossary exists to stop is "the database component of the iTunes component." `itunes_db.exe` is a subsystem of the iTunes Component.

| Term | Use | State |
| --- | --- | --- |
| Auto Core | The platform: source tree, `auto_core.dll`, Main, and every Component. Also the installation, whose root is the parent of the directory that contains the running image (`dist/` after a normal build). | Proposed canonical. The installation-root sentence is **Finalized**. |
| Main | The host family in `app/main/`: `auto_core.exe`, init orchestrators, host config executables, and the editors of `components.list` and `keymap.map`. | Proposed canonical. |
| Core | `auto_core.dll` and `app/core/`. | Proposed canonical. |
| Component | A top-level capability enrolled in `components.list`, implemented by exactly one `<name>_ac.exe`. The ten current Components are dash, itunes, journal, logger, server, slash, spotify, taskbar, wake, and writer. | Proposed canonical for the word. Enablement by `components.list` is **Finalized**. |
| Subsystem | A specialized capability that belongs to one Component or to Main and is not itself a Component. `itunes_db.exe` is a subsystem of iTunes. | Proposed canonical. |
| Service | A long-running subsystem process whose job is to accept a private protocol, almost always via `--serve`. Current services: `itunes_db.exe`, `spotify_db.exe`, `journal_db.exe`, `journal_cloud.exe`. | Proposed canonical. `server_ac.exe` is a Component. |
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
| Configuration | INI settings, compiled defaults, and the `<name>_config.exe` owner. | Proposed canonical. **Finalized** that runtimes do not create the INI. |
| Initialization | First-run, recognized by `config/auto_core.ini`, or the `--init` mode of an owner. Init orchestrators launch owners. They do not write the files. | **Finalized**. |
| Host | Main, when it starts Components. Also a service, when it accepts `--serve`. | Proposed canonical. |
| Client | The process that connects to a protocol. Name the protocol or API. | Proposed canonical. |
| Persistent state | Any file that survives process exit. Name the owner in the same sentence. | Proposed canonical. |

## Executable taxonomy

**Proposed canonical** for the suffix chart. Member lists are **Observed** and are complete for the live `app/` tree: 61 production executables plus `auto_core.dll`. Each name appears once.

| Executable | Project | Class | Lifetime | CLI | State |
| --- | --- | --- | --- | --- | --- |
| `auto_core.exe` | `app/main/runtime` | component host | long-running | no config menu | Finalized as the host. Not a Component. |
| `auto_core.dll` | `app/core` | shared library | in-process | none | Finalized stem reservation. Not an executable. |
| `auto_core_shell_launcher.exe` | `app/core/shell_launcher` | shell trampoline | one-shot | none | Observed. Not a Component. Does not link the DLL. |
| `auto_core_init.exe` | `app/main/init/auto_core` | first-run orchestrator | one-shot | menu; launches owners | Finalized: does not write files. Removes `auto_core.ini` if a required step fails after this run created it. |
| `logger_init.exe` | `app/main/init/logger` | first-run orchestrator | one-shot | menu; `--seed`, `--init`, or `--disable` on `logging_config.exe`, and `--seed` or `--init` on `logger_config.exe` | Finalized: does not write files. |
| `components_init.exe` | `app/main/init/components` | first-run orchestrator | one-shot | menu; `--init --seed` or `--init` on each `<name>_config.exe` | Finalized: does not write files. Combined flags are Observed under C8. |
| `auto_core_config.exe` | `app/main/config/auto_core` | INI writer | one-shot | `--seed`, `--init`, menu | Finalized owner of `auto_core.ini`. |
| `auto_core_settings.exe` | `app/main/settings` | settings application | one-shot | menu | Observed. Delegates first-run to `auto_core_init.exe` and launches `auto_core_config.exe` plus discovered `<name>_settings.exe`. Writes nothing. Discovers Settings applications from `*_settings.exe`, not from `components.list` or `*_ac.exe`. |
| `Auto Core Setup.exe` | `app/main/setup` | installation bootstrap | one-shot | none | Observed. Not a Component, config owner, editor, or init orchestrator. Does not link the DLL. Creates or refreshes the two installation-root shortcuts, then launches `auto_core_settings.exe`. |
| `components_config.exe` | `app/main/config/components` | INI writer | one-shot | `--seed`, `--init`, menu | Finalized owner of `components.ini`. |
| `keymap_config.exe` | `app/main/config/keymap` | INI writer | one-shot | `--seed`, `--init`, menu | Finalized owner of `keymap.ini`. |
| `logging_config.exe` | `app/main/config/logging` | INI writer | one-shot | `--seed`, `--init`, `--disable`, menu | Finalized owner of `logging.ini`. `--disable` is an extra mode. Observed. |
| `shutdown_config.exe` | `app/main/config/shutdown` | INI writer | one-shot | `--seed`, `--init`, menu | Finalized owner of `shutdown.ini`. |
| `crash_recovery_config.exe` | `app/main/config/crash_recovery` | INI writer | one-shot | `--seed`, `--init`, menu | Finalized owner of `crash_recovery.ini`. |
| `components_editor.exe` | `app/main/editors/components` | operational editor | one-shot | menu, `--seed`, `--component` with `--on` or `--off` | Finalized sole writer of `components.list`. |
| `keymap_editor.exe` | `app/main/editors/keymap` | operational editor | one-shot | menu, `--seed` | Finalized sole writer of `keymap.map`. |
| `dash_ac.exe` | `app/components/dash/runtime` | Component, on-demand | one-shot per launch | `AC_LAUNCH_DESCRIPTOR` | Listed in `components.list`. Not a v1 child. The embedded descriptor supplies `launch_dash`, a new console, and foreground context. |
| `dash_config.exe` | `app/components/dash/config` | INI writer | one-shot | `--seed`, `--init`, menu | Finalized owner of `dash.ini`. |
| `dash_editor.exe` | `app/components/dash/editor` | operational editor | one-shot | menu | Observed owner of the DPAPI vault. |
| `dash_settings.exe` | `app/components/dash/settings` | settings application | one-shot | shared menu | Observed. Does not write the list or the INI. |
| `itunes_ac.exe` | `app/components/itunes/runtime` | Component | long-running while hosted | `ac.component.v1` | Observed hosted member. |
| `itunes_config.exe` | `app/components/itunes/config` | INI writer | one-shot | `--seed`, `--init`, menu | Finalized owner of `itunes.ini`. |
| `itunes_db.exe` | `app/components/itunes/db` | database service | `--serve` long-running; otherwise one-shot | `--serve`, `--seed`, `--owner-handle`, menu | Observed. |
| `itunes_formatter.exe` | `app/components/itunes/formatter` | format-definition owner | one-shot | `--seed`, `--init`, menu | Observed owner of `song.format` and `library.format`. |
| `itunes_settings.exe` | `app/components/itunes/settings` | settings application | one-shot | shared menu | Observed. |
| `journal_ac.exe` | `app/components/journal/runtime` | Component | long-running while hosted | `ac.component.v1` | Observed hosted member. |
| `journal_config.exe` | `app/components/journal/config` | INI writer | one-shot | `--seed`, `--init`, menu | Finalized owner of `journal.ini`. |
| `journal_db.exe` | `app/components/journal/db` | database service | `--serve` long-running; otherwise one-shot | `--serve`, `--seed`, `--init`, `--owner-handle` | Observed. Reconnect-per-call. |
| `journal_builder.exe` | `app/components/journal/builder` | recipe materializer | one-shot | `--seed`, `--init`, interactive edit | Observed. Interactive edit is a mode of this owner. |
| `journal_series.exe` | `app/components/journal/series` | domain-named store owner | one-shot | `--seed`, `--init`, menu; may launch `journal_db.exe --serve` | Observed. Not renamed to `_editor`. |
| `journal_clock.exe` | `app/components/journal/clock` | domain-named store owner | one-shot | `--seed`, `--init` | Observed owner of `extended_hour.clock`. |
| `journal_cloud.exe` | `app/components/journal/cloud` | domain-named service | `--serve` long-running; otherwise one-shot | `--serve`, `--seed`, `--init`, `--owner-handle` | Observed. |
| `journal_settings.exe` | `app/components/journal/settings` | settings application | one-shot | shared menu | Observed. |
| `logger_ac.exe` | `app/components/logger/runtime` | Component | hosted long-running; `--once` one-shot | `--once`, `--shutdown` as a label on `--once` | **Exception** candidate: same owner, extra CLI mode. Proposed until the taxonomy decision. |
| `logger_config.exe` | `app/components/logger/config` | INI writer | one-shot | `--seed`, `--init`, menu | Finalized owner of `logger.ini` only. |
| `logger_settings.exe` | `app/components/logger/settings` | settings application | one-shot | shared menu | Observed. |
| `server_ac.exe` | `app/components/server/runtime` | Component | long-running while hosted | `ac.component.v1` | Observed. HTTP server. Not a service in the glossary sense. |
| `server_config.exe` | `app/components/server/config` | INI writer | one-shot | `--seed`, `--init`, menu | Finalized owner of `server.ini`. |
| `server_editor.exe` | `app/components/server/editor` | operational editor | one-shot | `--seed`, `--init`, menu | Observed owner of `port.id` and `document_root.id`. |
| `server_builder.exe` | `app/components/server/builder` | recipe materializer | one-shot | `--seed` only | Observed. |
| `server_settings.exe` | `app/components/server/settings` | settings application | one-shot | shared menu | Observed. |
| `slash_ac.exe` | `app/components/slash/runtime` | Component, on-demand | one-shot per command | `AC_LAUNCH_DESCRIPTOR` | Listed in `components.list`. Not a v1 child. The embedded descriptor names the commands and waits until the process exits. |
| `slash_config.exe` | `app/components/slash/config` | INI writer | one-shot | `--seed`, `--init`, menu | Finalized owner of `slash.ini`. |
| `slash_settings.exe` | `app/components/slash/settings` | settings application | one-shot | shared menu | Observed. |
| `spotify_ac.exe` | `app/components/spotify/runtime` | Component | long-running while hosted | `ac.component.v1`; also the devices pipe host | Observed hosted member. |
| `spotify_config.exe` | `app/components/spotify/config` | INI writer | one-shot | `--seed`, `--init`, both, menu | Finalized owner of `spotify.ini`. Combined flags continue into authorization. See C8. |
| `spotify_db.exe` | `app/components/spotify/db` | database service | `--serve` long-running; otherwise one-shot | `--serve`, `--seed`, `--owner-handle`, menu | Observed. |
| `spotify_editor.exe` | `app/components/spotify/editor` | operational editor | one-shot | `--seed`, `--client-id`, menu | Observed. Pipe client of `ac.spotify.devices.v1`. Not a service. |
| `spotify_formatter.exe` | `app/components/spotify/formatter` | format-definition owner | one-shot | `--seed`, `--init`, menu | Observed owner of `song.format`. |
| `spotify_oauth.exe` | `app/components/spotify/oauth` | authorization helper | one-shot | OAuth flow | Observed. Initial `tokens.map` write. `spotify_ac.exe` also writes that file on refresh. |
| `spotify_settings.exe` | `app/components/spotify/settings` | settings application | one-shot | shared menu | Observed. |
| `taskbar_ac.exe` | `app/components/taskbar/runtime` | Component | hosted long-running; `--refresh-cache` one-shot | `ac.component.v1`, `--refresh-cache` | **Exception** candidate: same owner, extra CLI mode. |
| `taskbar_config.exe` | `app/components/taskbar/config` | INI writer | one-shot | `--seed`, `--init`, menu | Finalized owner of `taskbar.ini`. |
| `taskbar_builder.exe` | `app/components/taskbar/builder` | recipe materializer | one-shot | `--seed`, menu | Observed. After application maps are written, launches `taskbar_ac.exe --refresh-cache`. |
| `taskbar_settings.exe` | `app/components/taskbar/settings` | settings application | one-shot | shared menu | Observed. |
| `wake_ac.exe` | `app/components/wake/runtime` | Component | long-running while hosted; `--snapshot` one-shot | `ac.component.v1`, `--snapshot` | Observed. `--snapshot` is an extra mode used by `wake_config.exe`. |
| `wake_config.exe` | `app/components/wake/config` | INI writer | one-shot | `--seed`, `--init`, menu | Finalized owner of `wake.ini`. |
| `wake_settings.exe` | `app/components/wake/settings` | settings application | one-shot | shared menu | Observed. |
| `writer_ac.exe` | `app/components/writer/runtime` | Component | long-running while hosted | `ac.component.v1` | Observed hosted member. The daily-note command launches `writer_editor.exe --daily-note`. |
| `writer_config.exe` | `app/components/writer/config` | INI writer | one-shot | `--seed`, `--init`, menu | Finalized owner of `writer.ini`. |
| `writer_editor.exe` | `app/components/writer/editor` | operational editor | one-shot | `--seed`, `--init`, `--daily-note`, menu | Observed owner of `task_list.txt`, `session_prompts.list`, and the daily note. |
| `writer_settings.exe` | `app/components/writer/settings` | settings application | one-shot | shared menu | Observed. |

`--initialize` is a compatibility alias of `--init` in `parse_config_launch`. **Legacy** alias. It is not a second mode. Production INI parsers that use that function accept it.

No-argument `<name>_config.exe` uses `app/shared/config_menu.ixx` to list that owner's settings. The module is presentation only. The owner still defines each key, default, and validation rule, and it is still the only writer of `config/<name>.ini`. Closed-value settings are also written as a leading `# key = a | b` comment. `auto_core_config.exe` keeps its hub of the other Main configuration owners and leaves that hub with `0. Exit`. OAuth, editors, builders, and database initialization stay outside the setting menu. See [configuration.md](../docs/manual/configuration.md).

## Subsystems

These sit under Components or under Main. Grouping is **Observed**. The word "subsystem" is **Proposed canonical**.

| Group | Members |
| --- | --- |
| Host session | `auto_core.exe`, `Auto Core Setup.exe`, `auto_core_settings.exe`, `auto_core_init.exe`, `logger_init.exe`, `components_init.exe`, the six Main `*_config.exe` programs, `components_editor.exe`, `keymap_editor.exe` |
| Component Settings | every `<name>_settings.exe` on `component_settings` |
| Configuration owners | every `<name>_config.exe`, including Main |
| Operational editors | `components_editor`, `keymap_editor`, `writer_editor`, `server_editor`, `dash_editor`, `spotify_editor`, `journal_series`, `journal_builder` (also a builder), `journal_clock` |
| Database services | `itunes_db.exe`, `spotify_db.exe`, `journal_db.exe` |
| Other long-running pipe host | `journal_cloud.exe --serve`. Spotify devices is a private pipe from `spotify_editor.exe` to `spotify_ac.exe`, not a separate process. |
| One-shot utilities | `spotify_oauth.exe`, `itunes_formatter.exe`, `spotify_formatter.exe`, `server_builder.exe`, `taskbar_builder.exe`, `logger_ac.exe --once`, `wake_ac.exe --snapshot`, `taskbar_ac.exe --refresh-cache` |
| Platform substrates in the DLL | taskbar snapshot authority, pipes, process, paths, logging, INI read, clipboard, keyboard, crash diagnostics |
| Non-hosted commands | Dash and Slash, launched from `AC_LAUNCH_DESCRIPTOR` rather than hello/invoke |
| Installation trampoline | `auto_core_shell_launcher.exe` |

No production executable is in the wrong family folder. The live folders are `runtime/`, `config/`, `settings/`, `setup/`, `shared/`, plus `editor/`, `builder/`, `db/`, `formatter/`, `oauth/`, `clock/`, `cloud/`, and `series/` where that executable exists. Docs that still say component `main/` for `runtime/` are **Legacy** (C2 is Finalized).

## Internal protocols

Versioning policy for every internal wire is **Finalized** as policy: the prerelease ships one version of each protocol; a version change deletes the old id in the same change. The spellings below are **Observed** except `ac.component.v1`, whose wire ids are **Finalized**. There is no second taskbar pipe and no second keymap wire.

No protocol id parser was found that still accepts a superseded id. After hello, `ac.component.v1` does not allow unsolicited child messages. The private database and cloud pipes are request/response. The taskbar snapshot pipe is the exception: a connected client receives later generations.

### `ac.component.v1`

| Field | Value |
| --- | --- |
| State | Finalized wire ids |
| Transport | named pipe `ac_<name>_pipe`, length-prefixed frames |
| Host | the Component `<name>_ac.exe` |
| Client | Main |
| Cardinality | one Main-to-child session |
| Handshake | child sends one hello: protocol id, `termination_policy` (`graceful` or `force_allowed`), then catalog lines |
| Requests | `invoke = 0`, `shutdown = 1` |
| After hello | the child does not send unsolicited runtime messages |
| Owning module | `app/shared/protocols/component_protocol.ixx` |
| Tests | iTunes pipe tests cover hello rejection of a bad id and a bad termination policy. Main component-list tests cover enablement, not this frame layout. |

Hosted Components: itunes, journal, logger, server, spotify, taskbar, wake, writer. Dash and Slash do not speak this protocol.

### `ac.itunes.db.v1`

| Field | Value |
| --- | --- |
| State | Observed |
| Transport | `ac_itunes_db_pipe` |
| Host | `itunes_db.exe --serve` |
| Client | `itunes_ac.exe` (`itunes_db_client`) |
| Session | sticky one client |
| Requests | `observe = 1`, `shutdown = 2` |
| Parent | `itunes_ac.exe` passes `--owner-handle` |
| Owning module | `app/components/itunes/shared/itunes_db_protocol.ixx` |
| Second client | rejected by the local serve mutex and the sticky session |

### `ac.spotify.db.v1`

| Field | Value |
| --- | --- |
| State | Observed |
| Transport | `ac_spotify_db_pipe` |
| Host | `spotify_db.exe --serve` |
| Client | `spotify_ac.exe` |
| Session | sticky one client |
| Requests | `record_play = 1`, `shutdown = 2` |
| Parent | `spotify_ac.exe` |
| Owning module | `app/components/spotify/shared/db_protocol.ixx` (`export module spotify_db_protocol`) |

### `ac.journal.db.v2`

| Field | Value |
| --- | --- |
| State | Observed. The on-disk v1 `ALTER` is **Legacy** and is not this pipe id. |
| Transport | `ac_journal_db_pipe` |
| Host | `journal_db.exe --serve` |
| Clients | `journal_ac.exe` and `journal_series.exe` |
| Session | reconnect per call, sequential clients |
| Requests | `allocate_episode = 1`, `shutdown = 2`, `list_series = 3`, `add_series = 4`, `set_counter = 5`, `set_padding = 6`, `find_series = 7` |
| Parents | `journal_ac.exe` and `journal_series.exe` |
| Owning module | `app/components/journal/shared/journal_db_protocol.ixx` |
| Tests | `journal_schema_tests` lock the database file, not this frame layout |

### `ac.journal.cloud.v1`

| Field | Value |
| --- | --- |
| State | Observed |
| Transport | `ac_journal_cloud_pipe` |
| Host | `journal_cloud.exe --serve` |
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
| Client | `spotify_editor.exe` |
| Requests | `list_devices = 1` |
| Shutdown | no shutdown request on this pipe |
| Owning module | `app/components/spotify/shared/devices_protocol.ixx` |

### `AutoCore.Taskbar.v1`

| Field | Value |
| --- | --- |
| State | Observed. Does not use the `ac.<scope>.<role>.vN` spelling. **C6 Proposed canonical** would have to accept this or change it. |
| Transport | `\\.\pipe\AutoCore.Taskbar.v1`. Binary frames. Magic `ACTB`, `protocol_version` 7, `snapshot_schema_version` 7. |
| Host | `taskbar_ac.exe` snapshot authority. Mutex `Local\AutoCore.Taskbar.Authority.v1`. Cache mutex `Local\AutoCore.Taskbar.WinkeyCache.v1`. |
| Clients | processes that call `ac::taskbar::connect`. Found in Main, `journal_ac.exe`, `spotify_ac.exe`, `writer_ac.exe`, and `taskbar_ac.exe` itself. |
| Unsolicited | the authority publishes later snapshot generations to a connected listener. That push is the snapshot contract, not a Component hello. |
| Owning module | `app/core/taskbar/core_taskbar.ixx` (`auto_core.taskbar`). The pipe path and version constants are in `taskbar.cxx`. |
| Tests | none found that lock the magic or version constants |

## Command registration

Hosted components advertise commands in the `ac.component.v1` hello catalog. On-demand components list commands in the `AC_LAUNCH_DESCRIPTOR` resource inside the executable. Main registers both without compiling the component name. Name-specific protocol modules are for the component itself. They are not a Main command table. `auto_core.exe` rewrites `keymap/keymap_commands.txt` from that registry.

## External APIs

Classified only. They are platform or vendor compatibility, not Auto Core protocol versions.

| API | Where |
| --- | --- |
| iTunes COM | `itunes_ac.exe` |
| Spotify Web API | `spotify_ac.exe`, `spotify_oauth.exe` |
| Firebase HTTP | `journal_cloud.exe` |
| Loopback HTTP | `server_ac.exe` |
| DPAPI and Windows Hello | Dash vault |

## Convention index

Status is the status already assigned. This pass did not promote any Proposed canonical rule to Finalized. Verification notes:

| Id | Status | 1A note |
| --- | --- | --- |
| C1 | Proposed canonical. `_ac` is Finalized. | Taxonomy table is the member list. |
| C2 | Finalized | Live folders are `runtime/`. |
| C3 | Proposed canonical | Stem differences are in [modules.md](modules.md). `db_protocol.ixx` declares `spotify_db_protocol`. |
| C4 | Finalized for location and ownership | Section-equals-stem and `snake_case` stay proposed. |
| C5 | Finalized | No production INI parser accepts `true`/`false`. `components.ini` lowercases values before compare. |
| C6 | Proposed spelling. Finalized single-version policy. | Taskbar wire is the spelling exception. No second wire found. |
| C7 | Proposed canonical | Catalog table above. |
| C8 | Finalized for `--seed`, `--init`, init orchestrators, and `--component`. Proposed for `--serve` and the extra flags. | `--init --seed` is two flags. Seed wins, except Spotify continues. Not a third mode. |
| C9 | Finalized for INI, `components.list`, and `keymap.map`. Proposed as the general rule. | The persistent-state ledger is the check. `tokens.map` has two writers (`spotify_oauth.exe` and `spotify_ac.exe`). Daily notes have one writer (`writer_editor.exe`). |
| C10 | Finalized for INI | `new_components` invalid values become `prompt`, not the compiled default `on`. **Inconsistent.** |
| C11 | Finalized for the editor/config split | |
| C12 | Inconsistent. Proposed canonical is `configured_directory`. | Writer and Taskbar paths are still DLL exports. |
| C13 | Finalized for the component host. Proposed for services. | |
| C14 | Observed two session models. Proposed canonical: both stay legal. | iTunes db, Spotify db, and journal cloud are sticky. Journal db reconnects per call. |
| C15 | Finalized | |
| C16 | Proposed canonical | Journal v1 `ALTER` and the iTunes `user_version` 0 stamp are **Legacy**. |
| C17 | Finalized | Dash and Slash are the listed exception. |
| C18 | Finalized | |
| C19 | Proposed canonical | Extensions in use are listed in [persistent-state.md](persistent-state.md). |
| C20 | Proposed canonical | |
| C21 | Finalized where the lock record already says so. Proposed: no shared database process. | |
