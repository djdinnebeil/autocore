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

- `out/obj/` — disposable `IntDir` (compiler objects and test build output).
- `out/core/` — generated `auto_core.lib`, `auto_core.exp`, and the DLL
  build copy (`OutDir` for the core DLL). After link, copy
  `auto_core.dll` into `dist/` and `lib/`, and `auto_core.lib` into
  `lib/`.
- `lib/` — tracked seed `auto_core.dll` and `auto_core.lib`. Linker
  searches `out/core/` then `lib/`.
- `dist/auto_core.dll` — runtime copy after the core DLL links, and via
  [`scripts/copy-dist-dlls.ps1`](../scripts/copy-dist-dlls.ps1)
  (`out/core` then `lib/`) on every project that imports
  [`app/build/AutoCore.props`](../app/build/AutoCore.props). That script
  also copies `third_party/*/bin/*.dll` into `dist/`.
- `third_party/<dependency>/` — the single vendor tree, not generated core
  outputs. Product packages use `include/`, `lib/`, `bin/` per package.
  Catch2 is amalgamated test source at `third_party/catch2/`.

Details stay in [building.md](building.md). Do not copy that catalog here.

## Locked: clone and runtime paths

Relative paths work for another Windows collaborator who keeps the repo
layout. They do not depend on a machine-specific root.

- **Build.** [`app/build/AutoCore.props`](../app/build/AutoCore.props) sets
  `AutoCoreRootDir` from `MSBuildThisFileDirectory` plus `GetFullPath`. The
  props file is two levels below the repo root. A clone must not edit it.
- **Runtime.** [`app/core/modules/paths.ixx`](../app/core/modules/paths.ixx)
  derives `config/`, `keymap/`, and related directories from the executable
  directory (`dist/` after a normal build), not from the source tree. End
  users of a `dist/` tree do not need the repo.
- **Clone caveats.** Windows 11 and Visual Studio 2026 (version 18+) with
  C++23. `out/` and `dist/` are gitignored. Link `auto_core.lib` from
  `out/core/` if present, else `lib/`. Keep `app/build/` where it is.
  Post-Link DLL copy runs Windows PowerShell (`powershell.exe`).

## Locked: two audiences

Developers clone and build. End users run a configured `dist/` tree. There is
no installer yet; see [README.md](../README.md). `dist/` is gitignored.
Tracked portable defaults live in [`defaults/`](../defaults/) (repo
`defaults/X` is the sample for runtime `dist/X`). Do not commit personal
runtime data; see [CONTRIBUTING.md](../CONTRIBUTING.md).

## Current path

- Done: Phase 0; living [docs/STRATEGY.md](STRATEGY.md) plus `AGENTS.md`
  pointer. Clone-vs-end-user `dist/` inventory is done (config seeds,
  `defaults/` mirror, `dist/` gitignored). Readiness docs, vendor runtime
  DLLs under `third_party/<dependency>/bin/`, and copy into `dist/` on
  build via `scripts/copy-dist-dlls.ps1`. Folder structure assessment: Catch2 at `third_party/catch2/`;
  `app/` is build-input (not source-only); tests stay nested; `app/build/`
  and `app/resources/` stay under `app/`. README tree matches disk.
- Next: run [`scripts/build-all.ps1`](../scripts/build-all.ps1) on this
  PC. After it succeeds, delete `.git` and start a new project (`git init`,
  `git add .`, first commit, private remote, push `main`). Public GitHub
  later. `keymap_config.exe` is parked (see [TODO.md](TODO.md)).
