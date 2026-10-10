# Logger

Logger combines each component's main log into one daily file. Components still write their own logs. Logger only appends the new main-log records.

The daily file and `merge.state` are stored in the shared log folder from `config\logging.ini`. The default folder is `logs`, relative to the installation root. `logger.ini` does not choose that folder.

## Configuration

`config\logger.ini`

`[logger]`

| Setting | Default | Values | Description |
| --- | --- | --- | --- |
| `merge_interval_seconds` | `60` | `0` or a positive whole number of seconds | Seconds between merges. `0` turns off the periodic merge. The hosted Logger then waits until shutdown. |
| `merge_logs_on_shutdown` | `on` | `on`, `off` | After the hosted Logger exits, `on` runs one more merge. This is independent of the interval. |
| `logging` | `on` | `on`, `off` | Controls logging for the Logger component. |

A positive interval merges once when Logger starts, then again on each interval.

## Files and data

| File | What it is for |
| --- | --- |
| `YYYY-MM-DD_main.log` | The merged daily log in the shared log folder. |
| `merge.state` | How far each source main log has been read. Logger writes this file. |

Source `{date}_<name>.main.log` files stay in each component's own log folder. Logger reads them and does not replace them.

## Component tools

| Tool | What it is for |
| --- | --- |
| `logger_ac.exe` | Merges main logs while it is running. `--once` merges one time and exits. |
| `logger_config.exe` | Edits `logger.ini`. |
| `logger_settings.exe` | Opens Logger configuration. |
