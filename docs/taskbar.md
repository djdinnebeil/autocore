# Taskbar component

The Taskbar component maps keymap commands to programs on the Windows 11
primary taskbar. For positions 1 through 10 it sends `Win + 1` through
`Win + 9` and `Win + 0`. Programs outside those slots, or with no live
mapping, emulate the same Win+position behavior using window matching and
an optional `[fallback]` executable for a new session.

`taskbar_ac.exe` is the snapshot authority. `taskbar_config.exe` writes
`config/taskbar.ini` and missing per-program INI files. Shared matching, discovery, and Win+position input live
in the `auto_core.taskbar` DLL module. Main owns interactive cycling and
registers INI `activate_*` names from the published snapshot.

## Runtime architecture

```text
keymap activate("application") or INI activate_<program>
    -> Main command registry
    -> auto_core.taskbar snapshot lookup
    -> Win+position, held-Win cycling, or emulated Win+N
       (switch / minimize / cycle / [fallback] launch)

pipe commands (activate_auto_core, refresh_taskbar_positions)
    -> ac_taskbar_pipe
    -> taskbar_ac.exe

Main-local reserved names (not the control pipe)
    -> activate_wordpad / activate_powershell_in_admin
    -> launch_powershell / launch_gitbash / launch_taskbar_config
```

Auto Core launches `taskbar_ac.exe`, waits up to five seconds for the
`taskbar_ready` pipe message, then up to five seconds to connect to the
snapshot publisher. Failure is non-fatal: Main continues without native
Win+position. `taskbar_ac.exe` loads `config/taskbar.ini` and
application files under the configured taskbar data directory
(`taskbar/applications/*.ini` by default). In `live` mode it discovers the first ten taskbar buttons
with UI Automation, writes `taskbar/cached_positions.ini`, and publishes an
immutable snapshot. In `cache` mode it uses that file when it has slots.
`refresh_taskbar_positions` always rediscovers and overwrites the cache.

Main registers INI `commands.activate` names from that snapshot. A new program
does not need a C++ command-registry entry. `activate_auto_core` stays on the
`taskbar_ac.exe` pipe so Auto Core can activate its console window instead of
launching a second `auto_core.exe`. `activate_wordpad` and
`activate_powershell_in_admin` run on Main with the same emulated path as INI
`activate_*` commands. `launch_powershell`, `launch_gitbash`, and
`launch_taskbar_config` also run on Main.

## Configuration

Settings are in `dist/config/taskbar.ini`. Per-program files and
`cached_positions.ini` live under the configured taskbar data directory
(portable default `dist/taskbar/` when Auto Core runs from `dist`).

### `config/taskbar.ini`

```ini
[taskbar]
directory = taskbar
mode = live
```

| Setting | Behavior |
| --- | --- |
| `directory` | Taskbar data root. Portable default `taskbar` (`dist/taskbar` when run from `dist`). A relative path is resolved against the installation root; an absolute path is used as-is. A missing or malformed `taskbar.ini` uses that default in memory (`report_ini_unavailable`) and does not create the file. Shared by `taskbar_ac.exe` and `taskbar_config.exe`. Only `taskbar_config.exe` writes the live file. |
| `mode` | `live` discovers positions 1–10 and overwrites `cached_positions.ini` under that data root. `cache` uses that file when it has at least one valid slot; if the file is missing or empty, it discovers once, writes the file, and uses that. Any other value means `live`. `refresh_taskbar_positions` always rediscovers and overwrites the cache. |

The Auto Core mapping warning is `[main]` `warn_without_winkey_mapping` in `config/main.ini` (default `true`). When true, `taskbar_ac.exe` warns if Auto Core is not in positions 1–10. Set `false` to silence it.

### `taskbar/cached_positions.ini`

`taskbar_ac.exe` writes this file after a successful first-ten discovery. It is
not edited by hand. `taskbar_config.exe` does not write it.

```ini
[taskbar]
auto_core = 1
file_explorer = 2
```

### `taskbar/applications/<program>.ini`

`taskbar_config.exe` creates a file for each pinned or running taskbar icon
that does not already have one. Existing files are never overwritten. Restart
Auto Core after generating files so the authority and command registry reload
them. It prompts when `process_name` cannot be derived, and again when
`[fallback] executable_path` cannot be inferred from a catalog path, an open
window, or a pinned shortcut. Enter a full executable path, `[]` to disable
launch, or leave blank. When `process_name` is `notepad.exe`, it also prompts
for `[window] executable_path` so two programs that share that image name can
be told apart. Enter a full path or leave blank. Existing files are not
updated. Auto Core, iTunes, Spotify, Discord, Zoom, and Microsoft Teams are
cataloged as `multi_window = one-shot` from the pinned AUMID or button title;
the program does not need to be open. Everyone else is `cycle`.

```ini
[application]
key = google_chrome

[taskbar]
application_id = Chrome

[window]
process_name = chrome.exe
executable_path =
class =
application_id =

[activation]
multi_window = cycle

[fallback]
executable_path = C:\Program Files\Google\Chrome\Application\chrome.exe

[commands]
activate = activate_google_chrome | activate_chrome
```

| Field | Role |
| --- | --- |
| `application.key` | Unique internal key. Lowercase letters, digits, and underscores; begins with a letter; at most 64 characters. |
| `taskbar.application_id` | Taskbar AppUserModelID from UI Automation. The runtime also accepts an exact Automation ID match. |
| `activation.multi_window` | `one-shot` or `cycle`. |
| `[window]` | Identifies existing windows for `cycle`. Case-insensitive AND. A blank field is ignored. |
| `fallback.executable_path` | Launch path when there is no native mapping and no matching window. Blank means report that there is no fallback. `[]` also disables launch and prints a console notification. Packaged apps use `shell:AppsFolder\<AUMID>`. Not used for window counting. |
| `commands.activate` | Pipe-separated public command names. Multiple names route to the same internal key. |

Window titles are not used. Eligible windows are visible and titled. Matching
uses `process_name` (image filename), `executable_path` (full image path),
`class` (`GetClassNameW`), and HWND `application_id` (AppUserModelID). File
Explorer typically sets `class = CabinetWClass`. Firefox variants typically
set HWND `application_id` to distinguish normal and private windows.

The generator does not scan the disk for programs. The program must first be
pinned or running so its taskbar identity can be observed. If no fallback
path can be inferred, the generator may prompt for one. Writes are atomic.

## Win+position mapping

In `live` mode the authority matches each INI `taskbar.application_id` to a
discovered slot in positions 1 through 10. Moving icons among those slots
updates the mapping on `refresh_taskbar_positions` or after a restart. A
program past position 10, or with no matching identity, has no native
Win+1–10 shortcut. Configured applications stay in the snapshot so
`activate_*` can still mirror Win+position behavior.

`MainTaskbarState::activate_configured` then:

1. Sends Win+position when a mapping exists. For `cycle` with two or more
   matching windows, Main holds Win and repeats the position.
2. Otherwise emulates Win+position using `[window]` matching:
   - 0 windows: launches `[fallback] executable_path` and focuses the new
     window.
   - 1 window: restores, minimizes, or switches, matching Win+N.
   - 2+ windows: cycles when `multi_window = cycle` (same mapped key
     advances; any other mapped key ends the session). `one-shot`
     activates the top z-order match.
3. Otherwise reports that there is no fallback.

Emulated cycling does not hold Win. There is no position key past slot 10,
and a held Win key would open the Start menu.

### Differences from native Win+position

Positions 1 through 10 reuse Windows' own shortcut, including the taskbar
thumbnail flyout: while Win stays held, a white highlight box moves among
the program's windows. Emulated activation past slot 10 switches, minimizes,
restores, and cycles the matching windows directly. It does not open that
thumbnail strip. That is an accepted limitation.

Emulated cycling also uses Auto Core's `[window]` matcher and z-order, not
Explorer's taskbar group or most-recently-used thumbnail order. If the INI
matcher is too broad or too narrow, the set of windows that cycle can
differ from what Win+N would show.

Auto Core itself is an exception: if it has no mapping, `taskbar_ac.exe`
activates the shared console window instead of launching another
`auto_core.exe`.

## Auto Core console

`activate_auto_core` stays on the `taskbar_ac.exe` control pipe so Auto Core
activates its existing console instead of launching a second `auto_core.exe`.
That path uses native Win+position when a mapping exists, otherwise the
shared console window.

Interactive prompts in writer and journal use
`ac::console::focus_for_prompt_via_winkey()` in-process. When a process-local
snapshot has an `auto_core` slot (logical 1–10), that sends Win+number.
No snapshot, or a snapshot with no `auto_core` mapping, activates the
console window directly. Main prompts use `focus_for_prompt()`, which never
sends Win+number.

## Runtime commands

INI `activate_*` names are registered from the snapshot when Auto Core
starts. `keymap/components/taskbar.keymap_commands.txt` is rewritten by
`taskbar_config.exe` after generation and by `taskbar_ac.exe` when the authority
starts. `keymap/keymap_commands.txt` is refreshed from the full runtime
registry.

Known historical keymap names are preserved as aliases:

| Internal key | Generated command | Historical alias |
| --- | --- | --- |
| `file_explorer` | `activate_file_explorer` | `activate_folder` |
| `google_chrome` | `activate_google_chrome` | `activate_chrome` |
| `visual_studio` | `activate_visual_studio` | `activate_visual` |
| `visual_studio_code` | `activate_visual_studio_code` | `activate_vs_code` |
| `zoom_workplace` | `activate_zoom_workplace` | `activate_zoom` |

`mappings.ini` uses `activate_*` names (and historical aliases such as
`activate_chrome`) for ordinary programs. `activate_auto_core` remains a
reserved pipe command so Auto Core activates its existing console.

These Main names are reserved so INI handlers cannot replace them.
`taskbar_protocol` `commands::authority` is that reserved-name list, not a
pipe-destination list. `refresh_taskbar_positions` is also reserved by Main
registration and is a pipe payload, but it is not in that array.

| Command | Runs on | Behavior |
| --- | --- | --- |
| `activate_auto_core` | `taskbar_ac.exe` pipe | Native mapping, otherwise the shared Auto Core console. |
| `activate_powershell_in_admin` | Main | Native mapping for `powershell_admin`, otherwise emulated Win+N using `runas` PowerShell when no window matches. |
| `activate_wordpad` | Main | Native mapping for `wordpad`, otherwise emulated Win+N using `wordpad.exe` when no window matches. |
| `launch_powershell` | Main | Starts PowerShell and focuses a visible window from that process tree. |
| `launch_gitbash` | Main | Starts `C:\Program Files\Git\git-bash.exe --cd-to-home` and focuses a visible window. |
| `refresh_taskbar_positions` | `taskbar_ac.exe` pipe | Recalculates live slots 1–10. |
| `launch_taskbar_config` | Main | Starts `taskbar_config.exe` in a new console. |

`refresh_firefox` and `start_reddit_new_tab` are Main-owned keymap helpers,
not `taskbar_ac.exe` authority commands. If Firefox is already the foreground
window, `refresh_firefox` opens Reddit; otherwise it activates the configured
`firefox` application. `start_reddit_new_tab` launches
`C:\Program Files\Mozilla Firefox\firefox.exe` with `https://www.reddit.com`.

`activate("application")` is a Main factory that activates by internal key.
INI `activate_*` commands and that factory both log on Main as `[auto_core]`
(for example `[auto_core] activate_adobe_acrobat`). `taskbar_ac.exe` logs
`[taskbar]` only for reserved pipe commands.

## `taskbar_config.exe`

Run `taskbar_config.exe` from the Auto Core distribution directory. If
`config/taskbar.ini` is missing, it prompts for the application-data directory,
suggesting `.\taskbar` as the default to accept, and writes `[taskbar] directory`
and `mode = live`. Blank input stores `directory = taskbar`. If the file already
exists, the helper uses that value and does not rewrite it. Per-program files
are written under `<that directory>/applications/`. For each
taskbar icon without an INI it derives `application.key`,
`taskbar.application_id`, `[window] process_name`, `[fallback]`, and
`activate_<key>` (plus historical aliases for known programs). It prompts
when `process_name` cannot be derived, and when `[fallback] executable_path`
cannot be inferred. `process_name` `notepad.exe` also prompts for
`[window] executable_path` (Enter leaves it blank) so Store Notepad and
Notepad2 can be distinguished. Packaged-app `[fallback]` is still
`shell:AppsFolder\<AUMID>`. `Update.exe` and
`ApplicationFrameHost.exe` are never stored as `process_name`.

Catalog matching uses the pinned AUMID and button title, so the program does
not need to be open. Auto Core, iTunes, Spotify, Discord, Zoom, and
Microsoft Teams get stable keys (`auto_core`, `itunes`, `spotify`,
`discord`, `zoom_workplace`, `microsoft_teams`), a catalog `process_name`
(`auto_core.exe`, `iTunes.exe`, `Spotify.exe`, `discord.exe`, `Zoom.exe`,
`ms-teams.exe`), and `multi_window = one-shot`. Other programs default to
`cycle`. Discord's Squirrel `[fallback]` may still be `Update.exe`; only
`process_name` is forced to `discord.exe`.

If either Firefox icon is present, both `firefox.ini` and
`firefox_private_browsing.ini` are created when missing. An Acrobat Reader
taskbar identity (`AcrobatReader`) uses a catalog matcher of `Acrobat.exe`
and a disabled fallback (`[]`). Generation does not depend on a running
Acrobat window, a pinned shortcut target, or a known install path. Acrobat
is not launched when there is no matching window. Packaged app identities
contain `!` (`PackageFamilyName!AppId`, often `!App`). Those get
`shell:AppsFolder\<AUMID>` as fallback so Store apps are not launched from a
versioned `WindowsApps` file path. This catalog entry is preferred even when
the app is already running.

## Source layout

| Path | Role |
| --- | --- |
| `app/components/taskbar/main/` | `taskbar_ac.exe` authority, control pipe, compiled commands. |
| `app/components/taskbar/config/` | `taskbar_config.exe` generator and keymap rewrite. |
| `app/components/taskbar/config/enum_windows.cxx` | Config-side EnumWindows enumerator for window matching. |
| `app/core/taskbar/` | `auto_core.taskbar` snapshot, matching, and Win+position input. |
| `app/main/modules/main_taskbar.ixx` | Main cycling session and INI command registration. |
| `app/main/component/src/main_taskbar.cxx` | Native vs emulated `activate_*` and cycling. |
| `app/main/modules/taskbar_component.ixx` | Main-side `taskbar_ac.exe` lifecycle and control pipe. |
| `app/main/component/src/taskbar_component.cxx` | Lifecycle client, pipe invoke, and Main-local reserved launches. |
| `app/shared/protocols/taskbar_protocol.ixx` | Control-pipe contract and authority command names. |

## Manual test

1. Pin or run a program that has no file in `taskbar/applications`.
2. Start `taskbar_config.exe`. Confirm it creates a new INI and updates
   `keymap/components/taskbar.keymap_commands.txt`.
3. Restart Auto Core. Invoke the generated `activate_<key>` command. It must
   use Win+1 through Win+10 when the icon is in those positions.
4. Move the icon among the first ten positions and invoke
   `refresh_taskbar_positions` (or restart). The Win+position mapping must
   follow the icon.
5. Move the icon beyond position 10 or unpin it. `activate_<key>` must still
   switch, minimize, restore, or cycle matching windows. `[fallback]` launches
   only when no matching window is open. If there is no fallback and no
   window, Auto Core reports that there is no fallback.
6. For a program with two windows and `multi_window = cycle`, invoke its
   command repeatedly. In positions 1–10, native Win+position cycling remains
   active and the taskbar thumbnail flyout is expected. Beyond position 10,
   the same mapped key cycles matching windows without thumbnails, and the
   function key or 0 ends the session.
7. Confirm historical aliases such as `activate_folder` and `activate_chrome`
   still activate File Explorer and Google Chrome.
