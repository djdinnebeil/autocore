# Documentation

Index of Auto Core developer documentation.

## Guides

| Document | Topic |
| --- | --- |
| [Building](building.md) | Solutions, MSBuild order, output directories |
| [Strategy](STRATEGY.md) | Goals, locked decisions, agent git |
| [Configuration](configuration.md) | `dist/config`, `defaults/` samples, keymap, logging, local runtime data |
| [Development](development.md) | Shared protocols, runtime commands, adding a component |
| [Main](main.md) | `auto_core.exe` startup, hook, crash restart, shutdown |
| [Modules](modules.md) | C++23 module catalog |

Root docs for sharing the project: [CONTRIBUTING.md](../CONTRIBUTING.md), [SECURITY.md](../SECURITY.md), [NOTICE.md](../NOTICE.md).

Product follow-ups are in [TODO.md](TODO.md). DLL-specific deferred work is in [`app/core/TODO.md`](../app/core/TODO.md). Main-specific deferred work is in [`app/main/TODO.md`](../app/main/TODO.md).

## Components

| Document | Topic |
| --- | --- |
| [Dash](dash.md) | Secret storage contract, threat model, recovery |
| [iTunes](itunes.md) | COM automation, protocol, testing, limitations |
| [Spotify](spotify.md) | Web API, protocol, local history, testing |
| [Taskbar](taskbar.md) | INI contract, window matching, authority commands |
| [Taskbar config](taskbar_config.md) | On-demand INI generator (see also [taskbar.md](taskbar.md)) |
| [Writer config](writer_config.md) | `writer.ini` prompt and stub menu (`writer_config.exe`) |
| [Server](server.md) | Local loopback file server (`server_ac.exe`) |
| [Server config](server_config.md) | `server.ini` first-write and get/set menu (`server_config.exe`) |

## Tests

- Core: [`app/core/vs/auto_core_tests.vcxproj`](../app/core/vs/auto_core_tests.vcxproj) → `out\obj\auto_core_tests\auto_core_tests.exe`
- Main: no unit-test project; diagnostic keymap names live in `auto_core.main.test_commands`
- iTunes: [`app/components/itunes/TESTING.md`](../app/components/itunes/TESTING.md) → `out\obj\itunes_tests\itunes_tests.exe "~[live]"`
- Spotify: [`app/components/spotify/TESTING.md`](../app/components/spotify/TESTING.md) → `out\obj\spotify_tests\spotify_tests.exe "~[live]"`

See [Building](building.md) and [CONTRIBUTING.md](../CONTRIBUTING.md) for the MSBuild commands.
