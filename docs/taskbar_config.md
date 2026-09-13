# Taskbar configuration

Taskbar configuration, `taskbar_ac.exe`, `taskbar_config.exe`, and related
modules are documented in [Taskbar component](taskbar.md).

`taskbar_config.exe` shares `config/taskbar.ini` with `taskbar_ac.exe` (no
separate config INI). If that live file is missing, the helper prompts for the
application-data directory, suggesting `.\taskbar` as the default to accept.
Blank input stores `[taskbar] directory = taskbar`. An existing live file is
never rewritten. Per-program files stay under that data root's `applications/`
folder, not under `config/`.
