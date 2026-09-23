# Security

## Reporting

If you find a vulnerability in Auto Core, please do **not** open a public
issue that includes exploit details. Email the copyright holders listed in
[LICENSE](LICENSE), or open a private GitHub security advisory on the
repository.

Include what you observed, the Auto Core version or commit, and Windows
build when you can. Do not send Dash vault files, `dist/spotify/` tokens, or
other secrets.

## What this software does

Auto Core installs a **low-level keyboard hook**. Configured keys are
swallowed and run as keymap commands. Antivirus and SmartScreen may flag
this. That is expected for a system keyboard manager; review the source
before you run a binary you did not build.

The process may run elevated so some components (notably iTunes COM
automation) can attach. Elevation mismatches are documented in
[README.md](README.md) and [docs/itunes.md](docs/itunes.md).

## Secrets and local data

`dist/` is gitignored. Do not share or copy:

- `dist/spotify/` — Spotify tokens (`spotify_tokens.ini`, `spotify_codes.ini`)
- `dist/journal/` — `journals.db` and `journal_choices.ini`
- Dash vault at `%LOCALAPPDATA%\Auto Core\dash.vault`
- `.env` files, logs, crash dumps, writer notes, or a private
  `bindings.local.ini`

Dash is **not** a password manager. See [docs/dash.md](docs/dash.md).
Spotify tokens are machine-local; see [docs/spotify.md](docs/spotify.md).

## Local file server

`server_ac.exe` serves `[server] document_root` on **loopback only**
(`http://127.0.0.1:<port>/`). There is no authentication. Do not point
`document_root` at secrets. Do not change the bind to a LAN address.
See [docs/server.md](docs/server.md).

## Privilege and IPC

Named pipes and child processes are local Windows IPC. Do not expose them
on the network. Do not weaken integrity-level boundaries to make a helper
easier to attach.
