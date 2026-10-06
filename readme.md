# Auto Core for Windows 11

Auto Core is a C++23 automation utility for Windows 11. Its core feature is a specialized keyboard manager for numpad and system-level shortcuts, supporting quick taskbar access, automated text insertion, journaling workflows, and custom command execution.

Auto Core also includes component-based automation outside the keyboard layer, including music listening history, local file management, and system event logging.

This repository is for people who want to **build or extend** the source and **run** the `dist/` tree they built. Beta testers are developers: clone or copy the tree, build Release x64, then start `dist\bin\auto_core.exe` (or the root `Auto Core.lnk`). There is no installer.

> [!WARNING]
> Auto Core installs a low-level keyboard hook. Antivirus may flag it. Review [SECURITY.md](SECURITY.md) before you run it. Dash is not a password manager.

## Contents

- [Using Auto Core](#using-auto-core)
- [Building](#building)
- [Extending](#extending)
- [Main Features](#main-features)
- [Project Architecture](#project-architecture)
- [Project Folder Structure](#project-folder-structure)
- [Components](#components)
- [Requirements](#requirements)
- [Documentation](#documentation)
- [History](#history)
- [License](#license)

## Using Auto Core

Build from source (below), then run from `dist/` (`dist\bin\auto_core.exe`, runtime DLLs, and the component executables). Projects use the DLL CRT (`/MD`); if you run binaries you did not build on that PC, install the matching **MSVC v145** redistributable.

1. Start `auto_core.exe` from `dist\bin` (or `Auto Core.lnk` at the installation root). If `config/auto_core.ini` is missing, Main launches `auto_core_init.exe`. That orchestrator writes `auto_core.ini` with `auto_core_config.exe` first, then the other Main INIs, runs `components_init.exe` for component INIs, then `components_editor.exe --seed` and `keymap_editor.exe`. Presence of `auto_core.ini` means Auto Core is initialized. There is no `initialized` key. A failed required step removes `auto_core.ini` when that run created it. If a component INI is missing at runtime, that process uses the defaults in `app/components/<name>/shared/defaults.ixx` and reports the gap; it does not write the file.
2. Edit `dist/keymap.map` if you want a custom map. If the file is missing, run `keymap_editor.exe` to write a seed (`numpad_0` / `numpad_1` filled, every other `key_codes` name blank). An existing file is not overwritten.
3. Run `taskbar_builder.exe` from `dist\bin` so `taskbar/applications/*.map` matches the programs you pin. See [Taskbar](docs/taskbar.md).
4. Run `<name>_config.exe` from `dist\bin` to generate `config/<name>.ini`. Spotify tokens still use `spotify_oauth.exe` ([Spotify](docs/spotify.md)). Restart `server_ac.exe` after rewriting `server.ini`, `port.id`, or `document_root.id`.
5. Windows may prompt for elevation; that is required for some iTunes setups.

`dist/` is gitignored. Portable defaults are compiled in each child's `shared/defaults.ixx` and in [`app/main/shared/defaults.ixx`](app/main/shared/defaults.ixx) for Main host INIs. `*.local.ini` / `*.local.ixx` also stay off git. See [configuration](docs/configuration.md).

To run the same build on another Windows 11 PC, copy the `dist/` folder, then retune machine-specific files: install the **MSVC v145** redistributable if that PC did not build the binaries; do not copy `components/spotify/tokens.map` (run `spotify_oauth.exe` there); retarget `config/logging.ini` if `directory` is an absolute path; re-run `taskbar_builder.exe` when pins or exe paths differ. Dash is not portable between Windows installs. Details are in [configuration](docs/configuration.md#copying-dist-to-another-windows-11-pc).

## Building

1. Clone or copy the repository onto Windows 11.
2. Install Visual Studio **2026** (version **18+**) with the Desktop development with C++ workload (MSVC **v145**, C++23).
3. From the repository root, run [`scripts/build-all.ps1`](scripts/build-all.ps1), or follow the MSBuild order in [Building](docs/building.md). `auto_core_dll` publishes `lib/auto_core.dll` to `dist/bin/auto_core.dll`. When third-party runtime DLLs change, run [`scripts/copy-vendor-dlls.ps1`](scripts/copy-vendor-dlls.ps1) to copy `third_party/*/bin/*.dll` into `dist/bin/`. Builds do not run that script.
4. Shared paths live in [`msbuild/AutoCore.props`](msbuild/AutoCore.props) (repo-relative; a clone does not edit that file).

The linker searches `lib/auto_core.lib`. `dist/` is gitignored.

## Extending

New components, shared protocols, and keymap registration are documented in [Development](docs/development.md). Creating a child from File → New → Project is [Adding a new component](docs/new-component.md). The module catalog is [Modules](docs/modules.md). Contribution mechanics (tests, what not to commit) are in [CONTRIBUTING.md](CONTRIBUTING.md).

Journal aliases live in per-factory `.list` files under the configured journal data directory (default `dist/components/journal/`). `journal_builder.exe` owns those files. `journal_builder.exe --seed` creates a missing starter file and leaves an existing file unchanged. `journal_config.exe --init` and `--seed` walk the Journal stores by launching each owner. `journal_ac.exe` reads the files at startup and advertises the alias names. Restart Journal after a change. Unused aliases are not bound; `keymap.map` is the filter.

## Main Features

- **Numpad Keyboard Manager** — Monitors and intercepts numpad and additional keys to execute configured tasks.
- **System Console Window** — Uses the system console to display output and request input.
- **Automated Text Insertion** — Inserts text into the active text box by using the system clipboard and the `Ctrl + V` paste shortcut.
- **Secured Text Insertion** — While not intended as a credentials manager, there is a secured mode for more sensitive text.
- **Taskbar Activation** — Activates pinned programs with `Win + 0` through `Win + 9` when they occupy positions 1 through 10, and launches a configured fallback executable otherwise.
- **Music Player Integration** — Provides integration points for iTunes and Spotify.
- **Journaling Support** — Automates title generation and file creation for journaling workflows.
- **Creative Writing Inspiration** — Uses dice-roll-style randomization to generate numbers or prompts for writing inspiration.
- **Browser-Based Local File Management** — Runs a local server to support browser-based access to local files.
- **System Maintenance** — Supports maintenance tasks such as emptying the recycle bin and logging system wake events.

## Project Architecture

Auto Core uses a modular component architecture that separates the main system controller from specialized component executables.

The main application, core DLL, shared protocols, and component projects are separated by responsibility. Components that need to communicate with the main application use named pipes, with IPC support provided by the `auto_core.pipes` module.

### Components

| Component | Purpose | Executable | Notes |
| --- | --- | --- | --- |
| `dash` | Local secret insertion | `dash_ac.exe` | Current-user DPAPI with a Windows Hello access check; see [Dash](docs/dash.md) |
| `journal` | Journaling titles and file workflow | `journal_ac.exe` | Pipe child of Auto Core; episode counters in `series.db` under `[journal] directory` |
| `logger` | Merged main log | `logger_ac.exe` | Reads local `.main.log` files into `YYYY-MM-DD_main.log`. See [Logger](docs/logger.md) |
| `journal_config` | Journal configuration | `journal_config.exe` | Writes only `config/journal.ini`. `--init` and `--seed` launch the owners of the other Journal stores |
| `journal_builder` | Journal alias files | `journal_builder.exe` | Owns `<factory_name>.list` under the journal data directory. Interactive add/edit/delete; `--seed` creates missing starter files; `--init` prompts for each missing file |
| `journal_clock` | Journal extended hours | `journal_clock.exe` | Owns `extended_hour.clock` in the journal data directory (`components\journal` by default). Interactive token edit; `--seed` writes `extended_hours = +0` when the file is missing |
| `journal_db` | Journal series database | `journal_db.exe` | Owns `series.db`. `--serve` is the private database process. `--seed` creates a missing database and adds `Auto Core` with padding `2` |
| `journal_series` | Journal series map | `journal_series.exe` | Owns `series.map`. Select, add, count, padding, and refresh. `--seed` and `--init` refresh a missing map from `series.db`. Does not open SQLite |
| `journal_cloud` | Journal Firebase | `journal_cloud.exe` | Owns `firebase.id`. Interactive URL menu, and `--serve` when `remote_sync` is `on`. `--seed` creates a missing empty file |
| `itunes` | iTunes controller | `itunes_ac.exe` | Dedicated-owner-thread COM automation; see [iTunes](docs/itunes.md) |
| `server` | Local file server | `server_ac.exe` | Loopback HTTP; `server_config.exe` owns `server.ini`; `server_editor.exe` owns `port.id` and `document_root.id`; `server_builder.exe` writes only missing starter files under a relative document root |
| `slash` | Recycle bin utility | `slash_ac.exe` | Prints deleted items |
| `spotify` | Spotify controller | `spotify_ac.exe` | Web API playback control and local history; see [Spotify](docs/spotify.md) |
| `spotify_oauth` | Spotify authorization helper | `spotify_oauth.exe` | Handles the OAuth authorization flow (`spotify/oauth`) |
| `taskbar` | Native taskbar activation | `taskbar_ac.exe` | Snapshot authority for Win+position mappings; see [Taskbar](docs/taskbar.md) |
| `taskbar_config` | Taskbar configuration | `taskbar_config.exe` | Writes only `config/taskbar.ini` |
| `taskbar_builder` | Taskbar application definitions | `taskbar_builder.exe` | Owns `taskbar/applications/*.map` and `keymap/components/taskbar.keymap_commands.txt` |
| `wake` | System wake tracker | `wake_ac.exe` | Logs resume timestamps |
| `writer` | Text insertion and notes | `writer_ac.exe` | Pipe child of Auto Core; notepad, timestamps, and task list |
| `writer_config` | Writer configuration | `writer_config.exe` | Writes only `config/writer.ini`. `--init` and `--seed` delegate data provisioning to `writer_editor.exe` |
| `writer_editor` | Writer data | `writer_editor.exe` | Sole Auto Core writer of `task_list.txt`, `session_prompts.list`, and notes |

## Project Folder Structure

```text
Auto Core/
├─ .editorconfig        Encoding, newlines, indent
├─ .gitattributes       Line endings and binary types
├─ .gitignore           Outputs stay out of source
├─ AGENTS.md            Agent map
├─ README.md            Project overview
├─ LICENSE              License terms
├─ NOTICE.md            Third-party library attribution
├─ CONTRIBUTING.md      How to build and extend
├─ SECURITY.md          Hook, secrets, how to report issues
├─ scripts/             Repo build scripts and the manual vendor DLL refresh
├─ msbuild/             AutoCore.props (one level below repo root; not obj/)
├─ app/                 Build input (source, projects, resources). Not source-only
│  ├─ components/       Child trees: <name>/main, config, shared (spotify + oauth; tests when present)
│  ├─ core/             auto_core.dll source, include/ac_api.hpp, nested tests
│  ├─ main/             Main family: runtime/, editors/, config/, shared/
│  │  ├─ runtime/       auto_core.exe source only
│  │  ├─ editors/       components_editor.exe, keymap_editor.exe
│  │  ├─ config/        Main configuration executables
│  │  └─ shared/        Main-family shared code and defaults
│  ├─ resources/        Shared .ico and .rc files
│  └─ shared/           Compile-time IPC protocols and command_registry
├─ dist/                Gitignored assembled runtime for end users
│  ├─ bin/              Runtime executables and DLLs
│  ├─ components.list   Component enablement list
│  ├─ keymap.map        User key-to-command map
│  ├─ config/           Live configuration files
│  ├─ keymap/           keymap_commands.txt and components/ catalogs
│  ├─ components/       Optional component data (journal, spotify, server)
│  ├─ taskbar/          applications/*.map; winkey_map.cache
│  └─ writer/           session_prompts.list, task_list.txt, and notes/ (local)
├─ docs/                Developer documentation
├─ lib/                 Canonical auto_core.dll and auto_core.lib
├─ obj/                 Gitignored IntDir, including test binaries
├─ symbols/             Gitignored linker program databases (`.pdb`)
└─ third_party/         Committed vendors (`<dependency>/`; product `include`, `lib`, `bin`; Catch2 at `catch2/`)
```

`dist/` is gitignored runtime state. See [configuration](docs/configuration.md).

Crash reports, when enabled, are written under `dist/crash/` independently of
normal logging. Open the Auto Core installation folder, open `crash`, and send
only the requested event folder to the Auto Core team. A report contains
diagnostic process information and is never uploaded automatically.

Build artifacts are generated under `obj/` (intermediates), `lib/` (canonical `auto_core.lib` and `auto_core.dll`), `symbols/` (linker `.pdb` files), and `dist/bin/` (executables and the `auto_core.dll` published by `auto_core_dll`). Shared Visual Studio settings live in `msbuild/`. Wipe `obj/` anytime; rebuilding the core DLL overwrites `lib/` in place and publishes that DLL to `dist/bin/`.

## Dash

Dash stores and inserts revocable, noncritical secrets without plaintext secret files. Auto Core launches it with the `launch_dash` keymap command. Dash is not a general password manager, and its vault is not portable between ordinary Windows installations.

See [Dash secret storage](docs/dash.md) for the contract, threat model, storage location, recovery policy, and disclaimers.

## iTunes

The iTunes component controls playback through the iTunes COM automation interface and runs as `itunes_ac.exe`. `itunes_db.exe` owns the listening-history database `history.db`. `itunes_ac.exe` does not open that file.

> [!IMPORTANT]
> An elevated Auto Core process cannot connect to an iTunes instance that is already running without Administrator privileges. Either start Auto Core before iTunes, or close iTunes and restart it with Administrator privileges before starting Auto Core.

See [iTunes component](docs/itunes.md). Follow-up work is in [docs/TODO.md](docs/TODO.md).

## Spotify

The Spotify component runs as `spotify_ac.exe` and provides Web API playback control, queue and current-track formatting, playback-device transfer, album art download, and a local listening-history database.

See [Spotify component](docs/spotify.md).

## Taskbar

`taskbar_ac.exe` publishes live Win+1 through Win+10 mappings from `taskbar/applications/*.map`. `taskbar_builder.exe` creates missing program files and owns `keymap/components/taskbar.keymap_commands.txt`. `taskbar_config.exe` writes only `config/taskbar.ini`.

See [Taskbar component](docs/taskbar.md).

## Requirements

- Windows 11
- Visual Studio 2026 (version 18+) with the Desktop development with C++ workload (MSVC v145, C++23)
- Basic knowledge of C++
- The matching MSVC v145 redistributable if you run Release binaries you did not build on that PC

## Documentation

The documentation index is [docs/README.md](docs/README.md).

| Document | Topic |
| --- | --- |
| [Building](docs/building.md) | Solutions, MSBuild order, output directories |
| [Configuration](docs/configuration.md) | INI files, keymap, logging, copying `dist/` |
| [Development](docs/development.md) | Protocols, runtime commands, new components |
| [Adding a new component](docs/new-component.md) | File → New → Project, `AutoCore.props`, live list and keymap |
| [Main](docs/main.md) | `auto_core.exe` startup, hook, crash restart, shutdown |
| [Modules](docs/modules.md) | C++23 module catalog |
| [Contributing](CONTRIBUTING.md) | Clone, build, what not to commit |
| [Security](SECURITY.md) | Keyboard hook, secrets, reporting |

Product follow-ups are in [docs/TODO.md](docs/TODO.md). DLL-specific deferred work is in [app/core/TODO.md](app/core/TODO.md). Main-specific deferred work is in [app/main/TODO.md](app/main/TODO.md).

## History

Auto Core originally began as a Python project named Auto Song. The name was inspired by the original primary use case: formatting the currently playing song.

When the project was ported to C++, the program name changed to Auto Core to reflect the greater level of system control and precision tuning offered by C++.

## License

See [LICENSE](LICENSE). Third-party libraries are listed in [NOTICE.md](NOTICE.md).
