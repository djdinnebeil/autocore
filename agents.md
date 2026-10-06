# Agent map

`app/core` = `auto_core.dll`. `app/main/` is the Main family:
`runtime/` = `auto_core.exe` only, `init/` = first-run orchestrators
(`auto_core_init.exe`, `logger_init.exe`, `components_init.exe`; they do not write
configuration files), `editors/` = persistent operational-data
editors (`components_editor.exe` owns `components.list`,
`keymap_editor.exe` owns `keymap.map`), `config/` = configuration
executables (`components_config.exe`, `keymap_config.exe`, and the other
Main `_config.exe` helpers), `shared/` = Main-family code and defaults, `tests/` = the Main test
executable (`auto_core_main_tests`).
`app/components/` = optional child processes (`<name>/runtime`,
`<name>/config`, `<name>/star`, `<name>/shared`). Cross-subsystem
contracts are `app/shared` (`component_protocol`, `command_registry`,
`component_star`).
Name-specific protocols live in that child's `shared/`.

Start in [docs/](docs/). Overview (do not restate): [README.md](README.md).
Architecture, packaging, or git workflow: read [docs/STRATEGY.md](docs/STRATEGY.md).
Do not restate it. Do not reopen locked decisions unless asked.

Production shape is **Release x64**. Link `auto_core.lib` from `lib/`.
`app/AutoCore.sln` is the repository workspace. Family workspaces are
`app/core/Core.sln`, `app/main/Main.sln`, and one `<Name>.sln` in each
component directory. A `.vcxproj` is one executable and does not get its
own `.sln`.

## Naming traps

- Short child exes use `_ac` (`dash_ac.exe`, `journal_ac.exe`,
  `logger_ac.exe`, `server_ac.exe`, `slash_ac.exe`, `taskbar_ac.exe`,
  `wake_ac.exe`, `writer_ac.exe`). Keep `auto_core.exe` and config
  helpers unsuffixed (`<name>_config.exe`, `spotify_oauth.exe`, `spotify_editor.exe`, `spotify_db.exe`,
  `server_builder.exe`, `server_editor.exe`, `journal_builder.exe`, `journal_clock.exe`, `taskbar_builder.exe`, `itunes_formatter.exe`, `auto_core_shell_launcher.exe`, `components_config.exe`,
  `components_editor.exe`).
- `<name>_star.exe` is the user-facing management console for every
  production component. Enable/disable goes through
  `components_editor.exe`. The shared menu is `component_star`.
- dir `spotify/` nests `runtime` / `config` / `oauth` / `editor` / `db` / `star` / `shared`; exe
  `spotify_ac.exe`, `spotify_config.exe`, `spotify_oauth.exe`,
  `spotify_editor.exe`, `spotify_db.exe`, `spotify_star.exe`
- dir `logger/` nests `runtime` / `config` / `star`; exe `logger_ac.exe`,
  `logger_config.exe`, `logger_star.exe`
- dir `itunes/` nests `runtime` / `config` / `formatter` / `db` / `star` / `shared`; exe
  `itunes_ac.exe`, `itunes_config.exe`, `itunes_formatter.exe`,
  `itunes_db.exe`, `itunes_star.exe`
- dir `journal/` nests `runtime` / `config` / `builder` / `clock` / `db` /
  `series` / `cloud` / `star` / `shared`; exe `journal_ac.exe`,
  `journal_config.exe`, `journal_builder.exe`, `journal_clock.exe`,
  `journal_db.exe`, `journal_series.exe`, `journal_cloud.exe`,
  `journal_star.exe`
- dir `server/` nests `runtime` / `config` / `editor` / `builder` / `star` / `shared`; exe
  `server_ac.exe`, `server_config.exe`, `server_editor.exe`, `server_builder.exe`,
  `server_star.exe`
- dir `taskbar/` nests `runtime` / `config` / `builder` / `star` / `shared`; exe
  `taskbar_ac.exe`, `taskbar_config.exe`, `taskbar_builder.exe`,
  `taskbar_star.exe`
- file `core_config.ixx` / module `auto_core.core.config`
- file `main_log_client.ixx` / module `auto_core.core.logging.client`
- file `main_component.ixx` / module `auto_core.main.application`

## Do not read unless the task is that file

- `app/core/taskbar/taskbar.cxx`

## Interfaces first

Read `.ixx` and `app/shared/protocols/component_protocol.ixx` (plus the
child's `shared/<name>_protocol.ixx`) before implementations. Old names
(`ac_actions`, `numkey`, `itunes.ixx` under Main) are gone; use `*_component`.
