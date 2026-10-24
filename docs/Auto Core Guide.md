# Auto Core Guide

## Run Auto Core

Launch Auto Core using `Auto Core.lnk`.

For optimal performance, pin `Auto Core.lnk` to one of the first 10 positions (1–10) on the Windows taskbar. This allows Auto Core to use Windows key shortcuts for activation, providing more consistent responsiveness under system load.

Alternatively, run `app\auto_core.exe` directly.

## Configure Auto Core

Launch `Auto Core Settings.lnk` to manage settings and configure components.

You can also manually modify the following files:

- `components.list` — Enable or disable installed components.
- `keymap.map` — Assign keyboard inputs to keymap commands.

Configuration Mode allows you to interactively select your preferred settings.

Alternatively, run `app\<component>_settings.exe` to configure individual components.

## Installation Structure

| File or Directory | Purpose |
|---|---|
| `app/` | Application executables (`.exe`) and libraries (`.dll`). |
| `components/` | Data used by installed components. |
| `config/` | Application and component configuration settings. |
| `data/` | Logs, diagnostics, and other application data. |
| `info/` | User documentation, including the Manual and Tutorial. |
| `keymap/` | Reference information for available keymap commands. |
| `taskbar/` | Data used by the Taskbar component. |
| `writer/` | Data used by the Writer component. |
| `Auto Core.lnk` | Shortcut for launching Auto Core. Recommended for taskbar pinning. |
| `Auto Core Settings.lnk` | Shortcut for accessing the settings manager. |
| `Auto Core Guide.md` | Introduction to launching, configuring, and navigating Auto Core. |
| `components.list` | List of installed components and their enabled or disabled states. |
| `keymap.map` | Keyboard mappings that associate keys with configured commands. |

## More Documentation

See [Auto Core Tutorial](<info/Auto Core Tutorial.md>) for guided customization and extension walkthroughs.

See [Auto Core Manual](<info/Auto Core Manual.md>) for detailed reference documentation, including component settings, commands, configuration files, and other features.

Additional reference material is available under `info\manual\`.