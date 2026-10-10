# Server

Server publishes one folder over HTTP on this computer only, at `http://127.0.0.1:<port>/`.

The port and the document root are files in the Server directory. They are not settings in `server.ini`.

## Configuration

`config\server.ini`

`[server]`

| Setting | Default | Values | Description |
| --- | --- | --- | --- |
| `directory` | `components\server` | A directory path | Folder that stores Server data. A relative path is relative to the installation root. An absolute path is used as stored. |
| `logging` | `on` | `on`, `off` | Controls logging for the Server component. |

## Files and data

These files live in the Server directory.

| File | Default | What it is for |
| --- | --- | --- |
| `port.id` | `8585` | The listening port. One integer from `1` to `65535`. |
| `document_root.id` | `site` | The folder that is served. A relative path is inside the Server directory, so `site` is `<server directory>\site`. An absolute path is used as stored. |

`server_builder.exe` creates missing `index.html` and `styles.css` in that document root. It writes those files only when the document root is a relative path that stays inside the Server directory. An existing file is left as it is. Changing `directory` does not move the old files.

Put the files you want to publish in the document root. For a relative root, that folder is under the Server directory. An absolute root is served directly, and `server_builder.exe` does not modify it. If that absolute folder is missing, Server does not create it.

Server does not start unless both `port.id` and `document_root.id` exist and the document root folder is present.

## Component tools

| Tool | What it is for |
| --- | --- |
| `server_ac.exe` | Serves the document root on `127.0.0.1`. |
| `server_config.exe` | Edits `server.ini`. Initialization can also create the port file, the document-root file, and the starter site. |
| `server_editor.exe` | Edits `port.id` and `document_root.id`. |
| `server_builder.exe` | Creates missing starter `index.html` and `styles.css`. |
| `server_settings.exe` | Opens configuration, the port and document-root editor, and starter-site creation. |
