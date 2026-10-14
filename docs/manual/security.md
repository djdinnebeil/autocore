# Security

## Reporting

If you find a vulnerability in Auto Core, please do **not** open a public
issue that includes exploit details. Email the copyright holders listed in
`legal\license`, or open a private security advisory on the repository.

Include what you observed, the Auto Core version or commit, and Windows
build when you can. Do not send Dash vault files, Spotify tokens, or
other secrets.

## What this software does

Auto Core installs a **low-level keyboard hook**. Configured keys are
swallowed and run as keymap commands. Antivirus and SmartScreen may flag
this. That is expected for a system keyboard manager; review the source
before you run a binary you did not build.

Run Auto Core without Administrator privileges unless your setup requires
otherwise. Auto Core and applications it controls must run at compatible
Windows integrity levels. The iTunes requirements are documented in
[itunes.md](components/itunes.md).

## Secrets and local data

Do not share or copy:

- `components\spotify\tokens.map` or a populated `client.id`
- private data under `components\journal\`, including `series.db`,
  `series.map`, `firebase.id`, and alias `.list` files
- Dash vault at `%LOCALAPPDATA%\Auto Core\dash.vault`
- logs, crash reports, or Writer notes and task lists

Dash is **not** a password manager. See [dash.md](components/dash.md).
Spotify tokens are machine-local; see [spotify.md](components/spotify.md).

## Local file server

`server_ac.exe` serves the directory named by `document_root.id` on
**loopback only** (`http://127.0.0.1:<port>/`). `port.id` supplies the
port. There is no authentication. Files reached through a symbolic link
or junction inside that root are still served. Do not point the document
root at secrets. Do not change the bind to a LAN address. See
[server.md](components/server.md).

## Privilege and IPC

Named pipes and child processes are local Windows IPC. Do not expose them
on the network. Do not weaken integrity-level boundaries to make a helper
easier to attach.
