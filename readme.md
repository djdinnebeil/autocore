# Auto Core for Windows 11

Auto Core is a C++ automation utility for Windows 11. Its core feature is a specialized keyboard manager for numpad and system-level shortcuts, supporting quick taskbar access, automated text insertion, journaling workflows, and custom command execution.

Auto Core also includes component-based automation outside the keyboard layer, such as music listening history, local file management, and system event logging.

Although Auto Core is designed specifically for Windows 11, many of its concepts could be adapted to other desktop environments. Some features take advantage of Windows-specific shortcuts, such as `Win + 0–9` for taskbar navigation, so a port to another system would require platform-specific replacements for those workflows.

---

## Main Features

- **Numpad Keyboard Manager:** Monitors and intercepts numpad and additional keys to carry out tasks.
- **System Console Window:** Uses the system console to display output or request input.
- **Automated Text Insertion:** Automates text insertion into the active textbox by using the system clipboard and the `Ctrl + V` paste shortcut.
- **Taskbar PowerShell:** Simplifies quick taskbar access by sending Windows key shortcuts, such as `Win + 0-9`, with additional support for taskbar productivity.
- **Music Player Integration:** Supports music player integration for iTunes and Spotify.
- **Journaling Support:** Automates titling and file creation for journaling documents.
- **Creative Writing Inspiration:** Uses a dice-roll style method to generate random numbers or prompts for journaling inspiration.
- **Browser-Based Local File Management:** Runs a local server to support browser-based local file access.
- **System Maintenance:** Supports maintenance tasks, including emptying the recycle bin and logging system wake events.

---

## Project Architecture

Auto Core uses a modular component architecture that separates the main system controller from specialized component executables.

The main application source lives in `src`, shared modules and the Auto Core runtime library live in `shared`, and component projects live in `core`. Components communicate with the main application through named pipes. Core components that need IPC support use the shared `pipes_x.ixx` module.


| Component | Purpose                                 | Executable Name | Notes |
|-----------|-----------------------------------------|------------------|-------|
| `dash`    | Runtime mapping and developer overlay   | `dash_x.exe`     | Supports IntelliSense-assisted runtime mapping |
| `itunes`  | Controls iTunes                         | `itunes.exe`     | Uses COM |
| `server`  | Hosts a local file server               | `server.exe`     | Simple HTTP server |
| `slash`   | Empties the recycle bin                 | `slash.exe`      | Prints deleted items |
| `sp`      | Tracks Spotify history                  | `sp.exe`         | Stores play history |
| `wake`    | Tracks system wake events               | `wake.exe`       | Logs resume timestamps |


### Component Naming Conventions

Auto Core uses suffixes to distinguish component scripts, component classes, and component threads from main application code.

| Suffix | Meaning |
| --- | --- |
| `_x` | External component file or module |
| `_c` | Component class |
| `_t` | Component thread |

For example, the main application may use `sp.cxx`, while the Spotify component uses `sp_x.cxx`.

---

## Recommended Folder Structure

```text
Auto Core/
├─ assets/             Graphical resources, manifests, icons, and shared resource files.
├─ build/              CMake configuration and build-generation files.
├─ core/               Core Auto Core components.
├─ dist/               Portable runtime output, including `.exe`, `.dll`, and user-facing runtime files.
│  ├─ config/          Configuration files.
│  ├─ dash/            Runtime configuration and developer-facing runtime mapping files.
│  ├─ notepad/         User-custom strings.
│  ├─ server/          Local server setup files.
│  ├─ sun/             Taskbar shortcut files.
│  ├─ star/            User-specific runtime data, such as login tokens and journal databases.
│  └─ visual/          Runtime debugging symbols, such as copied `.pdb` files.
├─ docs/               Project documentation.
├─ import/             Main program modules.
├─ lib/                External libraries and `auto_core.lib`.
├─ shared/             Source code for `auto_core.lib` and shared component modules.
├─ src/                Main application source code.
├─ tests/              Testing scripts and test projects.
├─ tools/              Developer-facing utilities for expanding project functionality.
├─ utils/              Helper scripts, such as scripts for creating a new journaling database.
└─ visual/             Visual Studio-related files, such as `.sln` files and development-time debug files.
```

### Outside the Project Folder

```text
log/                   Debugging and operational logging.
```

Keeping logs outside the project folder helps preserve portability and prevents local runtime output from being mixed with source-controlled project files.

---

## Architecture Notes

The current structure is a strong foundation for a portable Windows utility project. It separates development files from runtime files and keeps the portable application package centered around `dist`.

| Area | Recommendation |
| --- | --- |
| `dist` | Treat this as the portable application package. Anything required to run the app belongs here. Source files and development-only files should remain outside it. |
| `dist/star` | Treat login tokens, journal databases, and other user-specific files as runtime data. Do not commit real user data to source control. Include only templates or placeholder files when needed. |
| `build` | If this folder contains source-controlled CMake files, the name is acceptable. If it also contains generated output, keep generated files under a clear subfolder such as `build/obj/`. |
| `lib` | Consider separating generated Auto Core libraries and third-party libraries if the project grows, such as `lib/auto_core/` and `lib/external/`. |
| `tools` and `utils` | Keep both if they serve different purposes. Use `tools` for developer-facing utilities and `utils` for smaller helper scripts used by the application or build process. |
| `visual` | Keeping Visual Studio files in a dedicated folder is reasonable. Generated `.pdb` files should stay out of source control unless intentionally copied into `dist/visual` for runtime debugging. |

The most important rule is to keep source files, build artifacts, runtime distribution files, logs, and user-specific data clearly separated.

---

## Runtime Configuration

Runtime configuration files belong in `dist/config`. These files control runtime behavior, logging, taskbar mappings, local server settings, journaling settings, and optional runtime key mapping.

| File | Purpose |
| --- | --- |
| `clock.ini` | Defines `end_of_day`, such as `00:59` or `24:59`. |
| `itunes.ini` | Defines the number of tabs to copy from iTunes. |
| `logger.ini` | Enables enhanced debugging by forwarding log statements to the console window. |
| `runtime.ini` | Controls whether runtime configuration is enabled and selects logging behavior. |
| `runtime_map.ini` | Stores runtime key mappings used when runtime configuration is enabled. If runtime mapping is disabled, mappings are hardcoded and this file is ignored. |
| `server.ini` | Stores local web server settings, such as the port number. |
| `star.ini` | Stores journal-related settings. |
| `taskbar.ini` | Lists the first 10 programs pinned to the user's taskbar. |

---

## Dash Runtime Mapping

The Dash folder acts as a convenience layer for modifying runtime mappings.

| File | Purpose |
| --- | --- |
| `dash_x.ini` | A hard link to `dash_x.ixx`. Contains functions tagged with `\runtime`. When opened in VS Code, this makes runtime functions globally accessible for IntelliSense autocomplete. |
| `runtime_map.ini` | A hard link to `dist/config/runtime_map.ini`. Contains the active key-to-function mappings defined by the user. |

This setup enables VS Code to provide autocomplete for user-defined runtime functions by opening `dash_x.ini`, while still linking directly to the active runtime mapping file.

---

## Shared Folder

The `shared` folder contains source code for the Auto Core shared runtime library and component-shared modules.

| Folder or Module | Purpose |
| --- | --- |
| `dll_source_code` | Contains source code for `auto_core.lib` and the runtime `.dll`. |
| `core_runtime` | Contains shared header files and shared modules. Modules shared through hard links are marked with `\hardlink`. |
| `pipes_x` | Contains the pipe module used by components that need IPC. |

---

## Tagging Runtime Functions

To make a function available for runtime configuration, tag it with `\runtime`.

After tagging the function, run `dash_x.exe` to update the runtime function list. This ensures the new function is recognized and available for use in `runtime_map.ini`.

Runtime configuration must be enabled in `dist/config/runtime.ini` for runtime mappings to take effect.

---

## Adding a New Component

To add a new component to Auto Core, create the project inside the `core` directory.

Auto Core should avoid hardcoded absolute paths such as `C:\Users\Name\...` or `C:\DJ\...`. Instead, component projects should use the `AUTOCORE_BUILD_DIR` environment variable.

---

## 1. Set the Auto Core Environment Variable

Before configuring Visual Studio, define an environment variable that points to the Auto Core project directory.

For example:

```text
%AUTOCORE_BUILD_DIR% = %USERPROFILE%\Project Files\Auto Core
```

This allows project settings to reference Auto Core paths without depending on a specific username or drive location.

### Option A: Set the Variable from Command Prompt

Open Command Prompt and run:

```cmd
setx AUTOCORE_BUILD_DIR "%USERPROFILE%\Project Files\Auto Core"
```

After running this command, close and reopen Visual Studio so it can detect the new environment variable.

To confirm the value was saved, open a new Command Prompt and run:

```cmd
echo %AUTOCORE_BUILD_DIR%
```

You should see the full path to your Auto Core directory.

### Option B: Set the Variable in Windows Settings

You can also set the variable through the Windows user interface:

1. Open **Start** and search for **Environment Variables**.
2. Select **Edit the system environment variables**.
3. Click **Environment Variables**.
4. Under **User variables**, click **New**.
5. Set the variable name to:

   ```text
   AUTOCORE_BUILD_DIR
   ```

6. Set the variable value to your Auto Core path, for example:

   ```text
   %USERPROFILE%\Project Files\Auto Core
   ```

7. Click **OK** to save the changes.
8. Restart Visual Studio.

---

## 2. Use the Variable in Visual Studio

Windows Command Prompt references environment variables using `%VARIABLE_NAME%`.

Visual Studio project settings should reference environment variables using MSBuild macro syntax:

```text
$(AUTOCORE_BUILD_DIR)
```

For example, if this environment variable is set:

```text
AUTOCORE_BUILD_DIR=%USERPROFILE%\Project Files\Auto Core
```

Visual Studio can reference the shared library folder with:

```text
$(AUTOCORE_BUILD_DIR)\lib\
```

---

## 3. Visual Studio Project Settings

Update the following settings for the component project.

### General

#### Output Directory

```text
$(AUTOCORE_BUILD_DIR)\dist\
```

This places compiled output files in the portable Auto Core `dist` directory.

#### Intermediate Directory

```text
$(AUTOCORE_BUILD_DIR)\build\obj\$(ProjectName)\
```

This stores temporary build files under `build\obj`, separated by project name.

### Linker → General

#### Additional Library Directories

```text
$(AUTOCORE_BUILD_DIR)\lib\
```

This allows the component project to locate Auto Core library files without using a hardcoded absolute path.

### Linker → Input

#### Additional Dependencies

```text
auto_core.lib;
```

This links the component against the shared Auto Core runtime library.

---

## 4. Resource File

If the component uses a shared resource image, manifest, or other resource file, reference the shared resource file with the environment variable:

```text
$(AUTOCORE_BUILD_DIR)\assets\resource.rc
```

This keeps the resource path portable and consistent across projects.

---

## 5. Optional: Create a Symbolic Link for Visual Studio

If Visual Studio needs access to the Auto Core directory from another solution folder, create a symbolic link.

Open Command Prompt from the solution directory and run:

```cmd
mklink /D ".\AutoCore" "%AUTOCORE_BUILD_DIR%"
```

> Note: Creating symbolic links may require Command Prompt to be run as Administrator, unless Developer Mode is enabled in Windows.

After creating the link, Visual Studio can reference the linked folder with paths such as:

```text
$(SolutionDir)AutoCore\lib\
```

Using `$(AUTOCORE_BUILD_DIR)` directly is preferred when the project should not depend on a symbolic link.

---

## 6. Recommended Component Configuration Summary

| Setting | Value |
| --- | --- |
| Environment Variable | `AUTOCORE_BUILD_DIR=%USERPROFILE%\Project Files\Auto Core` |
| Output Directory | `$(AUTOCORE_BUILD_DIR)\dist\` |
| Intermediate Directory | `$(AUTOCORE_BUILD_DIR)\build\obj\$(ProjectName)\` |
| Additional Library Directories | `$(AUTOCORE_BUILD_DIR)\lib\` |
| Additional Dependencies | `auto_core.lib;` |
| Shared Resource File | `$(AUTOCORE_BUILD_DIR)\assets\resource.rc` |

---

## Troubleshooting Component Builds

### Visual Studio does not recognize `$(AUTOCORE_BUILD_DIR)`

Close and reopen Visual Studio after setting the environment variable. Visual Studio reads environment variables when it starts.

### The build cannot find `auto_core.lib`

Verify that the library exists here:

```text
%AUTOCORE_BUILD_DIR%\lib\auto_core.lib
```

Also confirm that **Linker → General → Additional Library Directories** is set to:

```text
$(AUTOCORE_BUILD_DIR)\lib\
```

### The resource file cannot be found

Verify that the shared resource file exists here:

```text
%AUTOCORE_BUILD_DIR%\assets\resource.rc
```

Then confirm the project references:

```text
$(AUTOCORE_BUILD_DIR)\assets\resource.rc
```

---

## Spotify Component

The Spotify component provides music integration for Spotify. Because Spotify history data is not immediately available from Spotify itself, the component maintains a local database of the user's listening history.

### Upsert

Upsert is used to enable efficient processing as the database grows.

---

## Runtime Performance

The runtime behavior of Auto Core can be adjusted by modifying `dist/config/runtime.ini`. There are four available levels, each providing a tradeoff between observability and performance.

| Runtime Mode | Description | Required Settings |
| --- | --- | --- |
| `debug` | Enables full runtime debugging. | `runtime_enabled=true`, `runtime_debugger=true` |
| `buffer` | Enables buffered logging for runtime events. | `runtime_enabled=true`, `runtime_logger=true` |
| `silence` | Disables logging but keeps runtime mapping active. | `runtime_enabled=true`, `runtime_logger=false` |
| `disabled` | Disables runtime mapping entirely. All mappings must be hardcoded. | `runtime_enabled=false` |

### Performance Benchmarks

Benchmarks as of April 13, 2025:

| Runtime Mode | Average Time per Call |
| --- | --- |
| `debug` | 230-250 µs |
| `buffer` | 105-130 µs |
| `silence` | 75-95 µs |
| `disabled` | 0.20-0.35 µs |

These benchmarks represent the overhead added by runtime configuration handling. Disabling runtime yields the highest performance and is recommended for production builds where configurability is not needed.

---

## Modules Overview

For a detailed description of each module, refer to the Doxygen documentation:

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

For portable builds, set `AUTOCORE_BUILD_DIR` before configuring Visual Studio projects. Runtime files should be placed under `dist`, while machine-specific logs and user-specific data should remain outside source-controlled project files.

---

## Changelog

Auto Core originally began as a Python project named Auto Song. This name was inspired by the original primary use case: formatting the currently playing song.

When the project was ported to C++, the program name changed to Auto Core to reflect the greater level of system control and precision tuning offered by C++.

With the upgrade to C++23 modules, Auto Core branched off into two projects: Auto Core and Visual Core. Auto Core focuses on improving and fine-tuning the program core, while Visual Core is dedicated to developing and optimizing separate components, such as music player or cloud components.

---

## License

License © 2024 DJ, Daniel

Anyone is free to use, copy, and distribute Auto Core. This software is expected to be modified and configured by the individual user. Contributions to the Auto Core project can be made through the Visual Core project.

Disclaimer: This software depends on the proper functionality and configuration of Windows 11. The developers disclaim responsibility for failures, inaccuracies, or other issues that may arise from malfunctions or misconfigurations of Windows 11.
