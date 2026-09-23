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
| `auto_core.core.config` | [`core_config.ixx`](../app/core/modules/core_config.ixx) | Cached `main.ini`; `journal_config.exe` may seed `journal/journal_choices.ini` |
| `auto_core.core.encoding` | [`encoding.ixx`](../app/core/modules/encoding.ixx) | Strict UTF-8 / UTF-16 conversion |
| `auto_core.core.error` | [`error.ixx`](../app/core/modules/error.ixx) | Best-effort stderr and `errors/errors.log` reporting |
| `auto_core.core.formatting` | [`formatting.ixx`](../app/core/modules/formatting.ixx) | UTF-8 `std::format` with wide-text normalization |
| `auto_core.core.ini` | [`ini.ixx`](../app/core/modules/ini.ixx) | Sectioned INI parse and file read |
| `auto_core.core.keyboard` | [`keyboard.ixx`](../app/core/modules/keyboard.ixx) | `SendInput` helpers, including `VK_RWIN`+position |
| `auto_core.core.logging.protocol` | [`log_protocol.ixx`](../app/core/modules/log_protocol.ixx) | Versioned frames for `logger_ac.exe` |
| `auto_core.core.logging.config` | [`logging_config.ixx`](../app/core/modules/logging_config.ixx) | Cached `logger.ini` settings and `components.ini` `[components]` logger enablement |
| `auto_core.core.logging.client` | [`main_log_client.ixx`](../app/core/modules/main_log_client.ixx) | Asynchronous component connection to `logger_ac.exe` |
| `auto_core.core.paths` | [`paths.ixx`](../app/core/modules/paths.ixx) | Process-lifetime `bin_directory` and `installation_root` |
| `auto_core.core.pipes` | [`pipes.ixx`](../app/core/modules/pipes.ixx) | Named-pipe handles, string frames, and command dispatch |
| `auto_core.core.thread` | [`thread.ixx`](../app/core/modules/thread.ixx) | Thread-entry exception reporting |
| `auto_core.taskbar` | [`core_taskbar.ixx`](../app/core/taskbar/core_taskbar.ixx) | Snapshot authority, client lookup, and native Win+position |

## Main executable (`app/main/component`)

Process lifetime for `auto_core.exe` is [main.md](main.md).

| Module | Source | Role |
| --- | --- | --- |
| `auto_core.main.application` | [`main_component.ixx`](../app/main/component/modules/main_component.ixx) | Main `Component`, process launch, F-lock, shutdown |
| `auto_core.main.components` | [`ac_components.ixx`](../app/main/component/modules/ac_components.ixx) | Generic host: open `components.ini` `[components]`, hello, invoke, shutdown |
| `auto_core.main.components.dash` | [`dash_component.ixx`](../app/main/component/modules/dash_component.ixx) | Launches `dash_ac.exe` |
| `auto_core.main.components.slash` | [`slash_component.ixx`](../app/main/component/modules/slash_component.ixx) | Launches Slash recycle-bin commands |
| `auto_core.main.components.taskbar` | [`taskbar_component.ixx`](../app/main/component/modules/taskbar_component.ixx) | Main-local taskbar launches and reserved control names |
| `auto_core.main.crash_recovery` | [`crash_recovery.ixx`](../app/main/component/modules/crash_recovery.ixx) | Previous-crash dialog and restart handler |
| `auto_core.main.key_codes` | [`key_codes.ixx`](../app/main/component/modules/key_codes.ixx) | Normalized virtual-key codes and `mappings.ini` names (`keys`, `resolve`) |
| `auto_core.main.keyboard_input` | [`keyboard_input.ixx`](../app/main/component/modules/keyboard_input.ixx) | Low-level hook and main-thread dispatch |
| `auto_core.main.keymap` | [`keymap.ixx`](../app/main/component/modules/keymap.ixx) | Active primary/secondary key bindings |
| `auto_core.main.keymap.runtime` | [`keymap_runtime.ixx`](../app/main/component/modules/keymap_runtime.ixx) | File keymap, command registry, workspace files |
| `auto_core.main.logger` | [`logger_init.ixx`](../app/main/component/modules/logger_init.ixx) | Starts and stops `logger_ac.exe` from Main |
| `auto_core.main.program_ready` | [`program_ready.ixx`](../app/main/component/modules/program_ready.ixx) | Startup status output |
| `auto_core.main.shutdown_events` | [`shutdown_events.ixx`](../app/main/component/modules/shutdown_events.ixx) | Console and session shutdown |
| `auto_core.main.taskbar` | [`main_taskbar.ixx`](../app/main/component/modules/main_taskbar.ixx) | Interactive cycling session owned by Main |
| `auto_core.main.test_commands` | [`test_commands.ixx`](../app/main/component/modules/test_commands.ixx) | Diagnostic keymap commands |

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
| `taskbar_config_protocol` | [`taskbar_config_protocol.ixx`](../app/components/taskbar/shared/taskbar_config_protocol.ixx) | Discovery pipe for `taskbar_config.exe` |
| `taskbar_protocol` | [`taskbar_protocol.ixx`](../app/components/taskbar/shared/taskbar_protocol.ixx) | Reserved Main-local taskbar command names |
| `wake_protocol` | [`wake_protocol.ixx`](../app/components/wake/shared/wake_protocol.ixx) | Legacy wake command names |
| `writer_protocol` | [`writer_protocol.ixx`](../app/components/writer/shared/writer_protocol.ixx) | Writer command names used by `writer_ac.exe` |

## Components (`app/components`)

| Module | Source | Role |
| --- | --- | --- |
| `itunes_client` | [`itunes_client.ixx`](../app/components/itunes/main/itunes_client.ixx) | COM client and shared iTunes process state |
| `itunes_component` | [`itunes_component.ixx`](../app/components/itunes/main/itunes_component.ixx) | `itunes_ac.exe` Component session |
| `itunes_monitor` | [`itunes_monitor.ixx`](../app/components/itunes/main/itunes_monitor.ixx) | Track-change monitor |
| `itunes_pipe` | [`itunes_pipe.ixx`](../app/components/itunes/main/itunes_pipe.ixx) | Pipe command registration |
| `itunes_registry` | [`itunes_registry.ixx`](../app/components/itunes/main/itunes_registry.ixx) | iTunes runtime command registry |
| `itunes_removal` | [`itunes_removal.ixx`](../app/components/itunes/main/itunes_removal.ixx) | Recycle-bin track removal |
| `itunes_runtime` | [`itunes_runtime.ixx`](../app/components/itunes/main/itunes_runtime.ixx) | Testable playback and lifecycle boundaries |
| `journal_clock` | [`journal_clock.ixx`](../app/components/journal/main/journal_clock.ixx) | Journal day-rollover timestamp |
| `journal_cloud` | [`journal_cloud.ixx`](../app/components/journal/main/journal_cloud.ixx) | Optional Firebase sync |
| `journal_commands` | [`journal_commands.ixx`](../app/components/journal/main/journal_commands.ixx) | Journal runtime command registry |
| `journal_component` | [`journal_component.ixx`](../app/components/journal/main/journal_component.ixx) | `journal_ac.exe` Component session |
| `journal_database` | [`journal_database.ixx`](../app/components/journal/main/journal_database.ixx) | `journals.db` series, counters, Firebase URL |
| `journal_title` | [`journal_title.ixx`](../app/components/journal/main/journal_title.ixx) | Episode title and new-file workflow |
| `log_init` | [`log_init.ixx`](../app/components/logger/main/log_init.ixx) | `logger_ac.exe` startup logging |
| `logger_state` | [`logger_state.ixx`](../app/components/logger/main/logger_state.ixx) | Logger process state |
| `main_log` | [`main_log.ixx`](../app/components/logger/main/main_log.ixx) | Writes decoded central-log events |
| `server_logging` | [`server_logging.ixx`](../app/components/server/main/server_logging.ixx) | `server_ac.exe` Component logging |
| `music` | [`music.ixx`](../app/components/slash/main/music.ixx) | Recycled music-file naming |
| `path_utils` | [`path_utils.ixx`](../app/components/slash/main/path_utils.ixx) | Path stem and extension helpers |
| `spotify_client` | [`spotify_client.ixx`](../app/components/spotify/main/spotify_client.ixx) | Spotify Web API client state |
| `spotify_component` | [`spotify_component.ixx`](../app/components/spotify/main/spotify_component.ixx) | `spotify_ac.exe` Component session |
| `spotify_http` | [`spotify_http.ixx`](../app/components/spotify/main/spotify_http.ixx) | HTTP status helpers |
| `spotify_monitor` | [`spotify_monitor.ixx`](../app/components/spotify/main/spotify_monitor.ixx) | Playback monitor |
| `spotify_pipe` | [`spotify_pipe.ixx`](../app/components/spotify/main/spotify_pipe.ixx) | Pipe command registration |
| `spotify_registry` | [`spotify_registry.ixx`](../app/components/spotify/main/spotify_registry.ixx) | Spotify runtime command registry |
| `spotify_oauth` | [`spotify_oauth.ixx`](../app/components/spotify/oauth/spotify_oauth.ixx) | OAuth authorization and device registration |
| `taskbar_commands` | [`taskbar_commands.ixx`](../app/components/taskbar/main/taskbar_commands.ixx) | `taskbar_ac.exe` command registration |
| `taskbar_logging` | [`taskbar_logging.ixx`](../app/components/taskbar/main/taskbar_logging.ixx) | `taskbar_ac.exe` logging |
| `wake_logging` | [`wake_logging.ixx`](../app/components/wake/main/wake_logging.ixx) | Wake event logging via `powercfg /lastwake` |
| `writer_commands` | [`writer_commands.ixx`](../app/components/writer/main/writer_commands.ixx) | Writer runtime command registry |
| `writer_component` | [`writer_component.ixx`](../app/components/writer/main/writer_component.ixx) | `writer_ac.exe` Component session |
