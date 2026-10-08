# Auto Core

Auto Core is a C++23 Windows automation system. Production builds are **Release x64**.

## Project layout

- `app/core/` — `auto_core.dll`
- `app/main/` — Main application family
- `app/components/` — optional components and their supporting executables
- `app/shared/` — cross-subsystem contracts
- `devs/` — developing Auto Core
- `docs/` — using and configuring Auto Core, and installed runtime behavior
- `lib/` — canonical `auto_core.lib` and `auto_core.dll`
- `dist/` — installation layout

Within Main and components, directories follow executable responsibility:
`runtime/`, `config/`, `builder/`, `star/`, and `shared/`. Main uses
`editors/` and `init/`. A component editor directory is `editor/`.

A `.vcxproj` represents one executable or library. `app/AutoCore.sln` is the repository workspace; subsystem and component directories may also provide focused `.sln` files.

## Architecture

Start with interfaces (`.ixx`) before implementations.

Component communication uses `app/shared/protocols/component_protocol.ixx`.
Component-specific protocols belong in that component's `shared/` directory.

Configuration executables own configuration files. Editors own persistent operational data. Init executables orchestrate first-run setup rather than owning configuration.

Runtime component executables use `<name>_ac.exe`. User-facing component management executables use `<name>_star.exe`.

## Documentation

Start with [README.md](readme.md) for the project overview, [docs/](docs/) for using and configuring Auto Core, and [devs/](devs/) for developing it.

For architecture, packaging, build, or repository-wide design decisions, consult [devs/STRATEGY.md](devs/STRATEGY.md).
