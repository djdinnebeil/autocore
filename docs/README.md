# Documentation

Index of Auto Core developer documentation.

## Guides

| Document | Topic |
| --- | --- |
| [Building](building.md) | Solutions, MSBuild order, output directories |
| [Strategy](STRATEGY.md) | Goals, locked decisions, agent git |
| [Configuration](configuration.md) | `dist/config`, keymap, logging, and the Phase 1A INI key ledger |
| [Architecture inventory](architecture.md) | Phase 1A glossary, executables, subsystems, and internal protocols. Not the frozen standard. |
| [Persistent state](persistent-state.md) | Phase 1A ledger of every non-INI file that survives process exit |
| [Development](development.md) | Shared protocols, runtime commands, adding a component |
| [Adding a new component](new-component.md) | File → New → Project, portable `AutoCore.props`, live `components.ini` `[components]` |
| [Main](main.md) | `auto_core.exe` startup, hook, crash restart, shutdown |
| [Modules](modules.md) | Phase 1A ledger of every production `.ixx`. Dispositions are pending. |

Root docs for sharing the project: [CONTRIBUTING.md](../CONTRIBUTING.md), [SECURITY.md](../SECURITY.md), [NOTICE.md](../NOTICE.md).

Product follow-ups are in [TODO.md](TODO.md). DLL-specific deferred work is in [`app/core/TODO.md`](../app/core/TODO.md). Main-specific deferred work is in [`app/main/runtime/TODO.md`](../app/main/runtime/TODO.md).

## Components

| Document | Topic |
| --- | --- |
| [Dash](dash.md) | Secret storage, `dash.ini`, `dash_config.exe` |
| [iTunes](itunes.md) | COM automation, `itunes.ini`, `itunes_config.exe`, `itunes_formatter.exe` |
| [Journal](journal.md) | Titles, `journal.ini`, `journal_config.exe` |
| [Logger](logger.md) | Per-executable logs, `logging.ini`, `logger.ini` |
| [Slash](slash.md) | Recycle-bin helper, `slash.ini` |
| [Spotify](spotify.md) | Web API, `spotify.ini`, `spotify_config.exe`, oauth |
| [Taskbar](taskbar.md) | Snapshot authority, `taskbar.ini`, `taskbar_builder.exe` |
| [Wake](wake.md) | Last-wake logging, `wake.ini` |
| [Writer](writer.md) | Notes and prompts, `writer.ini`, `writer_config.exe`, `writer_editor.exe` |
| [Server](server.md) | Local loopback file server, `server_config.exe`, `server_editor.exe`, and `server_builder.exe` |

## Tests

- Core: [`app/core/tests/auto_core_tests.vcxproj`](../app/core/tests/auto_core_tests.vcxproj) → `obj\auto_core_tests\auto_core_tests.exe`
- Main: [`app/main/tests/auto_core_main_tests.vcxproj`](../app/main/tests/auto_core_main_tests.vcxproj) → `obj\auto_core_main_tests\auto_core_main_tests.exe`
- iTunes: [`app/components/itunes/tests/TESTING.md`](../app/components/itunes/tests/TESTING.md) → `obj\itunes_tests\itunes_tests.exe "~[live]"`
- Spotify: [`app/components/spotify/tests/TESTING.md`](../app/components/spotify/tests/TESTING.md) → `obj\spotify_tests\spotify_tests.exe "~[live]"`
- Server: [`app/components/server/tests/TESTING.md`](../app/components/server/tests/TESTING.md) → `obj\server_tests\server_tests.exe "[server][unit]"`

See [Building](building.md) and [CONTRIBUTING.md](../CONTRIBUTING.md) for the MSBuild commands.
