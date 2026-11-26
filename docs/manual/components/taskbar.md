# Taskbar

Taskbar is a Main subsystem. Its executable is `app\auto_core_taskbar.exe`. It binds keymap commands to programs on the primary Windows taskbar. A one-shot map in positions 1 through 10 sends Win+1 through Win+9 and Win+0, including the native thumbnail flyout. A cycle map in one of those positions, with two or more matching windows, holds Win and taps that slot digit on each repeat of the shortcut that started it. The digit 0 on the keyboard selects the highlighted window. A cycle map with no cache slot walks a fixed list of matching windows and skips any that close. A program with no cache slot and one window is matched by its window and can be started from a fallback executable.

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
| `taskbar_<program>` | Activates the program named in `<program>.map` through the Taskbar helper. The application key stays `<program>`. |
| `activate_<program>` | Shared component command. `keymap\activate_commands.map` selects the provider. Each current provider also has a direct alias: `<provider>_activate` when the provider name matches `<program>`, otherwise `<provider>_activate_<program>`. |
| `activate_auto_core` | Brings Auto Core forward through the compiled Taskbar handler. Provider `auto_core` offers it whether or not `auto_core.map` exists. `taskbar_auto_core` is the separate map-backed command. |
| `refresh_taskbar_positions` | Discovers the current Win+1 through Win+0 mappings and reports when they are updated. |
| `refresh_firefox` | If Firefox is in front, opens Reddit. Otherwise activates the configured `firefox` program. |
| `start_reddit_new_tab` | Opens `https://www.reddit.com` in Firefox. |

`ac::taskbar::activate_application` reads the application map and `winkey_map.cache` on each call. A one-shot map with a cache slot sends Win+digit and releases that chord in the same send. A cycle map with a slot and at least two matching windows holds Win and taps the slot digit; repeating the same shortcut taps it again, and the keyboard 0 key releases Win to select the highlight. Slot 10 uses a synthetic 0 for that tap, which is not the selection key. A cycle map with no slot keeps the window order from the first press. A `::runtime::` window path with no cache slot asks the user to pin the program and refresh positions.

## Catalogs

`auto_core_keymap.exe --build` rewrites `keymap\activate_commands.map` from component `activate_<program>` offers and the built-in `activate_auto_core` offer from provider `auto_core`. Taskbar publishes `taskbar_<program>` on the ordinary command path. Startup applies the same provider rules in memory and does not write the map. One current provider runs the shared name even when the saved row is missing or stale. Several providers keep a saved provider that is still offered and otherwise leave the shared name unresolved. Every current provider gets its direct alias.

`# Choices:` lists the providers discovered by that build. One provider is selected. A saved `taskbar` or `<key>.map` value is stale: one remaining provider replaces it, and several providers make it blank. A command with no current provider is removed. Setup still opens when a map file is missing. It does not write `taskbar` back into the activation map.

`refresh_taskbar_positions` is advertised by the running authority. `--build` does not launch that process, so the name is absent unless a map or another provider exports it. Slots stay in the snapshot. `winkey_map.cache` is the cache that snapshot can publish.

## Files and data

These files live in `<installation>\data\taskbar`.

| File | What it is for |
| --- | --- |
| `applications\<program>.map` | One program definition. `[commands] activate` is the keymap name. `[window]` matches an open window. `[fallback] executable_path` starts the program when it is not open. `[activation] multi_window` is `cycle` or `one-shot`. |
| `winkey_map.cache` | The current Win+1 through Win+0 positions. `live` mode replaces it. `cache` mode reads it. |

`keymap\activate_commands.map` is the provider chosen for each shared `activate_<program>` command. Section `[activation_map]`. `auto_core_keymap.exe --build` rewrites it. Startup reads it and does not write it. One provider is `# Choices: <provider>` and `<command> = <provider>`.

`app\auto_core_taskbar.exe --builder` creates a missing map. It does not replace an existing one. Run it again after pinned programs change. Windows Notepad is written as `notepad.map` with the packaged Notepad fallback. Microsoft Edge is written as `microsoft_edge.map`.

## Tools

| Tool | What it is for |
| --- | --- |
| `auto_core_taskbar.exe --run` | Keeps the position cache and handles `activate_auto_core` and `refresh_taskbar_positions`. Main starts this once per session. |
| `auto_core_taskbar.exe --config` | Edits `taskbar_policy.ini`. |
| `auto_core_taskbar.exe --builder` | Discovers pinned programs and writes missing maps, then runs `auto_core_taskbar.exe --run --refresh-cache`. |
| `auto_core_taskbar.exe --setup-application <key>` | Asks you to pin that program, waits for a key, then runs the noninteractive `--builder --seed` path and cache refresh. The console stays open until another keypress. For Excel the prompt names Excel. |
| `auto_core_taskbar.exe` | Prints `Discovering taskbar applications`, then runs `--builder` once before the menu. Exit 0 prints `Taskbar application discovery complete.` A failed discovery still opens the menu. A failed `--config --init` returns that exit code. |
| `auto_core_taskbar.exe --settings` | Opens configuration and discovery. |
