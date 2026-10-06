# Local file server

`server_ac.exe` serves one document root over HTTP so a web browser on this
computer can open it. Auto Core starts it with the other helper processes
and stops it with v1 `shutdown`.

## Configuration

`config/server.ini` stores the Server data location and the logging
switch. Defaults are
[`app/components/server/shared/defaults.ixx`](../app/components/server/shared/defaults.ixx).
`server_config.exe --seed` writes a missing live file from that text, then
asks `server_editor.exe --seed` and `server_builder.exe --seed` to fill any
missing stores they own. An existing file is never overwritten.

| Key | Default | Meaning |
| --- | --- | --- |
| `directory` | `components\server` | Server data directory. A relative path resolves against the installation root, so `components\server` is `dist/components/server` when Auto Core runs from `dist`. An absolute path is used as-is. A missing or empty key uses `<installation_root>\components\server` in memory. |
| `logging` | `on` | Server family logging switch. |

The listening port and document root are not INI keys.

| File | Seed | Meaning |
| --- | --- | --- |
| `<directory>\port.id` | `8585` | Loopback TCP port (`1`–`65535`). One scalar. |
| `<directory>\document_root.id` | `site` | Document root. A relative value resolves under the Server data directory. An absolute path is used as-is. |

`directory = components\server` with `document_root.id` = `site` is
`components\server\site`. The same `site` file under
`directory = D:\AutoCoreServer` is `D:\AutoCoreServer\site`.
`document_root.id` = `C:\DJ\My Folder\` stays that absolute path.

The live INI is under gitignored `dist/`. Only `server_config.exe` writes
it. Missing or malformed: `server_ac.exe` calls `report_ini_unavailable`,
uses the compiled directory only to locate the data files, and does not
create the INI. `port.id` and `document_root.id` have no runtime
substitute. A missing or invalid scalar is `log_print` and the process
exits before hello. Restart `server_ac.exe` after changing the INI or
either scalar file.

The listener is `http://127.0.0.1:<port>/`. It does not accept LAN
connections. There is no authentication. Do not point the document root
at secrets. Files and directories reached through a symbolic link or
junction inside the document root are still served.

## `server_config.exe`

Source: `app/components/server/config`. Defaults: `app/components/server/shared/defaults.ixx`.

`server_config.exe` writes only `config/server.ini`, and only `directory`
and `logging`. It does not write `port.id`, `document_root.id`,
`components.list`, or the starter page. Those stores are created by
launching their owners.

- `--init` walks the whole Server initialization surface and skips stores
  that already exist. A missing `config/server.ini` prompts
  `Server directory [components\server]:` and `Enable logging [on]:`, then
  writes only that file. Blank accepts the shown default. It then launches
  `server_editor.exe --init`. When that returns `0`, it asks
  `Create default site files in the document root? [Y/n]:`. Enter means
  yes and launches `server_builder.exe --seed`. `n` is a successful choice
  and does not create either starter file. An existing `server.ini` is
  left unchanged and the walk continues. A failed or cancelled required
  prompt returns nonzero and does not launch the next owner. Declining the
  site question is not cancellation.
- `--seed` and `--init --seed` take the seed path and never prompt. A
  missing `config/server.ini` is written from the compiled defaults. An
  existing file is left unchanged. The walk then always launches
  `server_editor.exe --seed` and, when that returns `0`,
  `server_builder.exe --seed`.
- No arguments stay the settings menu. That path prompts only when the INI
  is missing, then edits `directory` and `logging`. It does not launch the
  editor or the builder.

After the file exists, the menu can print or set `directory` and
`logging`. Setting the directory rewrites `server.ini` only. It does not
copy or seed the new directory. The previous tree stays where it was.
Rerun `server_config.exe --init` or `--seed` to fill missing stores in
the directory named by the current file. A child that is missing or that
returns nonzero stops the later owners. Files already written stay.

Legacy `port` and `document_root` keys are ignored. The next rewrite
drops them. Nothing imports them, and nothing probes an old layout for
`port.id`, `document_root.id`, `index.html`, or `styles.css`.

`server_ac.exe` and Main never write this file.

See [Runtime configuration](configuration.md).

## `server_editor.exe`

Source: `app/components/server/editor`.

`server_editor.exe` is the only writer of `port.id` and
`document_root.id` in the active Server data directory. Each file is one
scalar. It does not write `server.ini` or the starter page.

- `--seed` creates a missing `port.id` (`8585`) and a missing
  `document_root.id` (`site`). An existing file is left unchanged, even
  when its text is invalid. `--init --seed` takes this path and does not
  prompt. If `server.ini` exists but cannot be read, `--seed` writes
  nothing and returns `1`.
- `--init` prompts only for a missing file: `Server port [8585]:` and
  `Document root [site]:`. Blank accepts the shown default. The port must
  be an integer from `1` to `65535`. The document root is one scalar path.
  An existing file is skipped. Cancelling a required prompt returns
  nonzero and leaves a file already written in that run.
- No arguments open a menu to show or set the port, show or set the
  document root, or open the resolved folder. Set port accepts `1`–`65535`
  and rewrites only `port.id`. Set document root accepts one relative or
  absolute path and rewrites only `document_root.id`. Opening the folder
  creates that directory when it is missing.

These seed values are not used by `server_ac.exe` when a file is missing
or invalid.

## `server_builder.exe`

Source: `app/components/server/builder`. Starter text:
[`app/components/server/shared/default_site.ixx`](../app/components/server/shared/default_site.ixx).

`server_builder.exe` is the only programmatic writer of the two starter
files. `--seed` is its only successful invocation. It reads
`config/server.ini` only to resolve `[server] directory`, then reads
`document_root.id`. It does not write `server.ini`, `port.id`, or
`document_root.id`.

The scalar must be a relative path that stays inside the Server data
directory. `site` writes `<data>\site\index.html` and
`<data>\site\styles.css`. `web` writes those files under `<data>\web`.
`..\other` is refused. An absolute value such as `C:\DJ\My Folder\` is
user-managed: the builder does not create that directory and does not
modify files there.

A missing, empty, or malformed `document_root.id`, or an unreadable
existing INI, writes nothing and returns `1`. The builder does not invent
`site`.

When a relative root is accepted, `--seed` creates that directory and
writes `index.html` and `styles.css` only when each file is missing. The
two files are independent: a present page is left byte-for-byte unchanged
while a missing stylesheet is created, and the reverse. An existing file
is never replaced or normalized. `site` is only the recommended value
used when `document_root.id` itself is missing. A stored value such as
`public` is resolved under the Server data directory, and the builder
writes there. A directory change does not copy starters from the previous
Server directory.

`server_config.exe --seed` launches this command after
`server_editor.exe --seed` returns `0`. `server_config.exe --init` asks
whether to create the default site and launches it only on yes, and only
after `server_editor.exe --init` returns `0`. If `document_root.id` was
not established, the builder is not launched. If the builder is missing
or fails, files already written stay. A later `--init` or `--seed` skips
those files and fills whatever is still missing.

## `server_ac.exe`

Source: `app/components/server/runtime`.

1. Resolve `[server] directory`.
2. Read and validate `port.id`. Missing, empty, malformed, or outside
   `1`–`65535`: `log_print` and exit before hello.
3. Read and validate `document_root.id`. Missing, empty, or not one
   scalar path: `log_print` and exit before hello.
4. Resolve a relative value under the Server data directory. Use an
   absolute value as-is.
5. If that directory exists, serve it. Missing `index.html` or
   `styles.css` does not create or replace those files.
6. If the directory is missing, `log_print` and exit before hello.
   A missing relative directory names `server_builder.exe --seed`.
   A missing absolute directory is not created. `server_ac.exe` does not
   launch `server_config.exe`, `server_editor.exe`, or
   `server_builder.exe`.

Hello is sent after those checks. A leftover process still holding the
configured port is logged after hello; it does not stall Main's hello
wait.

## `server_star.exe`

Source: `app/components/server/star`. The menu delegates and does not write
persistent files.

1. **Initialize** when `config/server.ini` is missing
   (`server_config.exe --init`). **Configure** when it exists
   (`server_config.exe` with no arguments). Configure stays the settings
   menu.
2. **Complete initialization** (`server_config.exe --init`) only when the
   INI exists and `port.id` or `document_root.id` is missing in the
   resolved Server directory. Missing `index.html` or `styles.css` does
   not show this item. After both scalar files exist, the item drops off
   the menu.
3. **Edit port and document root** (`server_editor.exe`).
4. **Create default site files** (`server_builder.exe --seed`).
   This is safe when a starter was removed from an existing relative
   document root: existing customized pages stay untouched. An absolute
   document root is refused.
5. **Enable** or **Disable**, which delegates to `components_editor.exe`.
6. **Exit**.

## Using the document root

`index.html` is the starter page for a relative document root. It links
to `styles.css`. The page says that this folder is the generated site and
that `server_editor.exe` chooses the document root.

To share files:

1. Keep `document_root.id` as a relative path such as `site` and put files
   in that folder, or replace the starter files.
2. Or set `document_root.id` to an absolute path. CivetWeb serves that
   directory directly. `server_builder.exe` will not modify it. If that
   directory is missing, `server_ac.exe` does not create it.

## Lifecycle

Main starts `server_ac.exe` as a generic `ac.component.v1` child when
`server` is enabled in `components.list`. Shutdown sends v1 `shutdown` on
`ac_server_pipe`.
