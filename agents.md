# Auto Core

Auto Core is a C++23 Windows automation system. Production builds are **Release x64**.

## Testing

Do not generate, modify, build, execute, inspect, or analyze tests unless explicitly requested. Do not perform testing-related verification.

AutoCore.sln is the production solution and may be built to verify compilation. Do not build Tests.sln unless explicitly requested.

## Project layout

- `app/core/` — `auto_core.dll`
- `app/main/` — Main application family
- `app/components/` — optional components and their supporting executables
- `app/shared/` — cross-subsystem contracts
- `devs/` — developing Auto Core
- `docs/` — using and configuring Auto Core, and installed runtime behavior
- `lib/` — canonical `auto_core.lib` and `auto_core.dll`
- `dist/` — installation layout. `app\` holds Main executables and shared DLLs. `components\` holds `<name>_ac.exe`

Within Main and components, directories follow executable responsibility:
`runtime/`, `config/`, `builder/`, `settings/`, and `shared/`. Main uses
`editors/` and `init/`. A component editor directory is `editor/`.

A `.vcxproj` represents one executable or library. `app/AutoCore.sln` is the production workspace. `app/Tests.sln` builds the complete test suite. Subsystem and component directories may also provide focused `.sln` files.

## Architecture

Start with interfaces (`.ixx`) before implementations.

Component communication uses `app/shared/protocols/component_protocol.ixx`.
Component-specific protocols belong in that component's `shared/` directory.

Configuration commands own configuration files. Editors own persistent operational data. Init executables orchestrate first-run setup rather than owning configuration.

A component is one `<name>_ac.exe`. `--run` is the hosted session, `--settings` is the settings menu, and `--config` is the sole owner of `config/components/<name>.ini`. A no-argument launch opens settings when that INI exists or the existence check fails, and otherwise runs `--config --init`. Third-party components use the same commands. Main keeps `auto_core_settings.exe` for its settings menu, `auto_core_settings.exe --config` for Main configuration, and `auto_core_settings.exe --initialize` for first-run initialization.

## Documentation

Start with [readme.md](readme.md) for the project overview, [docs/](docs/) for using and configuring Auto Core, and [devs/](devs/) for developing it.

For architecture, packaging, build, or repository-wide design decisions, consult [devs/strategy.md](devs/strategy.md).
