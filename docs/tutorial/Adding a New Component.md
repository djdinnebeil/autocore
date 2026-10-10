# Adding a New Component

A short path for a hosted component under `app/components/<name>/`. The other notes in this set are listed in [Auto Core Tutorial](<../Auto Core Tutorial.md>).

Follow `app/components/wake/` for the tree: `runtime/` (`wake_ac.exe`), `config/` (`wake_config.exe`), `settings/` (`wake_settings.exe`), `shared/defaults.ixx`, and `Wake.sln`. Wake does not advertise keymap commands. `app/components/slash/` shows a command catalog.

Auto Core does not learn the component name when `auto_core.exe` is built. `<name>_config.exe` writes only `config/<name>.ini`. `<name>_settings.exe` can enable or disable the component. `components_editor.exe` is the only writer of `components.list`.

Dash and Slash embed an `AC_LAUNCH_DESCRIPTOR` resource. They are started on demand and are outside this sequence.

## 1. Create the component tree

Add `app/components/<name>/` with `runtime/`, `config/`, `shared/`, and `<Name>.sln`.

Each nested `.vcxproj` imports `../../../../msbuild/AutoCore.props` once. The runtime target name is `<name>_ac`.

This step is done when `<Name>.sln` contains those projects.

Project creation and folder layout: `devs/new-component.md` and `devs/STRATEGY.md`.

## 2. Define runtime behavior and defaults

Put the portable INI text in `shared/defaults.ixx`. The `config/` executable owns `config/<name>.ini`.

`runtime/main.cxx` connects as the pipe client on `ac_<name>_pipe`, sends an `ac.component.v1` hello, and handles invoke and shutdown. If the INI is missing or unreadable, the runtime uses those defaults in memory and does not create the file.

This step is done when the runtime speaks that host protocol and `<name>_config.exe` is the only writer of the INI.

Protocol and defaults: `devs/development.md`.

## 3. Advertise commands

Register plain command names, with no parentheses, in the hello catalog. See `app/components/slash/`.

After hello, Main records those names and rewrites `dist/keymap/keymap_commands.txt`. Do not add the component name to `auto_core.dll` or to Main.

This step is done when the catalog lists the names a keymap can bind.

Registration order: `devs/development.md`.

## 4. Add the settings application

Add `settings/` whose `main.cxx` calls `ac::component_settings::run("<name>")`, as `app/components/wake/settings/main.cxx` does.

Editors, builders, and other helpers are optional sibling folders, and only when the component needs them.

This step is done when `<name>_settings.exe` can enable or disable the component and does not own the INI.

## 5. Join the repository build and publish binaries

Add each `.vcxproj` to `<Name>.sln` and to `app/AutoCore.sln`.

From the repository root, `scripts/build-all.ps1` builds that solution Release x64 and publishes `bin/*.exe` into `dist/bin/`.

This step is done when `bin/<name>_ac.exe`, `bin/<name>_config.exe`, and `bin/<name>_settings.exe` exist, and the same names are under `dist/bin/`.

Build outputs: `devs/building.md`.

## 6. Enable the component and verify it

Run `<name>_config.exe` so a missing `dist/config/<name>.ini` is written. Enable the name with `<name>_settings.exe`. Restart `dist/bin/auto_core.exe`.

This step is done when the host starts `<name>_ac.exe` on `ac_<name>_pipe` and any advertised command can be bound in `dist/keymap.map`.

Smoke checks: `devs/new-component.md`.
