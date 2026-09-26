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

- **Tokens.** Always-on map is `AGENTS.md` only. Thoroughness lives in
  `docs/`. Add Cursor glob rules only after a repeated mistake is cheaper to
  encode than to rediscover.
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
- `dist/bin/auto_core.dll` — runtime copy from `lib/` via
  [`scripts/copy-dist-dlls.ps1`](../scripts/copy-dist-dlls.ps1) on every
  project that imports
  [`msbuild/AutoCore.props`](../msbuild/AutoCore.props). That script
  also copies `third_party/*/bin/*.dll` into `dist/bin/`. Application
  `OutDir` is `dist/bin/`. `dist/` remains the installation root.
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
- **Runtime.** [`app/core/modules/paths.ixx`](../app/core/modules/paths.ixx)
  derives `bin_directory` from the running image (`dist/bin` after a normal
  build) and `installation_root` as that directory's parent (`dist/`).
  Configuration, `components.list`, `keymap.map`, and runtime data resolve
  from `installation_root`. Child executables resolve from `bin_directory`.
  The process current working directory is never the path base. End users of
  a `dist/` tree do not need the repo.
- **Clone caveats.** Windows 11 and Visual Studio 2026 (version 18+) with
  C++23. `obj/` and `dist/` are gitignored. Link `auto_core.lib` from
  `lib/`. Keep `msbuild/` at the repo root. Post-Link DLL copy runs Windows
  PowerShell (`powershell.exe`).

## Locked: two audiences

Developers clone and build. End users run a configured `dist/` tree. There is
no installer yet; see [README.md](../README.md). `dist/` is gitignored.
Tracked portable defaults live in [`defaults/`](../defaults/) (repo
`defaults/X` is the sample for runtime `dist/X`). Do not commit personal
runtime data; see [CONTRIBUTING.md](../CONTRIBUTING.md).

## Locked: folder layout

Placement is settled. The nested tree stays in [README.md](../README.md)
and lists only directories that exist. Output semantics stay in Locked:
build outputs. Path derivation stays in Locked: clone and runtime paths.
Clone vs `dist/` stays in Locked: two audiences.

- `app/` — build input (source, projects, resources). Not source-only.
  No root `.sln`. Shared props are not here.
- `app/components/` — child executable projects that ship in `dist/bin/`.
  The component root holds folders only: `main/` (`<name>_ac.exe`),
  `config/` (`<name>_config.exe`), and `shared/` (`<name>_protocol.ixx`,
  `defaults.ixx`). Spotify also has `oauth/` (`spotify_oauth.exe`).
  Each executable has its own `.sln` next to its `.vcxproj`. Unit tests
  live in `<name>/tests/` with their own `.sln` when present. Nested
  `.vcxproj` files import `..\..\..\..\msbuild\AutoCore.props`.
- `app/core/` — `auto_core.dll` source, `include/ac_api.hpp`, nested
  tests.
- `app/main/` — Main family. `runtime/` is the `auto_core.exe` source
  tree only; optional component executables stay under `app/components/`.
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
- `defaults/` — tracked samples for runtime `dist/X`. Never loaded.
- `dist/` — gitignored assembled runtime for end users. Nested live
  dirs are documented in README, not locked here.
- `docs/` — developer documentation. STRATEGY is current truth.
- `lib/` — canonical `auto_core.dll` and `auto_core.lib`. See build
  outputs.
- `msbuild/` — `AutoCore.props`. One level below the repo root. Not
  `obj/`.
- `obj/` — gitignored `IntDir`, including test binaries. See build
  outputs.
- `scripts/` — repo build and post-Link copy scripts.
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
  `dash` and `slash` may be listed; they are known specials, not
  v1 session children. The list is the enable switch when the file is
  readable. `discover_ac_executables` returns every valid `*_ac.exe` name
  from `bin_directory`, including those specials; the catalog split keeps
  them out of v1 `enabled`. `components_config.exe` owns
  `config/components.ini` (`[settings]` only: `new_components`,
  `sort_components`, `remove_missing_components`) and launches missing
  `<name>_config.exe` programs. `components_editor.exe` is the exclusive
  writer of `components.list`. Child `_config.exe` programs write
  `config/<name>.ini` and register with
  `components_editor.exe --component <name>` after a successful write.
  They never write `components.ini` or `components.list`. A missing
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
- Logger, taskbar snapshot/cycling and INI `activate_*`, dash, slash, and
  Main-local commands stay explicit specials. Do not add a controller DLL,
  sidecar manifests, component kinds, dependency graphs, restart, or
  catalog updates.

## Locked: component configuration

Every production child defines `dist/config/<name>.ini`, ships
`<name>_config.exe`, and keeps typed defaults in that child's
`shared/defaults.ixx`. Main host INIs are owned by helpers under
`app/main/config/`. `component_protocol` stays in `app/shared`.
Name-specific protocols live under that child's `shared/`.

- Only `_config.exe` writes `.ini` files. `auto_core.exe` and
  `<name>_ac.exe` never create or rewrite them. The presence of
  `auto_core.ini` means Auto Core is initialized. There is no
  `initialized` key. `auto_core_config.exe` writes that file after
  `components_config.exe`, `keymap_config.exe`, `keymap_editor.exe`,
  `shutdown_config.exe`, and `crash_recovery_config.exe` succeed, and
  prompts for `[auto_core] warn_without_winkey_mapping` (default `true`).
  Only `auto_core_config.exe` creates or rewrites it.
  `config/components.ini` is written only by
  `components_config.exe`. `components.list` is written only by
  `components_editor.exe`. The config helper initializes missing
  `components.ini`, launches `<name>_config.exe` when `config/<name>.ini`
  is missing, and may offer a no-arg `components_editor.exe` full sync.
  `components_editor.exe` reconstructs `components.list` from installed
  `*_ac.exe` names that already have `config/<name>.ini`, using INI
  settings or compiled defaults. Targeted `--component <name>` requires
  that INI to exist and does not prune missing names.
  `--component <name> --on` or `--off` rewrites that one entry to
  explicit `on` or `off` and does not rebuild the list. `<name>_star.exe`
  reads the list and launches that command; it does not write
  `components.list`. There is no
  legacy `[components]` / `[list]` migration. Sync does not overwrite
  existing on/off values.
- If an INI is missing or malformed, runtime uses in-memory defaults and
  `log_print` (`Component::report_ini_unavailable` for children). The
  file is not created. Per-key invalid values in a readable file keep that
  key's default and do not rewrite the file.
- Tracked samples under `defaults/config/<name>.ini` must match
  `defaults.ixx`. Runtime never reads `defaults/`.
- Nested project Target Names stay `<name>_ac`, `<name>_config`, and
  unsuffixed `spotify_oauth`.

## Locked: local executable logs

Each executable writes `{date}_{name}.log` and `{date}_{name}.main.log` under
`logs/components/<name>/`. `.main.log` is an exact subset of `.log`. The `nl`
logging names use the same routes and leave the record open.

`config/logger.ini` is host configuration written by `logger_config.exe`
(`app/components/logger/config`). The canonical keys are `directory = logs`,
`merge_interval_seconds`, `merge_logs_on_shutdown`, and
`write_logs_to_console`. A missing file keeps those defaults in memory.
Main reports that and tells the operator to run `logger_config.exe`. The
file is not created. `logger_ac.exe` incrementally merges `.main.log`
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

- Done: Phase 0; living [docs/STRATEGY.md](STRATEGY.md) plus `AGENTS.md`
  pointer. Clone-vs-end-user `dist/` inventory is done (`defaults/`
  mirror, `dist/` gitignored). Readiness docs, vendor runtime
  DLLs under `third_party/<dependency>/bin/`, and copy into `dist/bin/` on
  build via `scripts/copy-dist-dlls.ps1`. Folder layout is locked.
- Done: generic component host. A normal component is an executable that
  satisfies `ac.component.v1`; Main must not know that component at
  compile time. `components.list` `[components]` is an open lowercase
  catalog (`name` / `name on` / `name off`). Launch is the name only
  (`weather` → `weather_ac.exe` in `bin/` and `ac_weather_pipe`). Discovery of
  `*_ac.exe` is used by `components_editor.exe` to reconcile the catalog when
  `config/<name>.ini` exists, and by runtime only when an existing
  `components.list` is unreadable. The v1 control channel is
  hello/catalog, then Main-to-child `invoke` and `shutdown`. Taskbar
  snapshot/cycling/`activate_*`, dash, slash, and Main-local commands stay
  explicit specials. `dash` and `slash` may be listed in `[components]`;
  that section is their enable switch. They are not v1 session children.
  Discovery includes those specials; the catalog split keeps them non-v1.
- Done: per-executable logs. `{date}_{name}.log` and `{date}_{name}.main.log`
  are local. `logger_ac.exe` merges the `.main.log` files.
  `logger_config.exe` writes `config/logger.ini`.
- Done: host vs component INI contract. Host files are written only by
  Main `_config.exe` programs. Component INIs are generated only by
  `<name>_config.exe`. Nested `main` / `config` / `shared` trees land per
  child session. Main lives under `app/main/runtime`,
  `app/main/editors`, `app/main/config`, and `app/main/shared`.
- Remaining packaging: run
  [`scripts/build-all.ps1`](../scripts/build-all.ps1) on this PC. After
  it succeeds, delete `.git` and start a new project (`git init`,
  `git add .`, first commit, private remote, push `main`). Public GitHub
  later.
