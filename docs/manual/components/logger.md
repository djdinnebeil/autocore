# Logger

Logger combines each component log into one daily file. Components still write their own logs. Logger only appends the new records.

The daily file and `merge.state` are stored in the shared log folder from `config\logging_policy.ini`. The default folder is `logs`, relative to the installation root. `logger.ini` does not choose that folder.

`auto_core.exe` deletes `.log` files in the shared log folder from a background thread. It runs once after Auto Core is ready, then at each local midnight. It uses the logging configuration resolved when the host started, so edits to `logging_policy.ini` apply after the host restarts. Retention removes files whose last write time is strictly older than `retention_days` elapsed UTC days. Size cleanup then deletes the oldest remaining files until `max_directory_mb` is met. Files named with today's local date are kept and still count toward that size. `merge.state` is not a log and is not deleted.

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
| `logger_ac.exe` | Logger's executable. With no arguments, opens settings when `logger.ini` exists. When that file is missing, runs initialization and then opens settings only if initialization exits 0. |
| `logger_ac.exe --run` | Merges component logs while it is running. `--run --once` merges one time and exits. `--run --shutdown` only labels a `--once` launch. |
| `logger_ac.exe --settings` | Opens the Logger settings menu. |
| `logger_ac.exe --config` | Edits `logger.ini`. `--config --init` and `--config --seed` initialize it. |
