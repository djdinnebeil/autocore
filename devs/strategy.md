# Strategy

Living document: current truth only. Rewrite a section when a decision changes; do not append history.

Do not copy this file into `AGENTS.md`. Do not reopen a locked decision unless asked.

## Goals

Maximize efficiency across runtime, build, agent tokens, and Git.

Start recommendations from modern practice, then apply the locked decisions below.

## Agent work

`AGENTS.md` is the always-on project map. Keep it concise.

Detailed development guidance belongs in `devs/`. User-facing configuration and installed runtime behavior belong in `docs/`.

Do not duplicate detailed executable catalogs, settings catalogs, retired names, or implementation history across documentation.

Recommend and explore when a decision is not locked. Once a decision is locked, implement it or stop.

## Git

Feature work uses a new branch and closes by merging to `main`.

Do not force-push `main`.

Do not commit unless asked.

A private remote is allowed for the personal clone after the next `git init`. Public remotes may be added later.

## Locked: build outputs

`obj/` is disposable build output and is gitignored.

`lib/` contains the canonical `auto_core.dll` and `auto_core.lib` produced by the core DLL project. It is tracked. Product linking uses `lib/`.

`bin/` contains tracked published binaries.

`dist/` is the gitignored assembled end-user runtime tree. `scripts/publish-dist.ps1` is the normal publisher from `bin/` to `dist/`.

`s​​ymbols/` contains gitignored linker PDBs and remains outside `dist/`.

`third_party/` is the committed vendor tree and is not used for generated Auto Core outputs.

Detailed build and publish behavior belongs in `devs/building.md`.

## Locked: paths and portability

Repository and runtime paths are derived from the project or running executable. They are not based on machine-specific drive letters or the process current working directory.

`msbuild/AutoCore.props` derives repository paths for builds.

Runtime path resolution derives the binary directory from the running image and the installation root from its parent.

A built `dist/` tree must run independently of the repository.

## Locked: audiences

Developers clone and build the repository.

End users run the assembled `dist/` tree.

Personal runtime configuration and operational data are not committed.

There is no installer contract yet.

## Locked: repository layout

The repository layout documented in `README.md` is authoritative.

Create reserved directories only when the first real item exists. Do not add empty directory trees.

Detailed output semantics belong in `devs/building.md`. Detailed architecture belongs in the appropriate `devs/` documentation.

## Locked: component architecture

A normal component satisfies `ac.component.v1`.

Main must not require compile-time knowledge of a normal component.

`components.list` is the runtime enablement catalog.

Once a component name is known, its runtime executable is `<name>_ac.exe` and its pipe is `ac_<name>_pipe`.

The v1 wire contract is fixed:

- hello/catalog
- invoke = `0`
- shutdown = `1`

Do not reuse or renumber those message IDs.

The command catalog is immutable for the session.

Main retains successful child process handles and shuts generic components down in reverse successful-start order.

On-demand components use the embedded `AC_LAUNCH_DESCRIPTOR` resource rather than a normal hosted v1 session.

Logger, Taskbar-specific behavior, and Main-local commands may remain explicit specials.

Do not introduce a controller DLL, second command catalog, component kinds, dependency graphs, runtime catalog updates, or automatic restart architecture without reopening this decision.

Detailed component behavior belongs in architecture documentation.

## Locked: ownership

Configuration executables exclusively own configuration files.

Editors exclusively own persistent operational data assigned to them.

Settings applications delegate to those owners and do not acquire file ownership.

Init executables orchestrate initialization and do not become configuration owners.

Runtime executables use in-memory defaults when configuration is unavailable and do not create or repair configuration files.

`config/auto_core.ini` is the initialization sentinel.

Detailed settings, defaults, initialization flow, command-line modes, and failure behavior belong in `docs/` and the relevant `devs/` documentation.

## Locked: logging architecture

Executables write their own local logs.

`logging_policy.ini` controls shared logging policy.

The Logger component merges existing log output; components do not send log records to Logger as a logging service.

Logging configuration files remain owned by their corresponding configuration executables.

Detailed keys, defaults, filenames, merge behavior, and user configuration belong in the logging documentation.

## Current path

Run `scripts/build-all.ps1` on this PC.

After a successful build:

1. Delete the existing `.git`.
2. Run `git init`.
3. Add the repository contents.
4. Create the first commit.
5. Add the private remote.
6. Push `main`.

Public GitHub publishing comes later.