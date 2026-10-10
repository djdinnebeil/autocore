# Wake

Wake records the last system wake. History is written whether or not component logging is on.

## Configuration

`config\wake.ini`

`[wake]`

| Setting | Default | Values | Description |
| --- | --- | --- | --- |
| `directory` | `components\wake` | A directory path | Wake history folder. A relative path is relative to the installation root. An absolute path is used as stored. |
| `logging` | `on` | `on`, `off` | Controls logging for the Wake component. History files stay independent of this switch. |

## Files and data

These files live in the Wake directory.

| File | What it is for |
| --- | --- |
| `current.event` | The latest wake capture. |
| `previous.event` | The previous capture, used to tell whether the latest one changed. |
| `wake_events.log` | Append-only history. A capture is appended only when its text differs from the previous one. |

The first successful capture writes all three files. A failed or empty capture leaves an existing `current.event` in place.

`wake_config.exe` captures once, with `wake_ac.exe --snapshot`, when `current.event`, `previous.event`, or `wake_events.log` is missing. An existing file, including an empty one, is left as it is.

## Component tools

| Tool | What it is for |
| --- | --- |
| `wake_ac.exe` | Captures the last wake at startup, then keeps running. `--snapshot` captures once and exits. |
| `wake_config.exe` | Edits `wake.ini`. Initialization and seeding also fill a missing history file. |
| `wake_settings.exe` | Opens Wake configuration. |
