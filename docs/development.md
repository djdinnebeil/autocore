# Development

How to extend Auto Core: shared protocols, runtime commands, and new component projects.

## Shared folder

The `app/shared` folder contains the generic host protocol and command registry. Name-specific protocols and `defaults.ixx` live under `app/components/<name>/shared/`. Reusable runtime facilities such as paths, logging, and named pipes are provided by the core DLL under `app/core`.

The C++23 module catalog is [modules.md](modules.md).

| Folder or Module | Purpose |
| --- | --- |
| `app/shared/command_registry.ixx` | Stores and resolves runtime keymap command names. |
| `app/shared/components_editor_request.ixx` | Shared `--init` / `--seed` parsing. `<name>_star.exe` launches `components_editor.exe --component <name> --on` or `--off`. |
| `app/shared/protocols/component_protocol.ixx` | Generic `ac.component.v1` hello, invoke, and shutdown. |
| `itunes/shared/itunes_protocol.ixx` | iTunes command names used by `itunes_ac.exe`. |
| `journal/shared/journal_protocol.ixx` | Journal command tokens used by `journal_ac.exe`. |
| `slash/shared/slash_protocol.ixx` | Slash recycle-bin command names. |
| `spotify/shared/spotify_protocol.ixx` | Spotify command names used by `spotify_ac.exe`. |
| `taskbar/shared/taskbar_protocol.ixx` | Reserved Main-local taskbar command names. |
| `writer/shared/writer_protocol.ixx` | Writer command names used by `writer_ac.exe`. |

## Registering runtime commands

To make a function available to the keymap, advertise it from a hosted child's `ac.component.v1` hello catalog, or list it in the `AC_LAUNCH_DESCRIPTOR` resource of an on-demand component. Main-owned commands (`close_program`, the function-key commands, and the Taskbar commands that must run in the foreground process) are registered in Main. Auto Core refreshes `dist/keymap/keymap_commands.txt` from that registry at startup. Adding an ordinary hosted or on-demand component does not require a Main source change. Taskbar `activate_*` names for configured programs are registered from `taskbar/applications/*.map`.

The command registry is a process-lifetime static built in this order: test commands, Main-owned commands, Taskbar commands that run in Main, hosted hello catalogs and enabled on-demand commands, then configured `activate_*` names last so a taskbar `.map` cannot override a reserved name. Duplicate `add` / `add_factory` throws `std::logic_error` for Main-local names. A child catalog name that matches a Main-local command is skipped silently. A collision with an earlier child's catalog skips and logs.

Mappings in `dist/keymap.map` are the key map. If `keymap.map` is missing, run `keymap_editor.exe` to write a seed of every `key_codes` name (`numpad_0` / `numpad_1` filled, other keys left blank as `key =`). If workspace use or file load fails, Main installs a two-key emergency map in memory and does not write `keymap.map`. See [configuration.md](configuration.md).

## Defaults vs live files

A public clone should not contain another person's keymap, journal print-choice table, or machine-specific config. `dist/` is gitignored. Portable text is compiled; the live files are written when missing:

- `components/journal/<factory_name>.list` — `journal_builder.exe --seed` writes a missing starter alias file from [`journal_factories.ixx`](../app/components/journal/shared/journal_factories.ixx). An existing file is left unchanged. `journal_config.exe --init` and `--seed` launch that owner, then `journal_cloud.exe`, `journal_clock.exe`, `journal_db.exe`, and `journal_series.exe`, each for its own missing file.
- `keymap.map` — `keymap_editor.exe` writes a seed (`numpad_0` / `numpad_1` filled, every other `key_codes` name blank)
- `components.list` — `components_editor.exe --seed` creates it from installed `*_ac.exe` names that already have `config/<name>.ini`
- `config/*.ini` — Main host INIs are written only by the matching `_config.exe` from [`app/main/shared/defaults.ixx`](../app/main/shared/defaults.ixx). Component `<name>.ini` is written only by `<name>_config.exe` from that child's `shared/defaults.ixx`
- `dist/components/server/` — default Server data directory. `server_config.exe --init` and `--seed` write a missing `config/server.ini`, then launch `server_editor.exe` for missing `port.id` and `document_root.id` and `server_builder.exe --seed` for missing `index.html` and `styles.css`. `--seed` always requests the starter site. `--init` asks first. An existing file is left unchanged. The builder reads `document_root.id` and writes only under a relative document root from [`default_site.ixx`](../app/components/server/shared/default_site.ixx)

An existing `keymap.map`, `components.list`, or Journal `<factory_name>.list` file is left unchanged. `*.local.ini` / `*.local.ixx` are gitignored and are not loaded.

Alias lines are `name = factory(arguments)`. `journal_ac.exe` loads those aliases into its hello catalog at startup. Restart Journal after a change; a new alias does not require a rebuild. A new factory requires a registry entry and a rebuild of `journal_ac.exe` and `journal_builder.exe`. Do not commit the live alias file. Per-program `.map` definitions live under the configured taskbar data directory (portable default gitignored `dist/taskbar/applications/`). `taskbar_builder.exe` creates a missing `.map` and does not overwrite an existing one. `taskbar_config.exe` writes only `config/taskbar.ini`. `--init` and `--seed` then launch the builder; `--seed` is noninteractive. After map generation succeeds, the builder launches `taskbar_ac.exe --refresh-cache`, which replaces `winkey_map.cache`. That cache is not missing-only. A keymap catalog failure does not skip the refresh. Version 1 `.map` syntax is section/key.

## Main process session

`auto_core.exe` constructs `ac::Component auto_core`, then
`ac::main::components::initialize()` reads `components.list`
once (or discovers `*_ac.exe` if that file is unreadable)
and starts each enabled generic child:

1. If `taskbar` is enabled, start it first (control pipe, hello, then
   snapshot `connect`). Snapshot failure keeps the child and skips native
   Win+position.
2. Start every other enabled hosted name: create `ac_{name}_pipe` and
   launch `{name}_ac.exe` without waiting, then wait for those
   `ac.component.v1` hellos in parallel (5s window). A name whose executable
   embeds `AC_LAUNCH_DESCRIPTOR` is not started. A missing exe, bad
   hello, or timeout disables only that child.
3. `initialize_keymap()` then registers advertised catalog names from
   started children, plus commands from each enabled executable's
   `AC_LAUNCH_DESCRIPTOR`.

The returned `Session` is RAII. `close_program()` sends v1 `shutdown` to every
generic child, then waits under the single deadline in `shutdown.ini` until
that process has exited and its component job is empty. The hello metadata
declares `termination_policy = graceful` (the default) or
`termination_policy = force_allowed`; Main never infers policy from a
component name. Only `force_allowed` uses `TerminateJobObject`. A hosted
component that cannot be assigned to its job is not started. A component
whose executable embeds `AC_LAUNCH_DESCRIPTOR` is not a session
child. When that name is enabled, Main registers the command names in the
resource and launches `{name}_ac.exe` on demand. Journal parameterized names
and print-choice aliases are advertised in the same hello catalog.

Process lifetime, hook, F-lock, and crash restart are in [main.md](main.md).

## Adding a new component

From-scratch Visual Studio steps (File → New → Project, no copy of an
existing child) are in [new-component.md](new-component.md).

Put the project at `app/components/<name>/`. Import
`..\..\..\msbuild\AutoCore.props` once (relative). Target name is
`<name>_ac` for short children (`dash_ac`, `example_ac`, `itunes_ac`,
`journal_ac`, `logger_ac`, `server_ac`, `simple_test_ac`, `slash_ac`,
`spotify_ac`, `taskbar_ac`, `wake_ac`, `writer_ac`). Leave `auto_core`,
`spotify_oauth`, `spotify_editor`, `spotify_db`, `itunes_db`, `journal_config`, `journal_builder`, `journal_clock`, `journal_db`, `journal_series`, `journal_cloud`, `server_config`, `server_editor`, `server_builder`,
`taskbar_config`, `taskbar_builder`, `writer_config`, and `writer_editor` unsuffixed.

Enable a v1 child in live `dist/components.list` (`name` or `name on`).
Advertise
commands in the child's hello catalog and bind them in live
`dist/keymap/keymap.map`. Main does not need a per-component protocol
file or `register_with` entry. An on-demand component is any listed name
whose executable embeds `AC_LAUNCH_DESCRIPTOR`. Dash and Slash
are the current on-demand components. Do not start a v1 session for a
component that embeds that resource.

Run `<name>_star.exe` to enable the component, or run `components_editor.exe`
with no arguments so a live catalog full-syncs
discovered `<name>_ac.exe` names that already have `config/<name>.ini`. The portable
`[components]` seed is [`defaults/components.list`](../defaults/components.list),
owned by [`app/main/shared/defaults.ixx`](../app/main/shared/defaults.ixx).
Do not put ordinary component names in `auto_core.dll`.

A clone does not need an `obj` junction. If you want intermediates
elsewhere, create a directory junction to `obj` yourself and keep it out
of git.

### Generic host smoke test (`simple_test`)

[`simple_test_ac.exe`](../app/components/simple_test/) verifies that Main
discovers a completely new child from the live list without rebuilding
`auto_core.exe`, `auto_core.dll`, or any existing component. It is
intentionally omitted from the portable
[`defaults/components.list`](../defaults/components.list) seed.

1. Build only this child (Release x64):

   ```powershell
   msbuild "app\components\simple_test\simple_test.vcxproj" /m /t:Build /p:Configuration=Release /p:Platform=x64
   ```

2. Append `simple_test` to live `dist/components.list`,
   or run `simple_test_config.exe` / `components_editor.exe`. Auto Core
   does not write that file.
3. Bind `print_simple_test` (no parentheses) in live
   `dist/keymap/keymap.map`, for example
   `numpad_3 = {print_simple_test, make_print_choice("42nd", true)}`.
4. Start the already-built `dist/bin/auto_core.exe`. Confirm the child prints
   startup lines (`component: simple_test`, `pipe: ac_simple_test_pipe`),
   `print_simple_test` appears in regenerated
   `dist/keymap/keymap_commands.txt`, and the bound key prints
   `this is print_simple_test() from within simple_test_ac.exe`.
