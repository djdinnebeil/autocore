# Server tests

Release x64. Build the DLL first, or use the tracked `lib/auto_core.lib`. `build-all.ps1` does not build this project. The executable is `obj\server_tests\server_tests.exe`.

```powershell
msbuild app\components\server\tests\server_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64
.\obj\server_tests\server_tests.exe "[server][unit]"
```

`server_data_tests.cxx` covers `[server] directory` resolution, the `server.ini` text (`directory` and `logging` only), `port.id` bounds, one-line `document_root.id` parsing, relative resolution under the Server data directory, and builder refusal of absolute paths and `..\other`.

An existing document-root directory is served. A missing directory fails startup. Runtime does not launch `server_builder.exe`. Those choices do not look at `index.html` or `styles.css`.

`server_config.exe` rewrites `server.ini` from `ini_for` when that file is missing or when the settings menu saves it. `--init` and `--seed` then delegate missing `port.id` and `document_root.id` to `server_editor.exe`, and missing starter files to `server_builder.exe --seed`. `--init` asks before the builder; `--seed` does not. Changing `directory` from the menu rewrites `server.ini` only.
