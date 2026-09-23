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
| `document_root` | `components\server` | Folder to serve. A relative path is resolved against the installation root, so `components\server` is `dist/components/server` when Auto Core runs from `dist`. An absolute path is used as-is. Missing, empty, or absent file keeps `components\server`. |

The live file is under gitignored `dist/`. Only `server_config.exe` writes it
(prompts for both keys on first run; menu can rewrite either without
discarding the other). Missing or malformed: `server_ac.exe` uses
`shared/defaults.ixx` (`8585` / `server`), calls `report_ini_unavailable`,
and does not create the file. Restart `server_ac.exe` after changing the
live file.

The listener is `http://127.0.0.1:<port>/`. It does not accept LAN
connections. There is no authentication. Do not point `document_root` at
secrets.

If the configured folder is missing, the process still starts and logs that
the document root is missing.

## `server_config.exe`

Source: `app/components/server/config`. Defaults: `app/components/server/shared/defaults.ixx`.

If `config/server.ini` is missing, the helper prompts for document root
(`.\server`) then port (`8585`). Blank stores the defaults. After the file
exists, the menu can print or set `port`, print or set `document_root`, or
open the resolved folder. Set port rejects values outside `1`–`65535`.
Rewrites keep the other key.

`server_ac.exe` and Main never write this file.

See [Runtime configuration](configuration.md).

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
is enabled in `config/components.ini` `[components]`. Hello is sent before the HTTP bind. Shutdown
sends v1 `shutdown` on `ac_server_pipe`. A leftover process still holding
port `8585` is logged after hello; it does not stall Main's hello wait.
