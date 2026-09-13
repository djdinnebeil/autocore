# Server configuration

`server_config.exe` shares `config/server.ini` with `server_ac.exe` (no
separate config INI). If that live file is missing, the helper prompts for
the document root, suggesting `.\server` as the default to accept, then for
the port, suggesting `8585`. Blank document root stores
`[server] document_root = server`. Blank port stores `[server] port = 8585`.
An existing live file is not rewritten by that first-write helper.

Main and `server_ac.exe` never prompt. A missing live file is also written
once from those portable defaults when any process seeds `config/*.ini`
(same race as `journal.ini`: run `server_config.exe` first if you want the
first-write prompts). Invalid or missing keys keep `8585` and `<exe>/server`
in memory and do not rewrite the file.

After the INI exists, the helper shows a menu: print or set `port`, print or
set `document_root`, open the resolved document root in Explorer, or exit.
Set port rejects non-integers and values outside `1`–`65535` and re-prompts.
A port rewrite keeps the stored `document_root`. A document-root rewrite
keeps a valid stored `port`. Relative `document_root` values stay relative
in the file and resolve against the executable directory.

`server_ac.exe` reads the INI at process start. Restart it (or Auto Core)
after a menu rewrite.

See [Runtime configuration](configuration.md) and [Server](server.md).
