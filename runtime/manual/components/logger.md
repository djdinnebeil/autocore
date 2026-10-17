# Logger

Logger combines each component log into one daily file. Components still write their own logs. Logger only appends the new records.

The daily file and `merge.state` are stored in the shared log folder from `config\logging_policy.ini`. The default folder is `logs`, relative to the installation root. `logger.ini` does not choose that folder.

`auto_core_logging_cleanup.exe` reads and reports the shared `retention_days` and `max_directory_mb` values once after Auto Core reaches ready state. At this stage it does not scan, measure, or delete files.

## Configuration

`config\components\logger.ini`

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
| `merge.state` | How far each component log has been read. Logger writes this file. |

Source `{date}_<name>.log` files stay in each component's own log folder. Logger reads them and does not replace them.

## Component tools

| Tool | What it is for |
| --- | --- |
| `logger_ac.exe` | Merges component logs while it is running. `--once` merges one time and exits. |
| `logger_config.exe` | Edits `logger.ini`. |
| `logger_settings.exe` | Opens Logger configuration. |
