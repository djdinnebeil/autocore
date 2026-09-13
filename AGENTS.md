# Agent map

Three areas: `app/core` = `auto_core.dll`, `app/main` = `auto_core.exe`,
`app/components` = child processes. IPC contracts live in `app/shared`.

Start in [docs/](docs/). Overview (do not restate): [README.md](README.md).
Architecture, packaging, or git workflow: read [docs/STRATEGY.md](docs/STRATEGY.md).
Do not restate it. Do not reopen locked decisions unless asked.

Production shape is **Release x64**. Link `auto_core.lib` from `out/core`
then `lib/`. No root `.sln`.

## Naming traps

- Short child exes use `_ac` (`dash_ac.exe`, `journal_ac.exe`,
  `logger_ac.exe`, `server_ac.exe`, `slash_ac.exe`, `taskbar_ac.exe`,
  `wake_ac.exe`, `writer_ac.exe`). Keep `auto_core.exe`,
  `spotify_oauth.exe`, `journal_config.exe`, `server_config.exe`,
  `taskbar_config.exe`, and `writer_config.exe` unsuffixed.
- dir `spotify/` / modules `spotify_*` / exe `spotify_ac.exe`
- dir `itunes/` / exe `itunes_ac.exe`
- file `core_config.ixx` / module `auto_core.core.config`
- file `main_log_client.ixx` / module `auto_core.core.logging.client`
- file `main_component.ixx` / module `auto_core.main.application`

## Do not read unless the task is that file

- `app/core/taskbar/taskbar.cxx`

## Interfaces first

Read `.ixx` and `app/shared/protocols/*.ixx` before implementations. Old names
(`ac_actions`, `numkey`, `itunes.ixx` under Main) are gone; use `*_component`.
