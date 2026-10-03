# Module catalog

C++23 `export module` units for Auto Core. Public contracts are Doxygen-style
comments on the `.ixx` exports; read them in the IDE or at the source.

Implementation units (`.cxx` with `module Name;`) are omitted unless they are
the only source for a module.

## Core DLL (`app/core`)

| Module | Source | Role |
| --- | --- | --- |
| `auto_core.core.clipboard` | [`clipboard.ixx`](../app/core/modules/clipboard.ixx) | Unicode-only clipboard snapshot, restore, and paste |
| `auto_core.core.clock` | [`clock.ixx`](../app/core/modules/clock.ixx) | Local date and time strings, including extended-day timestamps |
| `auto_core.core.component` | [`component.ixx`](../app/core/modules/component.ixx) | Per-component logging, console output, text insertion, and `report_ini_unavailable` |
| `auto_core.core.component:logger` | [`component_logger.ixx`](../app/core/modules/component_logger.ixx) | Internal local-file logger partition |
| `auto_core.core.component:console_writer` | [`console_writer.ixx`](../app/core/modules/console_writer.ixx) | Internal stdout/stderr writer partition |
| `auto_core.core.component:text_inserter` | [`text_inserter.ixx`](../app/core/modules/text_inserter.ixx) | Internal clipboard insertion partition |
| `auto_core.core.console` | [`console.ixx`](../app/core/modules/console.ixx) | Console and window activation for prompts |
| `auto_core.core.config` | [`core_config.ixx`](../app/core/modules/core_config.ixx) | Cached `auto_core.ini` `[auto_core] warn_without_winkey_mapping` |
| `auto_core.core.encoding` | [`encoding.ixx`](../app/core/modules/encoding.ixx) | Strict UTF-8 / UTF-16 conversion |
| `auto_core.core.error` | [`error.ixx`](../app/core/modules/error.ixx) | Best-effort stderr and `errors/errors.log` reporting |
| `auto_core.core.formatting` | [`formatting.ixx`](../app/core/modules/formatting.ixx) | UTF-8 `std::format` with wide-text normalization |
| `auto_core.core.ini` | [`ini.ixx`](../app/core/modules/ini.ixx) | Sectioned INI parse and file read |
| `auto_core.core.keyboard` | [`keyboard.ixx`](../app/core/modules/keyboard.ixx) | `SendInput` helpers, including `VK_RWIN`+position |
| `auto_core.core.logging.config` | [`logging_config.ixx`](../app/core/modules/logging_config.ixx) | Cached `logging.ini` policy (`disable_all`, both sinks, `log_print_mode`, `component_logging_default`) and `logger.ini` merge timing |
| `auto_core.core.paths` | [`paths.ixx`](../app/core/modules/paths.ixx) | Process-lifetime `bin_directory` and `installation_root` |
| `auto_core.core.pipes` | [`pipes.ixx`](../app/core/modules/pipes.ixx) | Named-pipe handles, string frames, and command dispatch |
| `auto_core.core.shell` | [`shell.ixx`](../app/core/modules/shell.ixx) | Assigns process AppUserModelID `Djdinn.AutoCore` |
| `auto_core.core.thread` | [`thread.ixx`](../app/core/modules/thread.ixx) | Thread-entry exception reporting |
| `auto_core.taskbar` | [`core_taskbar.ixx`](../app/core/taskbar/core_taskbar.ixx) | Snapshot authority, client lookup, and native Win+position |

## Main executable (`app/main/runtime`)

Process lifetime for `auto_core.exe` is [main.md](main.md).

| Module | Source | Role |
| --- | --- | --- |
| `auto_core.main.application` | [`main_component.ixx`](../app/main/runtime/modules/main_component.ixx) | Main `Component`, process launch, F-lock, shutdown |
| `auto_core.main.components` | [`ac_components.ixx`](../app/main/runtime/modules/ac_components.ixx) | Generic host: open `components.ini` `[components]`, hello, invoke, shutdown |
| `auto_core.main.components.dash` | [`dash_component.ixx`](../app/main/runtime/modules/dash_component.ixx) | Launches `dash_ac.exe` |
| `auto_core.main.components.slash` | [`slash_component.ixx`](../app/main/runtime/modules/slash_component.ixx) | Launches Slash recycle-bin commands |
| `auto_core.main.components.taskbar` | [`taskbar_component.ixx`](../app/main/runtime/modules/taskbar_component.ixx) | Main-local taskbar launches and reserved control names |
| `auto_core.main.crash_recovery` | [`crash_recovery.ixx`](../app/main/runtime/modules/crash_recovery.ixx) | Previous-crash dialog and restart handler |
| `auto_core.main.key_codes` | [`key_codes.ixx`](../app/main/runtime/modules/key_codes.ixx) | Normalized virtual-key codes and `mappings.ini` names (`keys`, `resolve`) |
| `auto_core.main.keyboard_input` | [`keyboard_input.ixx`](../app/main/runtime/modules/keyboard_input.ixx) | Low-level hook and main-thread dispatch |
| `auto_core.main.keymap` | [`keymap.ixx`](../app/main/runtime/modules/keymap.ixx) | Active primary/secondary key bindings |
| `auto_core.main.keymap.runtime` | [`keymap_runtime.ixx`](../app/main/runtime/modules/keymap_runtime.ixx) | File keymap, command registry, workspace files |
| `auto_core.main.logging` | [`logging_init.ixx`](../app/main/runtime/modules/logging_init.ixx) | Creates the log directory and writes Main's local session records |
| `auto_core.main.program_ready` | [`program_ready.ixx`](../app/main/runtime/modules/program_ready.ixx) | Startup status output |
| `auto_core.main.shutdown_events` | [`shutdown_events.ixx`](../app/main/runtime/modules/shutdown_events.ixx) | Console and session shutdown |
| `auto_core.main.taskbar` | [`main_taskbar.ixx`](../app/main/runtime/modules/main_taskbar.ixx) | Interactive cycling session owned by Main |
| `auto_core.main.test_commands` | [`test_commands.ixx`](../app/main/runtime/modules/test_commands.ixx) | Diagnostic keymap commands |

## Shared (`app/shared` plus per-child `shared/`)

Generic host contracts stay here. Name-specific protocols move to
`app/components/<name>/shared/` as each child is nested.

| Module | Source | Role |
| --- | --- | --- |
| `command_registry` | [`command_registry.ixx`](../app/shared/command_registry.ixx) | Name and factory lookup for runtime commands |
| `component_protocol` | [`component_protocol.ixx`](../app/shared/protocols/component_protocol.ixx) | Generic `ac.component.v1` hello, invoke, and shutdown |
| `itunes_protocol` | [`itunes_protocol.ixx`](../app/components/itunes/shared/itunes_protocol.ixx) | iTunes command names used by `itunes_ac.exe` |
| `journal_protocol` | [`journal_protocol.ixx`](../app/components/journal/shared/journal_protocol.ixx) | Journal command tokens used by `journal_ac.exe` |
| `slash_protocol` | [`slash_protocol.ixx`](../app/components/slash/shared/slash_protocol.ixx) | Slash keymap command names |
| `spotify_protocol` | [`spotify_protocol.ixx`](../app/components/spotify/shared/spotify_protocol.ixx) | Spotify command names used by `spotify_ac.exe` |
| `taskbar_protocol` | [`taskbar_protocol.ixx`](../app/components/taskbar/shared/taskbar_protocol.ixx) | Reserved Main-local taskbar command names |
| `writer_protocol` | [`writer_protocol.ixx`](../app/components/writer/shared/writer_protocol.ixx) | Writer command names used by `writer_ac.exe` |

## Components (`app/components`)

| Module | Source | Role |
| --- | --- | --- |
| `itunes_client` | [`itunes_client.ixx`](../app/components/itunes/main/itunes_client.ixx) | COM client and shared iTunes process state |
| `itunes_db_protocol` | [`itunes_db_protocol.ixx`](../app/components/itunes/shared/itunes_db_protocol.ixx) | Private `ac.itunes.db.v1` pipe between `itunes_ac.exe` and `itunes_db.exe` |
| `itunes_db_client` | [`itunes_db_client.ixx`](../app/components/itunes/main/itunes_db_client.ixx) | `itunes_ac.exe` client for `itunes_db.exe` |
| `itunes_sqlite` | [`itunes_sqlite.ixx`](../app/components/itunes/db/itunes_sqlite.ixx) | `history.db` schema and listening rows, compiled only into `itunes_db.exe` |
| `itunes_defaults` | [`defaults.ixx`](../app/components/itunes/shared/defaults.ixx) | Compiled `itunes.ini` text and the default song template. Parsing and the verified token catalog are the headers [`song_template_detail.hpp`](../app/components/itunes/shared/song_template_detail.hpp) and [`itunes_metadata_detail.hpp`](../app/components/itunes/shared/itunes_metadata_detail.hpp). |
| `itunes_component` | [`itunes_component.ixx`](../app/components/itunes/main/itunes_component.ixx) | `itunes_ac.exe` Component session |
| `itunes_monitor` | [`itunes_monitor.ixx`](../app/components/itunes/main/itunes_monitor.ixx) | Track-change monitor |
| `itunes_pipe` | [`itunes_pipe.ixx`](../app/components/itunes/main/itunes_pipe.ixx) | Pipe command registration |
| `itunes_registry` | [`itunes_registry.ixx`](../app/components/itunes/main/itunes_registry.ixx) | iTunes runtime command registry |
| `itunes_removal` | [`itunes_removal.ixx`](../app/components/itunes/main/itunes_removal.ixx) | Recycle-bin track removal |
| `itunes_runtime` | [`itunes_runtime.ixx`](../app/components/itunes/main/itunes_runtime.ixx) | Testable playback and lifecycle boundaries |
| `journal_clock` | [`journal_clock.ixx`](../app/components/journal/main/journal_clock.ixx) | Journal extended timestamp from `extended_hour.clock` in the journal data directory |
| `journal_extended_hours` | [`journal_extended_hours.ixx`](../app/components/journal/shared/journal_extended_hours.ixx) | Cutoff token parse and format. `n` extends while `hour < n`; `+n` also includes exactly `n:00` |
| `journal_cloud_client` | [`journal_cloud_client.ixx`](../app/components/journal/main/journal_cloud_client.ixx) | `journal_ac.exe` client that launches `journal_cloud.exe --serve` |
| `journal_commands` | [`journal_commands.ixx`](../app/components/journal/main/journal_commands.ixx) | Journal runtime command registry |
| `journal_factories` | [`journal_factories.ixx`](../app/components/journal/shared/journal_factories.ixx) | Compiled factory registry, alias parse/serialize, and starter files |
| `journal_component` | [`journal_component.ixx`](../app/components/journal/main/journal_component.ixx) | `journal_ac.exe` Component session |
| `journal_cloud_protocol` | [`journal_cloud_protocol.ixx`](../app/components/journal/shared/journal_cloud_protocol.ixx) | Private `ac.journal.cloud.v1` pipe between `journal_ac.exe` and `journal_cloud.exe` |
| `journal_db_protocol` | [`journal_db_protocol.ixx`](../app/components/journal/shared/journal_db_protocol.ixx) | Private `ac.journal.db.v2` pipe for `journal_db.exe` |
| `journal_db_client` | [`journal_db_client.ixx`](../app/components/journal/main/journal_db_client.ixx) | Runtime owner that launches `journal_db.exe --serve` |
| `journal_db_session` | [`journal_db_session.ixx`](../app/components/journal/shared/journal_db_session.ixx) | Short-lived database pipe calls |
| `journal_episode_format` | [`journal_episode_format.ixx`](../app/components/journal/shared/journal_episode_format.ixx) | Zero-padding for an episode number |
| `journal_firebase` | [`journal_firebase.ixx`](../app/components/journal/shared/journal_firebase.ixx) | `firebase.id` text and the JSON push body |
| `journal_auto_select` | [`journal_auto_select.ixx`](../app/components/journal/shared/journal_auto_select.ixx) | `on` / `off` parsing for `auto_select_new_series` |
| `journal_remote_sync` | [`journal_remote_sync.ixx`](../app/components/journal/shared/journal_remote_sync.ixx) | `on` / `off` parsing for `remote_sync` |
| `journal_series_map` | [`journal_series_map.ixx`](../app/components/journal/shared/journal_series_map.ixx) | `series.map` active line and snapshot text |
| `journal_sqlite` | [`journal_sqlite.ixx`](../app/components/journal/db/journal_sqlite.ixx) | Private `series.db` store. Production linkage is `journal_db.exe` only |
| `journal_title` | [`journal_title.ixx`](../app/components/journal/main/journal_title.ixx) | Episode title and new-file workflow |
| `server_default_site` | [`default_site.ixx`](../app/components/server/shared/default_site.ixx) | Compiled `index.html` and `styles.css` starter text |
| `server_data` | [`server_data_detail.hpp`](../app/components/server/shared/server_data_detail.hpp) | `[server] directory`, `port.id`, and `document_root.id` resolution |
| `server_logging` | [`server_logging.ixx`](../app/components/server/main/server_logging.ixx) | `server_ac.exe` Component logging |
| `music` | [`music.ixx`](../app/components/slash/main/music.ixx) | Recycled music-file naming |
| `path_utils` | [`path_utils.ixx`](../app/components/slash/main/path_utils.ixx) | Path stem and extension helpers |
| `spotify_client` | [`spotify_client.ixx`](../app/components/spotify/main/spotify_client.ixx) | Spotify Web API client state |
| `spotify_component` | [`spotify_component.ixx`](../app/components/spotify/main/spotify_component.ixx) | `spotify_ac.exe` Component session |
| `spotify_http` | [`spotify_http.ixx`](../app/components/spotify/main/spotify_http.ixx) | HTTP status helpers |
| `spotify_monitor` | [`spotify_monitor.ixx`](../app/components/spotify/main/spotify_monitor.ixx) | Playback monitor |
| `spotify_pipe` | [`spotify_pipe.ixx`](../app/components/spotify/main/spotify_pipe.ixx) | Pipe command registration |
| `spotify_registry` | [`spotify_registry.ixx`](../app/components/spotify/main/spotify_registry.ixx) | Spotify runtime command registry |
| `spotify_oauth` | [`spotify_oauth.ixx`](../app/components/spotify/oauth/spotify_oauth.ixx) | OAuth authorization and the initial `tokens.map` write |
| `spotify_application_data` | [`application_data.ixx`](../app/components/spotify/shared/application_data.ixx) | `client.id` and `devices.list` parse and serialize |
| `spotify_token_store` | [`token_store.ixx`](../app/components/spotify/shared/token_store.ixx) | `tokens.map` parse, serialize, and coordinated replace |
| `spotify_db_client` | [`spotify_db_client.ixx`](../app/components/spotify/main/spotify_db_client.ixx) | `spotify_ac.exe` client for `spotify_db.exe` |
| `spotify_sqlite` | [`spotify_sqlite.ixx`](../app/components/spotify/db/spotify_sqlite.ixx) | `history.db` schema and upsert, compiled only into `spotify_db.exe` |
| `taskbar_commands` | [`taskbar_commands.ixx`](../app/components/taskbar/main/taskbar_commands.ixx) | `taskbar_ac.exe` command registration |
| `taskbar_logging` | [`taskbar_logging.ixx`](../app/components/taskbar/main/taskbar_logging.ixx) | `taskbar_ac.exe` logging |
| `wake_logging` | [`wake_logging.ixx`](../app/components/wake/main/wake_logging.ixx) | Wake event logging via `powercfg /lastwake` |
| `writer_commands` | [`writer_commands.ixx`](../app/components/writer/main/writer_commands.ixx) | Writer runtime command registry |
| `writer_data` | [`writer_data_detail.hpp`](../app/components/writer/shared/writer_data_detail.hpp) | Writer directory, session prompts, task list, and notes resolution |
| `writer_component` | [`writer_component.ixx`](../app/components/writer/main/writer_component.ixx) | `writer_ac.exe` Component session |
