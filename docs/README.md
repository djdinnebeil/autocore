# Documentation

Index of Auto Core developer documentation.

## Guides

| Document | Topic |
| --- | --- |
| [Building](building.md) | Solutions, MSBuild order, output directories |
| [Strategy](STRATEGY.md) | Goals, locked decisions, agent git |
| [Configuration](configuration.md) | `dist/config`, `defaults/` samples, keymap, logging, local runtime data |
| [Development](development.md) | Shared protocols, runtime commands, adding a component |
| [Adding a new component](new-component.md) | File → New → Project, portable `AutoCore.props`, live `components.ini` `[components]` |
| [Main](main.md) | `auto_core.exe` startup, hook, crash restart, shutdown |
| [Modules](modules.md) | C++23 module catalog |

Root docs for sharing the project: [CONTRIBUTING.md](../CONTRIBUTING.md), [SECURITY.md](../SECURITY.md), [NOTICE.md](../NOTICE.md).

Product follow-ups are in [TODO.md](TODO.md). DLL-specific deferred work is in [`app/core/TODO.md`](../app/core/TODO.md). Main-specific deferred work is in [`app/main/TODO.md`](../app/main/TODO.md).

## Components

| Document | Topic |
| --- | --- |
| [Dash](dash.md) | Secret storage, `dash.ini`, `dash_config.exe` |
| [iTunes](itunes.md) | COM automation, `itunes.ini`, `itunes_config.exe` |
| [Journal](journal.md) | Titles, `journal.ini`, `journal_config.exe` |
| [Logger](logger.md) | Central logger, `logger.ini`, `logger_config.exe` |
| [Slash](slash.md) | Recycle-bin helper, `slash.ini` |
| [Spotify](spotify.md) | Web API, `spotify.ini`, `spotify_config.exe`, oauth |
| [Taskbar](taskbar.md) | Snapshot authority, `taskbar.ini`, `taskbar_config.exe` |
| [Wake](wake.md) | Last-wake logging, `wake.ini` |
| [Writer](writer.md) | Notes and prompts, `writer.ini`, `writer_config.exe` |
| [Server](server.md) | Local loopback file server and `server_config.exe` |

## Tests

- Core: [`app/core/vs/auto_core_tests.vcxproj`](../app/core/vs/auto_core_tests.vcxproj) → `obj\auto_core_tests\auto_core_tests.exe`
- Main: [`app/main/component/vs/auto_core_main_tests.vcxproj`](../app/main/component/vs/auto_core_main_tests.vcxproj) → `obj\auto_core_main_tests\auto_core_main_tests.exe`
- iTunes: [`app/components/itunes/tests/TESTING.md`](../app/components/itunes/tests/TESTING.md) → `obj\itunes_tests\itunes_tests.exe "~[live]"`
- Spotify: [`app/components/spotify/tests/TESTING.md`](../app/components/spotify/tests/TESTING.md) → `obj\spotify_tests\spotify_tests.exe "~[live]"`

See [Building](building.md) and [CONTRIBUTING.md](../CONTRIBUTING.md) for the MSBuild commands.
