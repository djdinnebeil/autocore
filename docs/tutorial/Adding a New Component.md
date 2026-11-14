# Adding a New Component

A short path for a hosted component under `app/components/<name>/`. This
tutorial requires an Auto Core source checkout; paths under `app\`, `devs\`,
and `scripts\` refer to that checkout rather than the installed directory. The
other notes in this set are listed in [Auto Core Tutorial](<../Auto Core Tutorial.md>).

A component, including a third-party component, ships one `<name>_ac.exe`. `--config` writes `config/components/<name>.ini`. `--settings` can enable or disable the component. `auto_core_components.exe` is the only writer of `components.list`. `app/components/slash/` shows a command catalog. The contract is in `devs/component-commands.md`.

Auto Core does not learn the component name when `auto_core.exe` is built. It discovers `*_ac.exe` and always launches `--run`, `--settings`, or `--config`.

Dash and Slash embed an `AC_LAUNCH_DESCRIPTOR` resource. They are started on demand and are outside this sequence.

## 1. Create the component tree

Add `app/components/<name>/` with `runtime/`, `config/`, `shared/`, and `<Name>.sln`.

Each nested `.vcxproj` imports `../../../../msbuild/AutoCore.props` once. The runtime target name is `<name>_ac`.

This step is done when `<Name>.sln` contains those projects.

Project creation and folder layout: `devs/new-component.md` and `devs/STRATEGY.md`.

## 2. Define runtime behavior and defaults

Put the portable INI text in `shared/defaults.ixx`. The `config/` executable owns `config/components/<name>.ini`.

`runtime/main.cxx` connects as the pipe client on `ac_<name>_pipe`, sends an `ac.component.v2` hello, and handles invoke and shutdown. If the INI is missing or unreadable, the runtime uses those defaults in memory and does not create the file.

This step is done when the runtime speaks that host protocol and `<name>_ac.exe --config` is the only writer of the INI.

Protocol and defaults: `devs/development.md`.

## 3. Advertise commands

Register plain command names, with no parentheses, in the hello catalog. A description is optional text after ` - ` and is not part of the name. See `app/components/slash/`.

After hello, Main records those commands and rewrites `dist/keymap/keymap_commands.txt`. Do not add the component name to `auto_core.dll` or to Main.

This step is done when the catalog lists the names a keymap can bind.

Registration order: `devs/development.md`.

## 4. Add the settings application

Add `settings/main.cxx` that calls `ac::component_settings::run("<name>")`, as `app/components/wake/settings/main.cxx` does, and compile it into `<name>_ac.exe`.

Editors, builders, and other helpers are optional sibling folders, and only when the component needs them.

This step is done when `<name>_ac.exe --settings` can enable or disable the component and does not own the INI.

## 5. Join the repository build and publish binaries

Add each production `.vcxproj` to `<Name>.sln` and to `app/AutoCore.sln`. A test project joins `<Name>.sln` and `app/Tests.sln`, which builds the complete test suite. Do not add a test project to `app/AutoCore.sln`.

From the repository root, `scripts/build-all.ps1` builds that solution Release x64 and publishes `bin/components/<name>_ac.exe` into `dist/components/`.

This step is done when `bin/components/<name>_ac.exe` exists and the same name is under `dist/components/`.

Build outputs: `devs/building.md`.

## 6. Enable the component and verify it

Run `<name>_ac.exe --config` so a missing `dist/config/components/<name>.ini` is written. Enable the name with `<name>_ac.exe --settings`. Restart `dist/app/auto_core.exe`.

This step is done when the host starts `<name>_ac.exe` on `ac_<name>_pipe` and any advertised command can be bound in `dist/keymap.map`.

Smoke checks: `devs/new-component.md`.
