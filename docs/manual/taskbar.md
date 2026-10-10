# Taskbar

Taskbar binds keymap commands to programs on the primary Windows taskbar. Positions 1 through 10 use Win+1 through Win+9 and Win+0, including the native thumbnail flyout. A program outside those positions is switched, minimized, restored, or cycled directly, without that flyout. Which windows cycle depends on the map's window match, so the set can differ from native Win+number. A program with no live position is matched by its window and can be started from a fallback executable.

Restart `taskbar_ac.exe` before a new `directory` or `mode` applies.

## Configuration

`config\taskbar.ini`

`[taskbar]`

| Setting | Default | Values | Description |
| --- | --- | --- | --- |
| `directory` | `taskbar` | A directory path | Folder that stores Taskbar data. A relative path is relative to the installation root. An absolute path is used as stored. |
| `mode` | `live` | `live`, `cache` | `live` discovers taskbar positions and replaces `winkey_map.cache`. `cache` reuses a usable cache and discovers positions only when that cache is missing or empty. Letter case is ignored. Any other text is treated as `live`. The menu writes `live` or `cache`. |
| `logging` | `on` | `on`, `off` | Controls logging for the Taskbar component. |

## Commands

| Command | What it does |
| --- | --- |
| `activate("name")` | Activates the taskbar program with that key. |
| `activate_<program>` | Activates the program named in that application's map. The builder writes this name. |
| `activate_auto_core` | Brings Auto Core forward. |
| `refresh_taskbar_positions` | Discovers the current Win+1 through Win+0 mappings and reports when they are updated. |
| `launch_powershell` | Starts PowerShell and focuses it. |
| `activate_powershell_in_admin` | Activates the configured `powershell_admin` program. |
| `activate_wordpad` | Activates the configured `wordpad` program. |
| `launch_gitbash` | Starts Git Bash and focuses it. |
| `launch_taskbar_config` | Opens Taskbar configuration. |
| `refresh_firefox` | If Firefox is in front, opens Reddit. Otherwise activates the configured `firefox` program. |
| `start_reddit_new_tab` | Opens `https://www.reddit.com` in Firefox. |

## Files and data

These files live in the Taskbar directory.

| File | What it is for |
| --- | --- |
| `applications\<program>.map` | One program definition. `[commands] activate` is the keymap name. `[window]` matches an open window. `[fallback] executable_path` starts the program when it is not open. `[activation] multi_window` is `cycle` or `one-shot`. |
| `winkey_map.cache` | The current Win+1 through Win+0 positions. `live` mode replaces it. `cache` mode reads it. |

`taskbar_builder.exe` creates a missing map. It does not replace an existing one. Run it again after pinned programs change. Windows Notepad is written as `notepad.map` with the packaged Notepad fallback. Microsoft Edge is written as `microsoft_edge.map`.

## Component tools

| Tool | What it is for |
| --- | --- |
| `taskbar_ac.exe` | Keeps the position cache and handles `activate_auto_core` and `refresh_taskbar_positions`. |
| `taskbar_config.exe` | Edits `taskbar.ini`. |
| `taskbar_builder.exe` | Discovers pinned programs and writes missing maps. |
| `taskbar_settings.exe` | Opens configuration and discovery. |
