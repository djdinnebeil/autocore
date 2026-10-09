# Strategy

Living document: current truth only. Rewrite a section to change it; do not
append history. Do not copy this file into `AGENTS.md`. Do not reopen a
locked decision unless asked, and then rewrite that section in a new style.

## Goals

Maximize efficiency (runtime, build, agent tokens, git). Measured trade-offs
are allowed; about 1 ms is acceptable for a runtime-only configuration mode.
That figure is a budget, not a recorded benchmark.

Start recommendations from modern practice, then apply these goals. If a
section below already decides it, do not re-explore.

## Agent work

- **Tokens.** Always-on map is `AGENTS.md` only: layout and where to read
  next. Thoroughness for developing Auto Core lives in `devs/`. Thoroughness
  for using, configuring, and installed runtime behavior lives in `docs/`.
  Do not copy per-component executable
  lists, module stems, or retired names into it. Add Cursor glob rules only
  after a repeated mistake is cheaper to encode than to rediscover.
- **Git.** Feature work on a new branch; close by merging to `main`. No
  force-push to `main`. Do not commit unless asked. A private remote is
  allowed for a personal clone after the new `git init`; public remotes
  stay later.
- **Scope.** Recommend and explore when a section is not locked. Once locked,
  implement or stop.

## Locked: build outputs

- `obj/` — disposable `IntDir` (compiler objects and test build output).
- `lib/` — canonical `auto_core.dll` and `auto_core.lib` (`OutDir` for the
  core DLL). Tracked. The linker searches `lib/` only. The linker also
  writes `auto_core.exp` here; that file is gitignored.
- `bin/` — tracked published binaries. Application `OutDir` is `bin/`.
  `PublishAutoCoreDll` in
  [`app/core/auto_core_dll.vcxproj`](../app/core/auto_core_dll.vcxproj)
  copies `lib/auto_core.dll` to `bin/auto_core.dll` after that project
  links. [`scripts/copy-vendor-dlls.ps1`](../scripts/copy-vendor-dlls.ps1)
  copies `third_party/*/bin/*.dll` into `bin/` and does not run during a
  normal build.
- `dist/bin/` — ignored runtime copy. [`scripts/publish-dist.ps1`](../scripts/publish-dist.ps1)
  is the only normal publisher. It copies `bin/*.exe` and `bin/*.dll`
  into `dist/bin/`. `build-all.ps1` runs it after a successful build.
  The vendor script runs it after copying into `bin/`. `dist/` remains
  the installation root.
- `symbols/` — gitignored linker program databases (`.pdb`). Outside
  `dist/`. `AutoCoreSymbolsDir` in `AutoCore.props`.
- `third_party/<dependency>/` — the single vendor tree, not generated core
  outputs. Product packages use `include/`, `lib/`, `bin/` per package.
  Catch2 is amalgamated test source at `third_party/catch2/`.

Details stay in [building.md](building.md). Do not copy that catalog here.

## Locked: clone and runtime paths

A clone keeps the repo layout. Paths are relative to that layout, not to a
machine-specific drive letter.

- **Build.** [`msbuild/AutoCore.props`](../msbuild/AutoCore.props) lives one
  directory below the repo root. It sets `AutoCoreRootDir` from
  `MSBuildThisFileDirectory` plus one `..\` and `GetFullPath`.
  `AutoCoreAppDir` is `$(AutoCoreRootDir)app\`. Each `.vcxproj` imports the
  props file explicitly. A clone must not edit it.
- **Runtime.** [`app/core/config/paths.ixx`](../app/core/config/paths.ixx)
  derives `bin_directory` from the running image (`dist/bin` after a normal
  build) and `installation_root` as that directory's parent (`dist/`).
  Configuration, `components.list`, `keymap.map`, and runtime data resolve
  from `installation_root`. Child executables resolve from `bin_directory`.
  The process current working directory is never the path base. End users of
  a `dist/` tree do not need the repo.
- **Clone caveats.** Windows 11 and Visual Studio 2026 (version 18+) with
  C++23. `bin/` is the tracked published binary directory. `obj/` and
  `dist/` are gitignored. Link `auto_core.lib` from
  `lib/`. Keep `msbuild/` at the repo root.

## Locked: two audiences

Developers clone and build. End users run a configured `dist/` tree. There is
no installer yet; see [README.md](../README.md). `dist/` is gitignored.
Portable defaults are compiled in each child's `shared/defaults.ixx` and in
[`app/main/shared/defaults.ixx`](../app/main/shared/defaults.ixx) for Main
host INIs. Do not commit personal runtime data; see
[contributing.md](contributing.md).

## Locked: folder layout

Placement is settled. The nested tree stays in [README.md](../README.md)
and lists only directories that exist. Output semantics stay in Locked:
build outputs. Path derivation stays in Locked: clone and runtime paths.
Clone vs `dist/` stays in Locked: two audiences.

- `app/` — build input (source, projects, resources). Not source-only.
  [`app/AutoCore.sln`](../app/AutoCore.sln) is the repository workspace.
  Shared props are not here.
- `app/components/` — child executable projects that publish to `bin/`
  and run from `dist/bin/`.
  The component root holds one family `.sln` and responsibility folders:
  `runtime/` (`<name>_ac.exe`), `config/` (`<name>_config.exe`), and
  `shared/` (`<name>_protocol.ixx`, `defaults.ixx`). Spotify also has
  `oauth/` (`spotify_oauth.exe`). A `.vcxproj` is one executable. It does
  not get its own `.sln`. Unit tests live in `<name>/tests/`. Nested
  `.vcxproj` files import `..\..\..\..\msbuild\AutoCore.props`.
- `app/core/` — `auto_core.dll` source, `include/ac_api.hpp`,
  `shell_launcher/` (`auto_core_shell_launcher.exe`), and `tests/`.
  [`app/core/Core.sln`](../app/core/Core.sln) is the Core workspace.
- `app/main/` — Main family. [`app/main/Main.sln`](../app/main/Main.sln)
  is that workspace. `runtime/` is the `auto_core.exe` source tree only;
  optional component executables stay under `app/components/`.
  `editors/` holds executables that modify persistent operational data
  (`components_editor.exe` owns `components.list`, `keymap_editor.exe`
  owns `keymap.map`). `config/` holds configuration-management
  executables (`auto_core_config.exe` owns `config/auto_core.ini`,
  `components_config.exe` owns `config/components.ini`,
  `keymap_config.exe` owns `config/keymap.ini`,
  `crash_recovery_config.exe` owns `config/crash_recovery.ini`,
  `shutdown_config.exe` owns `config/shutdown.ini`). `shared/` is code,
  defaults, and helpers used by those executables and not generic enough
  for `app/shared/` or `auto_core.dll`. `_editor.exe` edits operational
  data. `_config.exe` manages settings. Those responsibilities stay
  separate.
- `app/shared/` — compile-time IPC protocols and `command_registry`.
  Runtime facilities stay in the core DLL.
- `app/resources/` — shared `.ico`/`.rc`. Stays under `app/`.
- `devs/` — developing Auto Core: contribution, building, architecture,
  maintenance, and internal project documentation. STRATEGY is current
  truth.
- `dist/` — gitignored assembled runtime for end users. Nested live
  dirs are documented in README, not locked here.
- `docs/` — using and configuring Auto Core, and installed runtime
  behavior.
- `lib/` — canonical `auto_core.dll` and `auto_core.lib`. See build
  outputs.
- `msbuild/` — `AutoCore.props`. One level below the repo root. Not
  `obj/`.
- `obj/` — gitignored `IntDir`, including test binaries. See build
  outputs.
- `scripts/` — repo build scripts and the manual vendor DLL refresh.
- `symbols/` — gitignored linker program databases. Outside `dist/`.
  See build outputs.
- `third_party/` — committed vendors. See build outputs for package
  shape.

Reserved names. Create the directory only with the first real item.
Do not add empty trees. Do not list them in README until they exist.
Do not copy these into `AGENTS.md`.

- `benchmarks/` — runnable performance benchmarks. Not shipped in
  `dist/`.
- `tests/` — repository-level integration and system tests. Nested
  Catch2 unit tests stay nested.
- `tools/` — developer utilities and standalone support programs. Not
  `app/components` and not `scripts/`.

## Locked: generic component host

A normal component is an executable that satisfies `ac.component.v1`.
Main must not know that component at compile time.

- `<installation_root>/components.list` `[components]` is an open catalog.
  Lines are `name`, `name on`, or `name off` (blank defaults to on;
  malformed values default to off). Names are case-sensitive and
  lowercase-only (`^[a-z][a-z0-9_]*$`). Invalid names are not normalized.
  An `AC_LAUNCH_DESCRIPTOR` RCDATA resource in `{name}_ac.exe` marks an
  on-demand component. Main registers those commands and does not start a v1
  session for that name. Dash and Slash embed that resource. The shared list
  parser does not name them. The list is the enable switch when the file is
  readable. `discover_ac_executables` returns every valid `*_ac.exe` name
  from `bin_directory`. `components_config.exe` owns
  `config/components.ini` (`[settings]` only: `new_components`,
  `sort_components`, `remove_missing_components`) and launches missing
  `<name>_config.exe` programs. `components_editor.exe` is the exclusive
  writer of `components.list`.   Child `_config.exe` programs write
  `config/<name>.ini` and do not launch `components_editor.exe`.
  They never write `components.ini` or `components.list`.
  `<name>_settings.exe` is the user-facing settings application. It may
  delegate to config, editors, builders, formatters, OAuth, and domain
  tools, and it may enable or disable the component. It does not own
  the INI. `auto_core_settings.exe` is the installation settings
  application. It delegates a missing `auto_core.ini` to
  `auto_core_init.exe`, then offers `auto_core_config.exe` and every
  installed `<name>_settings.exe` except itself. Discovery is the
  `*_settings.exe` filename, not `components.list` and not `*_ac.exe`.
  A missing
  `components.list` is reconstructible: Main launches no-arg
  `components_editor.exe` and fails startup if the list is still absent.
  Runtime scans `*_ac.exe` only when an existing `components.list` is
  unreadable.
- The name is the only launch input once listed: `weather` means
  `weather_ac.exe` in `bin_directory` and pipe `ac_weather_pipe`.
- v1 wire is hello/catalog, then Main-to-child `invoke = 0` plus one
  expression string, or `shutdown = 1`. Do not reuse or renumber those IDs.
  After hello, children do not send unsolicited runtime messages on this
  pipe. The catalog is immutable for the session.
- Main keeps each successful child's process handle for the session and
  stops generic children in reverse successful-start order.
- Logger, taskbar snapshot/cycling and INI `activate_*`, and
  Main-local commands stay explicit specials. On-demand components are
  described by the embedded `AC_LAUNCH_DESCRIPTOR` resource. Do not add a
  controller DLL, a second command catalog, component kinds, dependency
  graphs, restart, or catalog updates.

## Locked: component configuration

Every production child defines `dist/config/<name>.ini`, ships
`<name>_config.exe`, and keeps typed defaults in that child's
`shared/defaults.ixx`. Main host INIs are owned by helpers under
`app/main/config/`. `component_protocol` stays in `app/shared`.
Name-specific protocols live under that child's `shared/`.

- Only `_config.exe` writes `.ini` files. `auto_core.exe` and
  `<name>_ac.exe` never create or rewrite them. The presence of
  `auto_core.ini` means Auto Core is initialized. There is no
  `initialized` key. `auto_core.exe` checks that file first. If it is
  missing, the installation is new. `auto_core_init.exe` asks only
  for Auto Core configuration, then runs `auto_core_config.exe --seed`
  or `--init`. `logger_init.exe` and `components_init.exe` then make
  their own choices. The remaining owners are
  `keymap_config.exe --seed`, `components_config.exe --seed`,
  `shutdown_config.exe --seed`, `crash_recovery_config.exe --seed`,
  `components_editor.exe --seed`, and `keymap_editor.exe`.
  `logger_init.exe` launches `logging_config.exe` and
  `logger_config.exe` and does not write those files.
  `auto_core_config.exe` writes `auto_core.ini` from
  `[auto_core] warn_without_winkey_mapping` (default `on`) and
  `logging` (default `on`). `logging` is the family switch for
  `auto_core.exe`. If a required step fails or the user cancels that
  step, including Logger or Components initialization, and this run
  created `auto_core.ini`, the orchestrator removes the file before
  it exits. Only `auto_core_config.exe` creates or rewrites it.
  `config/components.ini` is written only by
  `components_config.exe`. `components.list` is written only by
  `components_editor.exe`. `components_init.exe` launches
  `<name>_config.exe` and does not write the list or any INI.
  `components_config.exe` may offer a no-arg `components_editor.exe`
  full sync after the user changes `components.ini`.
  `components_editor.exe --seed` creates a missing list and leaves an
  existing list unchanged.
  `components_editor.exe` reconstructs `components.list` from installed
  `*_ac.exe` names that already have `config/<name>.ini`, using INI
  settings or compiled defaults. Targeted `--component <name>` requires
  that INI to exist and does not prune missing names.
  `--component <name> --on` or `--off` rewrites that one entry to
  explicit `on` or `off` and does not rebuild the list. `<name>_settings.exe`
  reads the list and launches that command; it does not write
  `components.list`. There is no
  legacy `[components]` / `[list]` migration. Sync does not overwrite
  existing on/off values.
- If an INI is missing or malformed, runtime uses in-memory defaults and
  `log_print` (`Component::report_ini_unavailable` for children). The
  file is not created. Per-key invalid values in a readable file keep that
  key's default and do not rewrite the file.
- Portable INI text lives in each child's `shared/defaults.ixx`. Main host
  INI text (`auto_core`, `components`, `keymap`, `shutdown`,
  `crash_recovery`, `logging`, `logger`) lives in `app/main/shared/defaults.ixx`.
  `crash_recovery.ini` uses `[crash_recovery] default_response` (default `no`)
  and `crash_diagnostics` (default `on`). The latter is independent of logging
  and is initialized for every executable through the shared core DLL startup.
  `<name>_config.exe --seed` writes a missing live INI from that text.
- Nested project Target Names stay `<name>_ac`, `<name>_config`, and
  unsuffixed `spotify_oauth`.

## Locked: local executable logs

Each executable writes `{date}_{name}.log` and `{date}_{name}.main.log` under
`logs/components/<name>/`. `.main.log` is an exact subset of `.log`. The `nl`
logging names use the same routes and leave the record open.

`config/logging.ini` is the shared logging policy written by
`logging_config.exe` (`app/main/config/logging`). The canonical keys, in
file order, are `disable_all = off`, `directory = logs`,
`write_logs_to_files = on`, `write_logs_to_console = off`,
`log_print_mode = print`, and `component_logging_default = on`.
`disable_all` stops logging-controlled file and console output.
`write_logs_to_files` and `write_logs_to_console` are the two sinks.
`log_print_mode` classifies `log_print` as `log` or `print` before those
sinks. Family logging is `config/<scope>.ini` `[<scope>] logging`.
A normal missing key uses `component_logging_default`. Dash uses scope
`dash` with fallback off, so a missing key is off. A `Component` with no
scope does not read a component INI. A relative `directory` resolves from
the installation root. On a new installation, `logger_init.exe` asks
`logging_config.exe` to create a missing `logging.ini`. On an established installation, a missing
`logging.ini` keeps those defaults in memory. Main warns, and the file
is not created.

`config/logger.ini` is merger configuration written by `logger_config.exe`
(`app/components/logger/config`). The canonical keys are
`merge_interval_seconds` and `merge_logs_on_shutdown`. A missing file
keeps those defaults in memory. Main reports that and tells the operator
to run `logger_config.exe`. The file is not created. `logger_ac.exe`,
like every other executable, uses the shared `logging.ini` directory for
its local logs and for merged output. `logger.ini` controls only when
merging occurs. `logger_ac.exe` incrementally merges `.main.log`
files into `YYYY-MM-DD_main.log`. The hosted process does not merge on
shutdown. After it has exited, shutdown `on` starts a detached
`logger_ac.exe --once`. At most one `logger_ac.exe` may access
`merge.state` and the merged daily log files at a time. Main enforces
this during shutdown by waiting for the hosted logger to exit before
launching `logger_ac.exe --once`. `merge_interval_seconds = 0` disables
the hosted periodic logger without disabling `merge_logs_on_shutdown`;
a detached `logger_ac.exe --once` may still run at shutdown. Components
do not connect to it to send log lines.

## Current path

- Remaining packaging: run
  [`scripts/build-all.ps1`](../scripts/build-all.ps1) on this PC. After
  it succeeds, delete `.git` and start a new project (`git init`,
  `git add .`, first commit, private remote, push `main`). Public GitHub
  later.
