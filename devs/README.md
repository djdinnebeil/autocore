# Developer documentation

Developing Auto Core: contribution, building, architecture, maintenance, and internal project documentation.

Using and configuring an installation starts at [Auto Core Guide](<../docs/Auto Core Guide.md>), [Auto Core Tutorial](<../docs/Auto Core Tutorial.md>), and [Auto Core Manual](<../docs/Auto Core Manual.md>).

## Guides

| Document | Topic |
| --- | --- |
| [Contributing](contributing.md) | Clone, build, tests, what not to commit |
| [Building](building.md) | Solutions, MSBuild order, output directories |
| [Strategy](STRATEGY.md) | Goals, locked decisions, agent git |
| [Architecture inventory](architecture.md) | Phase 1A glossary, executables, subsystems, and internal protocols. Not the frozen standard. |
| [Persistent state](persistent-state.md) | Phase 1A ledger of every non-INI file that survives process exit |
| [Main](main.md) | `auto_core.exe` startup, hook, crash restart, shutdown |
| [Development](development.md) | Shared protocols, runtime commands, adding a component |
| [Adding a new component](new-component.md) | File → New → Project, portable `AutoCore.props`, live `components.ini` `[components]` |
| [Modules](modules.md) | Phase 1A ledger of every production `.ixx`. Dispositions are pending. |
| [Follow-up work](TODO.md) | Product follow-ups |

DLL-specific deferred work is in [`app/core/TODO.md`](../app/core/TODO.md). Main-specific deferred work is in [`app/main/runtime/TODO.md`](../app/main/runtime/TODO.md).

## Tests

- Core: [`app/core/tests/auto_core_tests.vcxproj`](../app/core/tests/auto_core_tests.vcxproj) → `obj\auto_core_tests\auto_core_tests.exe`
- Main: [`app/main/tests/auto_core_main_tests.vcxproj`](../app/main/tests/auto_core_main_tests.vcxproj) → `obj\auto_core_main_tests\auto_core_main_tests.exe`
- iTunes: [`app/components/itunes/tests/TESTING.md`](../app/components/itunes/tests/TESTING.md) → `obj\itunes_tests\itunes_tests.exe "~[live]"`
- Spotify: [`app/components/spotify/tests/TESTING.md`](../app/components/spotify/tests/TESTING.md) → `obj\spotify_tests\spotify_tests.exe "~[live]"`
- Server: [`app/components/server/tests/TESTING.md`](../app/components/server/tests/TESTING.md) → `obj\server_tests\server_tests.exe "[server][unit]"`

See [Building](building.md) and [Contributing](contributing.md) for the MSBuild commands.
