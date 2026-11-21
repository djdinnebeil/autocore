# Taskbar

Taskbar is a Main subsystem. Its executable is `app\auto_core_taskbar.exe`. It binds keymap commands to programs on the primary Windows taskbar. Positions 1 through 10 use Win+1 through Win+9 and Win+0, including the native thumbnail flyout. A program outside those positions is switched, minimized, restored, or cycled directly, without that flyout. Which windows cycle depends on the map's window match, so the set can differ from native Win+number. A program with no live position is matched by its window and can be started from a fallback executable.

Restart `auto_core_taskbar.exe` before a new `mode` applies. Taskbar data is always `<installation>\data\taskbar`.

## Configuration

`config\taskbar_policy.ini`

`[taskbar]`

| Setting | Default | Values | Description |
| --- | --- | --- | --- |
| `mode` | `live` | `live`, `cache` | `live` discovers taskbar positions and replaces `winkey_map.cache`. `cache` reuses a usable cache and discovers positions only when that cache is missing or empty. Letter case is ignored. Any other text is treated as `live`. The menu writes `live` or `cache`. |
| `logging` | `on` | `on`, `off` | Controls logging for Taskbar. |

## Commands

| Command | What it does |
| --- | --- |
| `activate("name")` | Activates the taskbar program with that key. |
| `activate_<program>` | Activates the program named in that application's map. When more than one provider offers the name, `keymap\activate_commands.map` selects who runs the shared name. `taskbar_activate_<program>` always runs Taskbar. |
| `activate_auto_core` | Brings Auto Core forward. |
| `refresh_taskbar_positions` | Discovers the current Win+1 through Win+0 mappings and reports when they are updated. |
| `launch_powershell` | Starts PowerShell and focuses it. |
| `activate_powershell_in_admin` | Activates the configured `powershell_admin` program. |
| `activate_wordpad` | Activates the configured `wordpad` program. |
| `launch_gitbash` | Starts Git Bash and focuses it. |
| `launch_taskbar_config` | Opens Taskbar configuration. |
| `refresh_firefox` | If Firefox is in front, opens Reddit. Otherwise activates the configured `firefox` program. |
| `start_reddit_new_tab` | Opens `https://www.reddit.com` in Firefox. |

A mapped command sends Win+digit through `auto_core.dll` in the process that already connected to the snapshot. That press does not start another executable. An application with a map and no slot uses Main's direct activation path: focus, restore, minimize, cycle, or the fallback executable.

## Catalogs

`auto_core_keymap.exe --build` and startup resolve a shared `activate_<program>` name with the same rules. `--build` sees successful provider exports and the application maps. Startup sees the providers whose actions it can invoke: the application maps, started component catalogs, and enabled launch descriptors. A component that exported successfully and then failed to start is reported at startup, and its action is not registered.

One provider and no saved line keeps that provider's `activate_<program>` command, with no alias. A saved provider that is available runs the shared name, and each available provider also gets an explicit alias. A saved provider that is unavailable is left in the file, reported, and does not run the shared name. The aliases of the providers that are available are still published.

`keymap\activate_commands.map` is written only by `auto_core_keymap.exe --build`, alongside `keymap\keymap_commands.txt`. A failed export leaves the map unchanged. A successful rebuild refreshes each choice comment from the providers that offer the command and preserves the saved value, including a blank. One provider uses `# Choice:`. Any other count uses `# Choices:`, with providers separated by ` | `. A new collision defaults to `taskbar` when Taskbar offers the command. Otherwise the value is left blank. Startup reads the map and does not create or repair it.

`refresh_taskbar_positions` is advertised by the running authority. `--build` does not launch that process, so the name is absent unless a map or another provider exports it. Slots stay in the snapshot. `winkey_map.cache` is the cache that snapshot can publish.

## Files and data

These files live in `<installation>\data\taskbar`.

| File | What it is for |
| --- | --- |
| `applications\<program>.map` | One program definition. `[commands] activate` is the keymap name. `[window]` matches an open window. `[fallback] executable_path` starts the program when it is not open. `[activation] multi_window` is `cycle` or `one-shot`. |
| `winkey_map.cache` | The current Win+1 through Win+0 positions. `live` mode replaces it. `cache` mode reads it. |

`keymap\activate_commands.map` is the provider chosen for each shared `activate_<program>` command. Section `[activation_map]`. `auto_core_keymap.exe --build` is the only writer.

`app\auto_core_taskbar.exe --builder` creates a missing map. It does not replace an existing one. Run it again after pinned programs change. Windows Notepad is written as `notepad.map` with the packaged Notepad fallback. Microsoft Edge is written as `microsoft_edge.map`.

## Tools

| Tool | What it is for |
| --- | --- |
| `auto_core_taskbar.exe --run` | Keeps the position cache and handles `activate_auto_core` and `refresh_taskbar_positions`. Main starts this once per session. |
| `auto_core_taskbar.exe --config` | Edits `taskbar_policy.ini`. |
| `auto_core_taskbar.exe --builder` | Discovers pinned programs and writes missing maps, then runs `auto_core_taskbar.exe --run --refresh-cache`. |
| `auto_core_taskbar.exe` | Prints `Discovering taskbar applications`, then runs `--builder` once before the menu. Exit 0 prints `Taskbar application discovery complete.` A failed discovery still opens the menu. A failed `--config --init` returns that exit code. |
| `auto_core_taskbar.exe --settings` | Opens configuration and discovery. |
