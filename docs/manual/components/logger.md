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
| `merge.journal` | A publish that has not been recorded in `merge.state` yet. The next merge removes it. |

Each component appends to one daily file, `logs\components\<component>\YYYY-MM-DD_<component>.log`. Every area of that component uses the same file. A line names its area before the event when the area is not the file stem, so a writer editor line merges as `[timestamp] [writer] [writer_editor] [event] message`. Settings messages, initialization, the Auto Core, components policy, crash recovery, logging policy, shutdown, startup, and warnings configuration streams, Keymap, and Components append to `logs\components\auto_core\YYYY-MM-DD_auto_core.log`, the same daily file `auto_core.exe` uses. Taskbar areas append to `logs\components\taskbar\YYYY-MM-DD_taskbar.log`, including taskbar calls made from other executables. Logger reads those files and leaves them in place. The Main-log tag is the complete filename suffix after `YYYY-MM-DD_`, without `.log`. Files already written under an area folder, including earlier Settings, Keymap, and Components logs, stay there and remain eligible for merge and retention, with their existing filename tags. A refresh reads only bytes after the saved offset for each file, including unread entries already in the `auto_core` daily file, and leaves a line with no newline unread.

A no-argument launch updates today's Main log, prints `Main log file updated.` when that succeeds, and then shows the menu. That line is also printed when nothing new was appended. A failed update prints the merge error and still shows the menu. The menu item `Update Main log` runs the same update and returns to the menu. `--settings` opens that menu without the startup update.

When `merge.state` is missing, Logger replaces every Main log from the component logs. It does not append those sources from offset zero. After `merge.state` exists, the startup update and `Update Main log` read only today's component logs. `--run` still merges every date. Both paths take the same merge lock.

Logger writes `merge.journal` before it changes a Main log and writes `merge.state` after that change. If the state file is not replaced, or the process exits between the two, the next merge reads the journal before it uses saved offsets. A finished Main log keeps its new bytes and those offsets advance, so the records are not written again. A partial append is cut back to the previous size. A replacement Main log is published only after its new file is complete, so an interrupted replacement leaves the previous Main log in place.

## Component tools

| Tool | What it is for |
| --- | --- |
| `logger_ac.exe` | Logger's executable. With no arguments, updates today's Main log, then opens settings when `logger.ini` exists. When that file is missing, runs initialization and opens settings only if initialization exits 0. A failed initialization does not update the log. |
| `logger_ac.exe --run` | Merges every component log while it is running. `--run --once` merges one time and exits. `--run --shutdown` only labels a `--once` launch. This path does not print the settings update line. |
| `logger_ac.exe --settings` | Opens the Logger settings menu without updating the Main log first. |
| `logger_ac.exe --config` | Edits `logger.ini`. `--config --init` and `--config --seed` initialize it. |
