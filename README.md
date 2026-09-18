# Auto Core for Windows 11

Auto Core is a C++23 automation utility for Windows 11. Its core feature is a specialized keyboard manager for numpad and system-level shortcuts, supporting quick taskbar access, automated text insertion, journaling workflows, and custom command execution.

Auto Core also includes component-based automation outside the keyboard layer, including music listening history, local file management, and system event logging.

This repository is for people who want to **build or extend** the source and **run** the `dist/` tree they built. Beta testers are developers: clone or copy the tree, build Release x64, then start `dist\auto_core.exe`. There is no installer.

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

Build from source (below), then run from `dist/` (`auto_core.exe`, `auto_core.dll`, vendor runtime DLLs, and the component executables). Projects use the DLL CRT (`/MD`); if you run binaries you did not build on that PC, install the matching **MSVC v145** redistributable.

1. Start `auto_core.exe` from `dist`. Missing `dist/config/` files (`*.ini` and `components.list`) are written once from portable defaults (`auto_core.ini` is not fatal). Edit the live files as needed. Tracked samples in [`defaults/`](defaults/) are documentation only; Auto Core never reads them.
2. Edit `dist/keymap/bindings.ini` if you want a custom map. If the file is missing, Auto Core writes a seed map on first start and does not overwrite an existing file. [`defaults/keymap/bindings.ini`](defaults/keymap/bindings.ini) is a sample to copy by hand, not a file the program loads.
3. Run `taskbar_config.exe` from `dist` so `taskbar/applications/*.ini` matches the programs you pin. See [Taskbar](docs/taskbar.md).
4. Optional helpers (or let Main seed the INI): `spotify_oauth.exe` ([Spotify](docs/spotify.md)), `journal_config.exe` (episode counters in `journals.db` under `[journal] directory`), `writer_config.exe` ([Writer config](docs/writer_config.md)), `server_config.exe` ([Server config](docs/server_config.md)). Restart `server_ac.exe` after rewriting `server.ini`.
5. Windows may prompt for elevation; that is required for some iTunes setups.

`dist/` is gitignored. Tracked portable defaults live in [`defaults/`](defaults/) (repo `defaults/X` is the sample for runtime `dist/X`). `*.local.ini` / `*.local.ixx` also stay off git. See [configuration](docs/configuration.md).

To run the same build on another Windows 11 PC, copy the `dist/` folder, then retune machine-specific files: install the **MSVC v145** redistributable if that PC did not build the binaries; do not copy `spotify/spotify_tokens.ini` (run `spotify_oauth.exe` there); retarget `config/logger.ini` if `directory` is an absolute path; re-run `taskbar_config.exe` when pins or exe paths differ. Dash is not portable between Windows installs. Details are in [configuration](docs/configuration.md#copying-dist-to-another-windows-11-pc).

## Building

1. Clone or copy the repository onto Windows 11.
2. Install Visual Studio **2026** (version **18+**) with the Desktop development with C++ workload (MSVC **v145**, C++23).
3. From the repository root, run [`scripts/build-all.ps1`](scripts/build-all.ps1), or follow the MSBuild order in [Building](docs/building.md). After Link, Windows PowerShell runs [`scripts/copy-dist-dlls.ps1`](scripts/copy-dist-dlls.ps1), which copies vendor runtime DLLs from `third_party/*/bin/` and `lib/auto_core.dll` into `dist/`.
4. Shared paths live in [`msbuild/AutoCore.props`](msbuild/AutoCore.props) (repo-relative; a clone does not edit that file).

The linker searches `lib/auto_core.lib`. `dist/` is gitignored.

## Extending

New components, shared protocols, and keymap registration are documented in [Development](docs/development.md). Creating a child from File → New → Project is [Adding a new component](docs/new-component.md). The module catalog is [Modules](docs/modules.md). Contribution mechanics (tests, what not to commit) are in [CONTRIBUTING.md](CONTRIBUTING.md).

Person-name journal aliases live in `journal_choices.ini` under the configured journal data directory (default `dist/journal/`). A missing live file is written once from [`defaults/journal/journal_choices.ini`](defaults/journal/journal_choices.ini). An existing file is never overwritten. Unused aliases are not bound; `bindings.ini` is the filter.

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
| `journal` | Journaling titles and file workflow | `journal_ac.exe` | Pipe child of Auto Core; episode counters in `journals.db` under `[journal] directory` |
| `journal_config` | Journal database setup | `journal_config.exe` | Writes `config/journal.ini` if missing; creates `journals.db` and series tables; on-demand like `taskbar_config` |
| `itunes` | iTunes controller | `itunes_ac.exe` | Dedicated-owner-thread COM automation; see [iTunes](docs/itunes.md) |
| `logger` | Central component logger | `logger_ac.exe` | Receives component log events over named pipes |
| `server` | Local file server | `server_ac.exe` | Loopback HTTP file server |
| `server_config` | Server INI setup | `server_config.exe` | Writes `config/server.ini` if missing; menu get/set for `port` and `document_root` |
| `simple_test` | Generic host smoke test | `simple_test_ac.exe` | New-child discovery via live `components.list`; see [Development](docs/development.md#generic-host-smoke-test-simple_test) |
| `slash` | Recycle bin utility | `slash_ac.exe` | Prints deleted items |
| `spotify` | Spotify controller | `spotify_ac.exe` | Web API playback control and local history; see [Spotify](docs/spotify.md) |
| `spotify_oauth` | Spotify authorization helper | `spotify_oauth.exe` | Handles the OAuth authorization flow |
| `taskbar` | Native taskbar activation | `taskbar_ac.exe` | Snapshot authority for Win+position mappings; see [Taskbar](docs/taskbar.md) |
| `taskbar_config` | Taskbar INI generator | `taskbar_config.exe` | Creates missing `taskbar/applications` files and rewrites the taskbar keymap manifest |
| `wake` | System wake tracker | `wake_ac.exe` | Logs resume timestamps |
| `writer` | Text insertion and notes | `writer_ac.exe` | Pipe child of Auto Core; notepad, timestamps, and task list |
| `writer_config` | Writer data-directory setup | `writer_config.exe` | Writes `config/writer.ini` if missing; stub menu for paths and empty `gpt_prompts.txt` / `task_list.txt` |

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
├─ scripts/             Repo build and post-Link copy scripts
├─ msbuild/             AutoCore.props (one level below repo root; not obj/)
├─ app/                 Build input (source, projects, resources). Not source-only
│  ├─ components/       Child executables that ship in dist/ (unit tests nested)
│  ├─ core/             auto_core.dll source, include/ac_api.hpp, nested tests
│  ├─ main/             auto_core.exe project
│  ├─ resources/        Shared .ico and .rc files
│  └─ shared/           Compile-time IPC protocols and command_registry
├─ defaults/            Tracked samples for dist/X (never loaded at runtime)
│  ├─ config/           Samples for dist/config/
│  ├─ keymap/           Sample bindings.ini
│  ├─ journal/          Sample journal_choices.ini
│  └─ server/           Shipped document root (copied to dist/server on build)
├─ dist/                Gitignored assembled runtime for end users
│  ├─ config/           Live configuration files
│  ├─ keymap/           bindings.ini, keymap_commands.txt, and components/ catalogs
│  ├─ journal/          Journal aliases and journals.db (local)
│  ├─ notes/            Dated Writer notes (local)
│  ├─ server/           Local server document root
│  ├─ spotify/          Spotify tokens, codes, and listening history (local)
│  ├─ taskbar/          Per-program INIs in applications/; cached_positions.ini
│  ├─ writer/           gpt_prompts.txt and task_list.txt (local)
│  └─ symbols/          Debug symbol files, such as .pdb files
├─ docs/                Developer documentation
├─ lib/                 Canonical auto_core.dll and auto_core.lib
├─ obj/                 Gitignored IntDir, including test binaries
└─ third_party/         Committed vendors (`<dependency>/`; product `include`, `lib`, `bin`; Catch2 at `catch2/`)
```

`dist/` is gitignored runtime state. See [configuration](docs/configuration.md).

Build artifacts are generated under `obj/` (intermediates), `lib/` (canonical `auto_core.lib` and `auto_core.dll`), and `dist/` (executables and the runtime `auto_core.dll`). Shared Visual Studio settings live in `msbuild/`. Wipe `obj/` anytime; rebuilding the core DLL overwrites `lib/` in place.

## Dash

Dash stores and inserts revocable, noncritical secrets without plaintext secret files. Auto Core launches it with the `launch_dash` keymap command. Dash is not a general password manager, and its vault is not portable between ordinary Windows installations.

See [Dash secret storage](docs/dash.md) for the contract, threat model, storage location, recovery policy, and disclaimers.

## iTunes

The iTunes component controls playback and records listening history through the iTunes COM automation interface. It runs as `itunes_ac.exe`.

> [!IMPORTANT]
> An elevated Auto Core process cannot connect to an iTunes instance that is already running without Administrator privileges. Either start Auto Core before iTunes, or close iTunes and restart it with Administrator privileges before starting Auto Core.

See [iTunes component](docs/itunes.md). Follow-up work is in [docs/TODO.md](docs/TODO.md).

## Spotify

The Spotify component runs as `spotify_ac.exe` and provides Web API playback control, queue and current-track formatting, playback-device transfer, album art download, and a local listening-history database.

See [Spotify component](docs/spotify.md).

## Taskbar

`taskbar_ac.exe` publishes live Win+1 through Win+10 mappings from `taskbar/applications/*.ini`. `taskbar_config.exe` creates missing program files and rewrites `keymap/components/taskbar.keymap_commands.txt`.

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
| [Configuration](docs/configuration.md) | INI files, `defaults/`, keymap, logging, copying `dist/` |
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
