# Taskbar component

The Taskbar component maps keymap commands to programs on the Windows 11
primary taskbar. For positions 1 through 10 it sends `Win + 1` through
`Win + 9` and `Win + 0`. Programs outside those slots, or with no live
mapping, emulate the same Win+position behavior using window matching and
an optional `[fallback]` executable for a new session.

`taskbar_ac.exe` is the snapshot authority. `taskbar_config.exe` writes
only `config/taskbar.ini`. `taskbar_builder.exe` writes missing
per-program `.map` files and `keymap/components/taskbar.keymap_commands.txt`.
Shared matching, discovery, and Win+position input live
in the `auto_core.taskbar` DLL module. Main owns interactive cycling and
registers `activate_*` names from the published snapshot.

## Runtime architecture

```text
keymap activate("application") or .map activate_<program>
    -> Main command registry
    -> auto_core.taskbar snapshot lookup
    -> Win+position, held-Win cycling, or emulated Win+N
       (switch / minimize / cycle / [fallback] launch)

pipe commands (activate_auto_core, refresh_taskbar_positions)
    -> hello catalog
    -> ac_taskbar_pipe
    -> taskbar_ac.exe

Main-owned names (one registration, not the control pipe)
    -> activate_wordpad / activate_powershell_in_admin
    -> launch_powershell / launch_gitbash / launch_taskbar_config
```

Auto Core launches `taskbar_ac.exe`, waits up to five seconds for the
`taskbar_ready` pipe message, then up to five seconds to connect to the
snapshot publisher. Failure is non-fatal: Main continues without native
Win+position. `taskbar_ac.exe` loads `config/taskbar.ini` and
application definitions under the configured taskbar data directory
(`taskbar/applications/*.map` by default). In `live` mode it discovers the first ten taskbar buttons
with UI Automation, writes `taskbar/winkey_map.cache`, and publishes an
immutable snapshot. In `cache` mode it uses that file when it has slots.
`refresh_taskbar_positions` always rediscovers and overwrites the cache.

Main registers `.map` `commands.activate` names from that snapshot. A new program
does not need a C++ command-registry entry. `activate_auto_core` stays on the
`taskbar_ac.exe` pipe so Auto Core can activate its console window instead of
launching a second `auto_core.exe`. `activate_wordpad` and
`activate_powershell_in_admin` run on Main with the same emulated path as
`.map` `activate_*` commands. `launch_powershell`, `launch_gitbash`, and
`launch_taskbar_config` also run on Main.

## Configuration

Settings are in `dist/config/taskbar.ini`. Per-program files and
`winkey_map.cache` live under the configured taskbar data directory
(portable default `dist/taskbar/` when Auto Core runs from `dist`).

### `config/taskbar.ini`

```ini
[taskbar]
directory = taskbar
mode = live
```

| Setting | Behavior |
| --- | --- |
| `directory` | Taskbar data root. Portable default `taskbar` (`dist/taskbar` when run from `dist`). A relative path is resolved against the installation root; an absolute path is used as-is. A missing or malformed `taskbar.ini` uses that default in memory (`report_ini_unavailable`) and does not create the file. `applications/` and `winkey_map.cache` live under this directory. Changing it does not move or delete existing files. `taskbar_ac.exe` reads it once per process, so restart `taskbar_ac.exe` before a new value applies. Only `taskbar_config.exe` writes the live file. |
| `mode` | `live` discovers positions 1–10 and overwrites `winkey_map.cache` under that data root. `cache` uses that file when it has at least one valid slot; if the file is missing or empty, it discovers once, writes the file, and uses that. Any other stored value means `live`. The interactive config UI accepts only `live` or `cache`. `refresh_taskbar_positions` always rediscovers and overwrites the cache. Restart `taskbar_ac.exe` before a new mode applies. |

The Auto Core mapping warning is `[auto_core]` `warn_without_winkey_mapping` in `config/auto_core.ini` (default `on`). When on, `taskbar_ac.exe` warns if Auto Core is not in positions 1–10. Set `off` to silence it. Only `auto_core_config.exe` writes that file.

### `taskbar/winkey_map.cache`

`taskbar_ac.exe` writes this file after a successful first-ten discovery. The
read/write implementation lives in `auto_core.dll`. It is not edited by hand.
`taskbar_config.exe` does not write it. `taskbar_builder.exe` does not
serialize it; after application-map generation succeeds it launches
`taskbar_ac.exe --refresh-cache`.

`applications/*.map` files are persistent builder-owned definitions. Seed
creates a missing file and never overwrites an existing one.
`winkey_map.cache` is the runtime-owned generated cache of the current
taskbar. `--refresh-cache` intentionally replaces it. A second seed is
missing-only for `taskbar.ini` and the `.map` files, and is not
byte-for-byte idempotent for the cache.

`mode = live` discovers the taskbar and writes the cache. `mode = cache`
uses an existing usable cache; a missing or empty cache discovers once and
writes the cache. `taskbar_ac.exe --refresh-cache` forces live discovery
and replaces the cache regardless of `mode`. It resolves the Taskbar
directory through the existing configuration path. When `config/taskbar.ini`
is unavailable, it reports that and uses the compiled default
`directory = taskbar`, the same fallback as normal `taskbar_ac.exe`.

Cache replacement is serialized with
`Local\AutoCore.Taskbar.WinkeyCache.v1`. That mutex is separate from the
long-lived authority mutex. The cache is written to a temporary file and
replaces `winkey_map.cache` only after the new snapshot is serialized. A
failed discovery or a failed replace leaves the previous cache in place.

```ini
[taskbar]
auto_core = 1
file_explorer = 2
```

### `taskbar/applications/<program>.map`

`.map` is Taskbar-owned application data. Version 1 uses the same
section/key syntax as the previous representation. The extension identifies
the file; later versions are not required to keep that syntax. `config/*.ini`
remains configuration. `taskbar_builder.exe` and `auto_core.dll` load only
`*.map` from this directory. A leftover `*.ini` file there is not read,
renamed, deleted, or treated as an existing definition.

`taskbar_builder.exe` creates a file for each pinned or running taskbar icon
that does not already have a `.map` definition. Existing `.map` files
are never overwritten. Restart Auto Core after generating files so the
authority and command registry reload them. It prompts when `process_name`
cannot be derived, and again when `[fallback] executable_path` cannot be
inferred from a catalog path, an open window, or a pinned shortcut. Enter a
full executable path, `[]` to disable launch, or leave blank.

Windows Notepad is recognized when the discovered application ID starts with
`Microsoft.WindowsNotepad`. A new definition uses `key = notepad`,
`process_name = notepad.exe`, and `[window] executable_path = ::runtime::`.
The fallback stays `shell:AppsFolder\` plus that discovered ID. `::runtime::`
is not a literal image path. It keeps the definition from storing a
versioned WindowsApps path that changes on upgrade. Resolving that path at
activation time is not implemented. If Notepad is outside taskbar positions
1 through 10, `activate_notepad` logs that and stops instead of launching
the fallback. A mapping in positions 1 through 10 still sends Win+position.

When normal inference produces `process_name = notepad.exe` and a fallback
of `C:\Windows\System32\notepad.exe`, the builder copies that inferred
fallback string into `[window] executable_path` and does not prompt. The
generated key stays on the normal path, including the usual duplicate-key
suffix. Any other `notepad.exe` program still prompts for
`[window] executable_path`.

Microsoft Edge is recognized when the discovered ID is `MSEdge` or starts
with `MSEdge.`. A new definition uses `key = microsoft_edge`,
`process_name = msedge.exe`, and the default fallback
`C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe`. The builder
logs that the default was used so a non-default install directory can be
edited. Existing `.map` files are not updated. Auto Core, iTunes,
Spotify, Discord, Zoom, and Microsoft Teams are cataloged as
`multi_window = one-shot` from the pinned AUMID or button title; the program
does not need to be open. Everyone else is `cycle`.

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
activate = activate_google_chrome
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
uses `process_name` (image filename), `executable_path` (full image path;
`::runtime::` matches no window),
`class` (`GetClassNameW`), and HWND `application_id` (AppUserModelID). File
Explorer typically sets `class = CabinetWClass`. Firefox variants typically
set HWND `application_id` to distinguish normal and private windows.

The generator does not scan the disk for programs. The program must first be
pinned or running so its taskbar identity can be observed. If no fallback
path can be inferred, the generator may prompt for one. Writes are atomic.

## Win+position mapping

In `live` mode the authority matches each `.map` `taskbar.application_id` to a
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
Explorer's taskbar group or most-recently-used thumbnail order. If the `.map`
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

`.map` `activate_*` names are registered from the snapshot when Auto Core
starts. `keymap/components/taskbar.keymap_commands.txt` is an editor-aid
catalog owned by `taskbar_builder.exe`. Discovery rewrites it, and
`taskbar_builder.exe --refresh-manifest` always rewrites it from the
compiled Taskbar commands plus the current application files.
`taskbar_builder.exe --seed` writes it only when it is missing.
`taskbar_ac.exe` does not rewrite it. `keymap/keymap_commands.txt` is
refreshed from the full runtime registry by Main.

Each application registers `activate_<key>`. `mappings.ini` uses those
names for ordinary programs. `activate_auto_core` remains a reserved pipe
command so Auto Core activates its existing console.

These names are reserved so application-definition handlers cannot replace them.
`taskbar_protocol` `commands::authority` is that reserved-name list, not a
pipe-destination list. `activate_auto_core` and `refresh_taskbar_positions`
are registered only from the `taskbar_ac.exe` hello catalog.
`refresh_taskbar_positions` is not in the authority array. The four launch
and activation names below are registered only in Main.

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
`.map` `activate_*` commands and that factory both log on Main as `[auto_core]`
(for example `[auto_core] activate_adobe_acrobat`). `taskbar_ac.exe` logs
`[taskbar]` only for reserved pipe commands.

## `taskbar_config.exe`

`taskbar_config.exe` writes only `config/taskbar.ini`. It does not write
application maps or `winkey_map.cache`.

`--init` prompts only when `config/taskbar.ini` is missing:

```text
Taskbar directory [taskbar]:
Taskbar mode [live]:
Enable logging [on]:
```

Enter accepts the shown value. `mode` must be `live` or `cache`. Logging
accepts `on` or `off`. An existing INI is left unchanged. Initialization
then always asks `Run Taskbar builder now? [Y/n]:`, because the INI is not
the sentinel for the whole Taskbar surface. Enter launches
`taskbar_builder.exe` with no arguments. `n` returns success and does not
launch the builder or `taskbar_ac.exe --refresh-cache`. If the builder
fails, the INI stays and `taskbar_config.exe` returns nonzero.

`--seed` is checked first, so `--init --seed` is `--seed` and never
prompts. Both write the compiled defaults when `taskbar.ini` is missing,
leave an existing file unchanged, and always run `taskbar_builder.exe
--seed`. They do not update `components.list`.

With no arguments, the program shows `directory` and `mode` and can change
either value. It does not discover applications. Restart `taskbar_ac.exe`
before a saved `directory` or `mode` applies. Saving `directory` does not
move or delete existing Taskbar data.

## `taskbar_builder.exe`

`taskbar_builder.exe` is the only writer of `taskbar/applications/*.map` and
`keymap/components/taskbar.keymap_commands.txt`.

With no arguments it discovers pinned and running taskbar icons. For each
icon without a `.map` file it derives `application.key`,
`taskbar.application_id`, `[window] process_name`, `[fallback]`, and
`activate_<key>`. It prompts
when `process_name` cannot be derived, and when `[fallback] executable_path`
cannot be inferred. Windows Notepad (`Microsoft.WindowsNotepad` prefix) is
written as `notepad.map` with `executable_path = ::runtime::` and the
packaged fallback. A `notepad.exe` whose inferred fallback is
`C:\Windows\System32\notepad.exe` copies that fallback into
`[window] executable_path` and does not prompt. Any other `notepad.exe`
still prompts for `[window] executable_path` (Enter leaves it blank).
Microsoft Edge (`MSEdge` or `MSEdge.`) is written as `microsoft_edge.map`
with `process_name = msedge.exe` and the default x86 Edge fallback.
Packaged-app `[fallback]` is still `shell:AppsFolder\<AUMID>`. `Update.exe` and
`ApplicationFrameHost.exe` are never stored as `process_name`. Existing
`.map` files are left unchanged. Files named `*.ini` in
`taskbar/applications/` are ignored. The command catalog is then rewritten.

`--seed` is noninteractive. It discovers the current taskbar and creates
only missing `applications/*.map` files. An existing `.map` is not opened
or rewritten, even when its contents differ from the current builder
output. Fields that would require a prompt are left blank. It writes the
command catalog only when that file is missing. After the application-map
step succeeds, it launches `taskbar_ac.exe --refresh-cache` even if the
catalog write fails. A catalog failure still makes the builder return
nonzero. Map generation failure does not launch the refresh and does not
delete maps already created in that run.

The no-argument builder keeps the interactive prompts, rewrites the command
catalog, and launches `taskbar_ac.exe --refresh-cache` only after
application-map generation succeeds. `--refresh-manifest` always rewrites
the catalog from the compiled Taskbar commands plus the current application
files and does not refresh the cache. The builder post-build runs
`--refresh-manifest`.

`taskbar_ac.exe --refresh-cache` resolves the Taskbar directory, loads
`applications/*.map` from that directory, forces live discovery, matches
those positions, and replaces `winkey_map.cache`. It then exits. It does
not start the control pipe or become the long-lived authority.
`mode = cache` does not make this flag load the old cache. An unavailable
`config/taskbar.ini` uses `directory = taskbar`. Discovery or replace
failure returns nonzero and leaves a previous cache file in place. When
that refresh fails, `taskbar_builder.exe` reports
`Unable to refresh winkey_map.cache.` and returns nonzero. Maps already
created stay in place.

`taskbar_star.exe` lists **Discover taskbar applications**, which launches
`taskbar_builder.exe` with no arguments. Initialize or Configure comes
first, then that action, then Enable or Disable.

Catalog matching uses the pinned AUMID and button title, so the program does
not need to be open. Auto Core, iTunes, Spotify, Discord, Zoom, and
Microsoft Teams get stable keys (`auto_core`, `itunes`, `spotify`,
`discord`, `zoom_workplace`, `microsoft_teams`), a catalog `process_name`
(`auto_core.exe`, `iTunes.exe`, `Spotify.exe`, `discord.exe`, `Zoom.exe`,
`ms-teams.exe`), and `multi_window = one-shot`. Other programs default to
`cycle`. Discord's Squirrel `[fallback]` may still be `Update.exe`; only
`process_name` is forced to `discord.exe`.

If either Firefox icon is present, both `firefox.map` and
`firefox_private_browsing.map` are created when missing. An Acrobat Reader
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
| `app/components/taskbar/runtime/` | `taskbar_ac.exe` authority, control pipe, compiled commands. |
| `app/components/taskbar/config/` | `taskbar_config.exe`, the only writer of `config/taskbar.ini`. |
| `app/components/taskbar/builder/` | `taskbar_builder.exe` application discovery and command catalog. |
| `app/components/taskbar/builder/enum_windows.cxx` | Window enumerator used while generating `.map` definitions. |
| `app/components/taskbar/star/` | `taskbar_star.exe` menu. It delegates and does not write files. |
| `app/core/taskbar/` | `auto_core.taskbar` snapshot, matching, and Win+position input. |
| `app/main/runtime/taskbar/main_taskbar.ixx` | Main cycling session, `.map` commands, and the Main-owned launches. |
| `app/main/runtime/taskbar/main_taskbar.cxx` | Native vs emulated `activate_*`, cycling, and foreground launches. |
| `app/components/taskbar/shared/taskbar_protocol.ixx` | Reserved command names shared with `taskbar_ac.exe`. |

## Manual test

1. Pin or run a program that has no file in `taskbar/applications`.
2. Run `taskbar_config.exe` and confirm it edits only `config/taskbar.ini`.
   Run Discover taskbar applications (`taskbar_builder.exe`). Confirm it
   creates a missing `.map` definition and rewrites
   `keymap/components/taskbar.keymap_commands.txt`. An existing `.map`
   file stays unchanged. A `*.ini` file in `taskbar/applications/` is ignored.
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
