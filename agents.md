# Agent map

Three areas: `app/core` = `auto_core.dll`, `app/main/component` =
`auto_core.exe`, `app/main/config` = Main `_config.exe` helpers,
`app/main/components` = `components_config.exe` and `components_editor.exe`,
`app/components` = child processes (`<name>/main`, `<name>/config`,
`<name>/shared`). Generic IPC is `app/shared` (`component_protocol`,
`command_registry`). Name-specific protocols live in that child's `shared/`.

Start in [docs/](docs/). Overview (do not restate): [README.md](README.md).
Architecture, packaging, or git workflow: read [docs/STRATEGY.md](docs/STRATEGY.md).
Do not restate it. Do not reopen locked decisions unless asked.

Production shape is **Release x64**. Link `auto_core.lib` from `lib/`.
No root `.sln`.

## Naming traps

- Short child exes use `_ac` (`dash_ac.exe`, `journal_ac.exe`,
  `logger_ac.exe`, `server_ac.exe`, `slash_ac.exe`, `taskbar_ac.exe`,
  `wake_ac.exe`, `writer_ac.exe`). Keep `auto_core.exe` and config
  helpers unsuffixed (`<name>_config.exe`, `spotify_oauth.exe`,
  `components_config.exe`, `components_editor.exe`).
- dir `spotify/` nests `main` / `config` / `oauth` / `shared`; exe
  `spotify_ac.exe`, `spotify_config.exe`, `spotify_oauth.exe`
- dir `itunes/` nests `main` / `config` / `shared`; exe `itunes_ac.exe`,
  `itunes_config.exe`
- file `core_config.ixx` / module `auto_core.core.config`
- file `main_log_client.ixx` / module `auto_core.core.logging.client`
- file `main_component.ixx` / module `auto_core.main.application`

## Do not read unless the task is that file

- `app/core/taskbar/taskbar.cxx`

## Interfaces first

Read `.ixx` and `app/shared/protocols/component_protocol.ixx` (plus the
child's `shared/<name>_protocol.ixx`) before implementations. Old names
(`ac_actions`, `numkey`, `itunes.ixx` under Main) are gone; use `*_component`.
