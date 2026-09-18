# Local file server

`server_ac.exe` serves a local folder over HTTP so a web browser on this
computer can open those files. Auto Core starts it with the other helper
processes and stops it with v1 `shutdown`.

## Configuration

Settings are in `dist/config/server.ini`. The tracked sample is
[`defaults/config/server.ini`](../defaults/config/server.ini). Auto Core
does not read `defaults/`.

| Key | Default | Meaning |
| --- | --- | --- |
| `port` | `8585` | Loopback TCP port (`1`–`65535`). Invalid or missing values keep `8585`. |
| `document_root` | `server` | Folder to serve. A relative path is resolved against the executable directory, so `server` is `dist/server` when Auto Core runs from `dist`. An absolute path is used as-is. Missing, empty, or absent file keeps `server`. |

The live file is under gitignored `dist/`. Missing keys or a missing file keep `server` and
`8585`. A missing live file is written once from those defaults. Keep a
machine-specific `document_root` in the live file only. `server_config.exe`
writes the live file if it is still missing (prompting for both keys) and
can later rewrite `port` or `document_root` from its menu without discarding
the other key. Those rewrites are explicit config-tool writes, not runtime
repairs. Restart `server_ac.exe` after changing the live file. See
[Server config](server_config.md).

The listener is `http://127.0.0.1:<port>/`. It does not accept LAN
connections. There is no authentication. Do not point `document_root` at
secrets.

If the configured folder is missing, the process still starts and logs that
the document root is missing.

## Using the document root

`/` serves `index.html` from the document root. Directory listing is on for
subfolders.

To share files:

1. Put files in `dist/server` (or the configured `document_root`).
2. Add a directory junction or symlink to another folder (`mklink /J` or
   `mklink /D`).
3. Or set `document_root` to an absolute path.

[`defaults/server/`](../defaults/server/) is the shipped document root
(`index.html`, `styles.css`). Building `server_ac.exe` copies those files
into `dist/server/`. Extra files and folders under `dist/server/` stay
local.

## Lifecycle

Main starts `server_ac.exe` as a generic `ac.component.v1` child when `server`
is enabled in `components.list`. Hello is sent before the HTTP bind. Shutdown
sends v1 `shutdown` on `ac_server_pipe`. A leftover process still holding
port `8585` is logged after hello; it does not stall Main's hello wait.
