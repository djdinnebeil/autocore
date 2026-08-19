# Auto Core for Windows 11

Auto Core is a C++23 automation utility for Windows 11. Its core feature is a specialized keyboard manager for numpad and system-level shortcuts, supporting quick taskbar access, automated text insertion, journaling workflows, and custom command execution.

Auto Core also includes component-based automation outside the keyboard layer, including music listening history, local file management, and system event logging.

> [!NOTE]
> Auto Core is designed specifically for Windows 11. Some workflows depend on Windows-specific shortcuts, such as `Win + 0` through `Win + 9` for taskbar navigation. The overall architecture could be adapted to other desktop environments, but platform-specific behavior would need replacement implementations.

---

## Contents

- [Main Features](#main-features)
- [Project Architecture](#project-architecture)
- [Project Folder Structure](#project-folder-structure)
- [Runtime Configuration](#runtime-configuration)
- [Keymap Commands](#keymap-commands)
- [Registering Runtime Commands](#registering-runtime-commands)
- [Adding a New Component](#adding-a-new-component)
- [Spotify Component](#spotify-component)
- [Runtime Performance](#runtime-performance)
- [Modules Overview](#modules-overview)
- [Requirements](#requirements)
- [Installation and Setup](#installation-and-setup)
- [Project Notes](#project-notes)
- [Changelog](#changelog)
- [License](#license)

---

## Main Features

- **Numpad Keyboard Manager** — Monitors and intercepts numpad and additional keys to execute configured tasks.
- **System Console Window** — Uses the system console to display output and request input.
- **Automated Text Insertion** — Inserts text into the active text box by using the system clipboard and the `Ctrl + V` paste shortcut.
- **Taskbar PowerShell** — Simplifies quick taskbar access by sending Windows key shortcuts such as `Win + 0` through `Win + 9`.
- **Music Player Integration** — Provides integration points for iTunes and Spotify.
- **Journaling Support** — Automates title generation and file creation for journaling workflows.
- **Creative Writing Inspiration** — Uses dice-roll-style randomization to generate numbers or prompts for writing inspiration.
- **Browser-Based Local File Management** — Runs a local server to support browser-based access to local files.
- **System Maintenance** — Supports maintenance tasks such as emptying the recycle bin and logging system wake events.

---

## Project Architecture

Auto Core uses a modular component architecture that separates the main system controller from specialized component executables.

The main application, core DLL, shared protocols, and component projects are separated by responsibility. Components that need to communicate with the main application use named pipes, with IPC support provided by the `auto_core.pipes` module.

### Components

| Component | Purpose | Executable | Notes |
| --- | --- | --- | --- |
| `itunes` | iTunes controller | `itunes.exe` | Uses COM |
| `logger` | Central component logger | `logger.exe` | Receives component log events over named pipes |
| `server` | Local file server | `server.exe` | Simple HTTP server |
| `slash` | Recycle bin utility | `slash.exe` | Prints deleted items |
| `sp` | Spotify controller | `sp.exe` | Stores play history |
| `sp_oauth` | Spotify authorization helper | `sp_oauth.exe` | Handles the OAuth authorization flow |
| `wake` | System wake tracker | `wake.exe` | Logs resume timestamps |

### Component Naming Conventions

Auto Core uses suffixes to distinguish component scripts, component classes, and component threads from main application code.

| Suffix | Meaning |
| --- | --- |
| `_x` | External component file or module |
| `_c` | Component class |
| `_t` | Component thread |

For example, the main application uses `sp.cxx`, while the Spotify component exports its interface from `sp_x.ixx`.

---

## Project Folder Structure

```text
Auto Core/
├─ app/                 Application source, project files, resources, and build configuration
│  ├─ build/            Shared Visual Studio build configuration, including AutoCore.props
│  ├─ components/       Component executable projects
│  ├─ core/             Auto Core DLL project and core library code
│  ├─ main/             Main executable project
│  ├─ resources/        Application resources, such as .ico and .rc files
│  ├─ shared/           Shared modules and code used across app projects
│  └─ tools/            Developer-facing tools for expanding project functionality
├─ dist/                Portable runtime output distributed with Auto Core
│  ├─ config/           Configuration files
│  ├─ keymap/           Keymap configuration files
│  ├─ link/             Shortcut, link, or launch-related files
│  ├─ notepad/          User-custom strings or notepad-related runtime data
│  ├─ server/           Local server setup files
│  ├─ star/             User-specific runtime data, such as login tokens and journal databases
│  ├─ sun/              Taskbar shortcut files
│  └─ symbols/          Debug symbol files, such as .pdb files
├─ docs/                Project documentation
├─ lib/                 External libraries and auto_core.lib
├─ tests/               Testing scripts and test projects
└─ readme.md            Main project overview and setup notes
```

The `dist/crash/` directory is created only when Auto Core records a crash. If `crash.log` exists, Auto Core reports the previous crash to the user.

### Outside the Project Folder

```text
log/                    Debugging and operational logging
```

Build artifacts are generated outside the project folder. The `app/build/` folder contains source-controlled build configuration, not compiler output.

Keeping logs outside the project folder helps preserve portability and prevents local runtime output from being mixed with source-controlled project files.

---

## Runtime Configuration

Runtime configuration files belong in `dist/config`. These files control runtime behavior, logging, taskbar mappings, local server settings, journaling settings, and optional runtime key mapping.

| File | Purpose |
| --- | --- |
| `app.ini` | Controls the program title. |
| `itunes.ini` | Controls whether iTunes starts automatically and the final tab copied when reading track data. |
| `journal.ini` | Defines the journal day-rollover hour. |
| `keymap_mode.ini` | Selects the compiled or runtime keymap. |
| `logger.ini` | Enables logging, controls console forwarding, and optionally selects the log directory. |
| `server.ini` | Stores local web server settings, such as the port number. |
| `star.ini` | Stores journal-related settings. |
| `taskbar.ini` | Lists the first 10 programs pinned to the user’s taskbar. |

---

## Keymap Commands

The `keymap` folder contains the runtime keymap workspace. Auto Core creates missing workspace files from compiled defaults.

| File | Purpose |
| --- | --- |
| `keymap_commands.txt` | Generated list of registered command expressions for editor autocomplete. |
| `keymap.ini` | Active key-to-command mappings used in runtime mode. |
| `keymap_settings.ini` | Controls runtime keymap settings such as trace logging. |

Set `mode = runtime` in `dist/config/keymap_mode.ini` to use `keymap.ini`. Otherwise, Auto Core uses the compiled keymap.

---

## Shared Folder

The `shared` folder contains protocols shared by the main application and component executables. Reusable runtime facilities such as paths, logging, and named pipes are provided by the core DLL under `app/core`.

| Folder or Module | Purpose |
| --- | --- |
| `wake_protocol.ixx` | Defines wake-component messages. |
| `spotify_protocol.ixx` | Defines Spotify-component messages. |
| `itunes_protocol.ixx` | Defines iTunes-component messages. |

---

## Registering Runtime Commands

To make a function available to the runtime keymap, register it with the appropriate `runtime_commands::register_with` function. Auto Core refreshes `dist/keymap/keymap_commands.txt` from the command registry when it starts in runtime mode.

Runtime key mapping must be selected in `dist/config/keymap_mode.ini` for mappings in `dist/keymap/keymap.ini` to take effect.

---

## Adding a New Component

To add a new component to Auto Core, create the project inside the `app/components` directory.

### 1. Create the Component Project

Create the new Visual Studio project under:

```text
app/components/<component-name>/
```

Use the existing component projects as templates when possible.

### 2. (Optional) Create a Symbolic Link for `.vs`

The `.vs` directory for a project can be replaced with a symbolic link that points to `$(AutoCoreBuildDir)`.

**Command Prompt:**

```cmd
cd "$(SolutionDir)"
mklink /D .\.vs "$(AutoCoreBuildDir)"
```

> `$(SolutionDir)` and `$(AutoCoreBuildDir)` are Visual Studio macros. Replace them with their resolved paths when running the command manually.

### 3. Add the Auto Core Property Sheet

Open Property Manager in Visual Studio:

```text
View → Other Windows → Property Manager
```

In Property Manager:

1. Expand the new project.

2. Expand the configuration and platform that you want to configure.

3. For the production build, select:

   ```text
   Release | x64
   ```

4. Right-click the configuration and select:

   ```text
   Add Existing Property Sheet
   ```

5. Select the following property sheet:

   ```text
   app\build\AutoCore.props
   ```

Repeat these steps for any additional configuration and platform combinations that should use the shared Auto Core settings.

### 4. Configure Visual Studio Project Settings

Set the project output directory:

```text
$(AutoCoreDistDir)
```

Set the intermediate directory:

```text
$(AutoCoreBuildDir)$(TargetName)\
```

Set the additional library directory:

```text
$(AutoCoreLibDir)
```

Add the Auto Core DLL import library dependency:

```text
auto_core.lib
```

In Visual Studio, this corresponds to:

```text
Linker → General → Additional Library Directories
Linker → Input   → Additional Dependencies
```

### 5. Reference Shared Resources

If your component uses a resource image or manifest, reference the shared resource file:

```text
..\..\resources\resource.rc
```

---

## Spotify Component

The Spotify component provides music integration for Spotify. Because Spotify history data is not immediately available from Spotify itself, the component maintains a local database of the user’s listening history.

### Upsert

The Spotify component uses an upsert-style workflow to process listening history efficiently as the database grows. This allows existing rows to be updated while new rows are inserted without requiring a full rebuild of the local history store.

---

## Runtime Performance

Auto Core supports compiled and runtime keymaps. The compiled keymap is the default. Runtime mode loads command mappings from `dist/keymap/keymap.ini`; optional trace logging is controlled by `dist/keymap/keymap_settings.ini`.

---

## Modules Overview

For a detailed description of each module, refer to the generated Doxygen documentation:

```text
./docs/html/files.html
```

---

## Requirements

- Windows 11
- C++ compiler with C++23 support
- Visual Studio or another compatible C++ development environment
- Basic knowledge of C++

---

## Installation and Setup

Auto Core is currently intended for developer-configured use rather than end-user distribution. Installation and usage require knowledge of C++ and the Windows development environment.

Recommended setup flow:

1. Clone or copy the project into a local development directory.
2. Open the solution or project files in Visual Studio.
3. Confirm the shared build properties in `app/build/AutoCore.props`.
4. Build the main executable and any required component executables.
5. Place runtime files under `dist`.
6. Keep machine-specific logs and user-specific data outside source-controlled project files.

---

## Project Notes

### Logging and Local Data

Runtime logs should remain outside the source-controlled project folder whenever possible. This keeps the project portable and avoids committing machine-specific or user-specific runtime output.

### Runtime Data Safety

The `dist/star/` folder may contain user-specific runtime data such as login tokens or journal databases. Treat this folder as local runtime state rather than portable project source.

### Technical Debt

Low-priority cleanup items, such as deprecated but harmless coding styles, should be tracked in an issue tracker or a project-level TODO file rather than emitted as runtime log events.

---

## Changelog

Auto Core originally began as a Python project named Auto Song. The name was inspired by the original primary use case: formatting the currently playing song.

When the project was ported to C++, the program name changed to Auto Core to reflect the greater level of system control and precision tuning offered by C++.

With the upgrade to C++23 modules, Auto Core branched off into two projects: Auto Core and Visual Core. Auto Core focuses on improving and fine-tuning the program core, while Visual Core is dedicated to developing and optimizing separate components, such as music player and cloud components.

---

## License

License © 2026 DJ, Daniel

Anyone is free to use, copy, and distribute Auto Core. This software is expected to be modified and configured by the individual user. Contributions to the Auto Core project can be made through the Visual Core project.

### Disclaimer

This software depends on the proper functionality and configuration of Windows 11. The developers disclaim responsibility for failures, inaccuracies, or other issues that may arise from malfunctions or misconfigurations of Windows 11.
