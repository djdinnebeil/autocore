# Auto Core for Windows 11

Auto Core is a modular **C++23 automation utility for Windows 11** built around configurable keyboard shortcuts. It combines numpad-driven commands, taskbar navigation, text insertion, journaling tools, media controls, and local system utilities.

The project is intended for developers who want to **build, run, or extend Auto Core from source**. It does not provide an installer; the supported workflow is to build a Release x64 distribution and run it from `dist/`.

> [!NOTE]
> Auto Core uses the Windows `SetWindowsHookEx` API to register a low-level keyboard hook (`WH_KEYBOARD_LL`) for processing configured keyboard inputs. Additional details are available in the [security documentation](docs/manual/security.md).

## Features

- **Keyboard automation:** Map numpad and other keys to configurable commands; mapped key presses are intercepted by the main application.
- **Taskbar navigation:** Activate applications pinned to Windows taskbar positions 1–10, with a configurable fallback executable.
- **Text insertion:** Insert text using the clipboard and `Ctrl + V`, including a mode for more sensitive text.
- **Journaling and writing:** Create journal entries, generate titles, manage notes and task lists, and produce randomized writing prompts.
- **Media integration:** Control iTunes and Spotify, including local listening-history functionality.
- **Local file access:** Serve local files through a loopback HTTP server for browser-based access.
- **System utilities:** Log wake events, manage recycle-bin tasks, and display application output in a console window.

## Requirements

- Windows 11
- Visual Studio **2026 (18+)**, with the **Desktop development with C++** workload (MSVC **v145**, C++23)
- Basic C++ knowledge for development and extension
- The matching **MSVC v145 redistributable** when running Release binaries built on another PC (`/MD` runtime)

## Build and run

1. Clone or copy the repository to a Windows 11 PC.
2. From the repository root, run the build and distribution script:

   ```powershell
   .\scripts\build-all.ps1
   ```

3. Launch the application from the generated distribution:

   ```powershell
   .\dist\app\auto_core.exe
   ```

4. Complete the first-run initialization when prompted, then configure your keyboard mappings and any components you intend to use.

The build script publishes executables and DLLs to `bin/` and assembles `dist/app/` with [`publish-dist.ps1`](scripts/publish-dist.ps1). The `dist/` directory contains local runtime state and is excluded from Git. See [build details](devs/building.md) for MSBuild order, output paths, and vendor DLL handling.

### First-run configuration

When `config/auto_core.ini` is missing, Auto Core launches `auto_core_init.exe` to initialize Main and component configuration, create missing starter data, and seed the keymap. The initialization process retains `auto_core.ini` as its completion marker only after required steps succeed; no separate `initialized` setting is used.

Common follow-up tasks:

- **Keymap:** Edit `dist/keymap.map`; use `keymap_editor.exe` if you need to create the initial map. Existing maps are not overwritten.
- **Taskbar:** Run `taskbar_builder.exe` when pinned applications or executable paths differ. See the [Taskbar manual](docs/manual/components/taskbar.md).
- **Component settings:** Run the relevant `<name>_config.exe` to create or update `config/components/<name>.ini`.
- **Spotify:** Authorize separately with `spotify_oauth.exe`. See the [Spotify manual](docs/manual/components/spotify.md).
- **Local server:** Restart `server_ac.exe` after changing its configuration, port, or document root.

If a component configuration file is missing during normal operation, that component reports the omission and uses its compiled defaults rather than writing a replacement. See the [configuration guide](docs/manual/configuration.md) and [configuration contract](devs/configuration.md).

### Running on another PC

Copy the complete `dist/` tree, install the matching MSVC redistributable if necessary, and review machine-specific settings. Reauthorize Spotify rather than copying `components/spotify/tokens.map`; adjust absolute logging paths and rebuild taskbar mappings as needed. Dash data is not portable between ordinary Windows installations.

## Architecture

Auto Core separates the **main controller**, **core DLL**, **shared protocols**, and **specialized component executables**. Components that communicate with the main process use named pipes through `auto_core.pipes`.

The main application registers the Windows low-level keyboard hook, passes configured key events to its message loop, and dispatches mapped commands. Component executables handle specialized functionality. See [Main architecture](devs/main.md), [development](devs/development.md), and the [module catalog](devs/modules.md).

### Components

| Component | Primary role | Main executable |
| --- | --- | --- |
| Dash | Protected local secret insertion using current-user DPAPI and a Windows Hello access check | `dash_ac.exe` |
| Journal | Journal titles, entries, and series workflows | `journal_ac.exe` |
| Logger | Combine component logs into a main log | `logger_ac.exe` |
| iTunes | Playback control through COM automation | `itunes_ac.exe` |
| Spotify | Web API playback control and listening history | `spotify_ac.exe` |
| Taskbar | Native taskbar activation and mappings | `taskbar_ac.exe` |
| Server | Loopback HTTP local-file server | `server_ac.exe` |
| Writer | Text insertion, notes, timestamps, and task lists | `writer_ac.exe` |
| Slash | Recycle-bin utility | `slash_ac.exe` |
| Wake | System-resume logging | `wake_ac.exe` |

The repository also contains dedicated configuration, editor, builder, authorization, and database executables. These tools own their respective settings and data files; for example, the Journal tools manage alias lists, its series database, mappings, and optional cloud configuration. Consult the [user manual](docs/Auto%20Core%20Manual.md) and [developer documentation](devs/README.md) for component-specific operations.

## Repository layout

```text
Auto Core/
├─ app/             C++ source, project files, resources, and shared code
│  ├─ core/         Core DLL
│  ├─ main/         Main runtime, editors, configuration, and shared defaults
│  ├─ components/   Specialized component projects
│  └─ shared/       Shared protocols and command registration
├─ scripts/         Build and distribution scripts
├─ msbuild/         Shared Visual Studio project settings
├─ third_party/     Bundled third-party dependencies
├─ devs/           Developer documentation
├─ docs/           User guides and component manuals
├─ bin/            Published executables and DLLs
├─ lib/            Canonical core DLL and import library
├─ obj/            Intermediate build files (Git-ignored)
├─ symbols/        Linker debug symbols (Git-ignored)
└─ dist/           Assembled runtime and local state (Git-ignored)
```

Use [`msbuild/AutoCore.props`](msbuild/AutoCore.props) for shared project settings. Local overrides (`*.local.ini` and `*.local.ixx`) are not committed. Crash reports, when enabled, are saved locally under `dist/data/crash_reports/` and are **not automatically uploaded**.

## Documentation

| Resource | Contents |
| --- | --- |
| [Auto Core Guide](docs/Auto%20Core%20Guide.md) | Getting started and everyday use |
| [Auto Core Tutorial](docs/Auto%20Core%20Tutorial.md) | Guided setup and workflows |
| [Auto Core Manual](docs/Auto%20Core%20Manual.md) | Detailed user reference |
| [Security](docs/manual/security.md) | Keyboard hook, protected data, and reporting |
| [Configuration](docs/manual/configuration.md) | User configuration |
| [Building](devs/building.md) | Build pipeline and dependency handling |
| [Development](devs/development.md) | Components, protocols, and keymap registration |
| [Adding a component](devs/new-component.md) | Creating and wiring new component projects |
| [Contributing](devs/contributing.md) | Development and contribution practices |

Additional references include [developer configuration](devs/configuration.md), [modules](devs/modules.md), and the [development TODO](devs/TODO.md).

## Project history

Auto Core began as a Python application named **Auto Song**, initially focused on formatting the currently playing song. It was later ported to C++ and renamed **Auto Core** to reflect its broader automation scope and low-level system integration.

## License

See [LICENSE](LICENSE) for license terms and [NOTICE.md](NOTICE.md) for third-party attributions.
