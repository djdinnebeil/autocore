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
- `dist/auto_core.dll` — runtime copy from `lib/` via
  [`scripts/copy-dist-dlls.ps1`](../scripts/copy-dist-dlls.ps1) on every
  project that imports
  [`msbuild/AutoCore.props`](../msbuild/AutoCore.props). That script
  also copies `third_party/*/bin/*.dll` into `dist/`.
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
  derives `config/`, `keymap/`, and related directories from the executable
  directory (`dist/` after a normal build), not from the source tree. End
  users of a `dist/` tree do not need the repo.
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
- `app/components/` — child executable projects that ship in `dist/`.
  Unit tests stay nested under the component that owns them.
- `app/core/` — `auto_core.dll` source, `include/ac_api.hpp`, nested
  tests.
- `app/main/` — `auto_core.exe` project.
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

- `config/components.list` is an open catalog. Names are case-sensitive and
  lowercase-only (`^[a-z][a-z0-9_]*$`). Invalid names are not normalized.
  `logger`, `dash`, and `slash` may be listed; they are known specials, not
  v1 session children. List on/off is the enable switch. There is no
  filesystem scan of `*_ac.exe`.
- The name is the only discovery input: `weather` means `weather_ac.exe`
  next to `auto_core.exe` and pipe `ac_weather_pipe`.
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

## Locked: pre-release logger status

`logger_ac.exe` is feature-complete for the Auto Core pre-release and is in
field-testing/maintenance status. Its logging API integration, millisecond
source timestamps, per-component `.main.log` coverage, timestamp-aware named
pipe protocol, centralized record format, synchronization, daily rollover,
and lifecycle behavior are the pre-release implementation.

Keep that implementation stable. Change `logger_ac.exe` before release only
to address a concrete field-test defect, regression, reliability issue, or
unmet pre-release requirement. Do not add speculative features, refactors, or
optimizations during field testing.

`log_merger_ac.exe` is a separate future project. It is not part of finalizing
or maintaining the pre-release `logger_ac.exe` implementation.

## Current path

- Done: Phase 0; living [docs/STRATEGY.md](STRATEGY.md) plus `AGENTS.md`
  pointer. Clone-vs-end-user `dist/` inventory is done (config seeds,
  `defaults/` mirror, `dist/` gitignored). Readiness docs, vendor runtime
  DLLs under `third_party/<dependency>/bin/`, and copy into `dist/` on
  build via `scripts/copy-dist-dlls.ps1`. Folder layout is locked.
- Done: generic component host. A normal component is an executable that
  satisfies `ac.component.v1`; Main must not know that component at
  compile time. `components.list` is an open lowercase catalog. Discovery
  is the name only (`weather` → `weather_ac.exe` and `ac_weather_pipe`).
  The v1 control channel is hello/catalog, then Main-to-child `invoke` and
  `shutdown`. Logger, taskbar snapshot/cycling/`activate_*`, dash, slash,
  and Main-local commands stay explicit specials. `logger`, `dash`, and
  `slash` may be listed in `components.list`; that file is their enable
  switch. They are not v1 session children.
- Done for pre-release: `logger_ac.exe` logging upgrade. The implementation is
  feature-complete and has moved to field-testing/maintenance; further work is
  limited to concrete defects, regressions, reliability issues, or unmet
  pre-release requirements. `log_merger_ac.exe` remains future work.
- Remaining packaging: run
  [`scripts/build-all.ps1`](../scripts/build-all.ps1) on this PC. After
  it succeeds, delete `.git` and start a new project (`git init`,
  `git add .`, first commit, private remote, push `main`). Public GitHub
  later. `keymap_config.exe` is parked (see [TODO.md](TODO.md)).
