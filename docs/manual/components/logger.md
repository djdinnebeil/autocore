# Logger

Logger combines each component log into one daily file. Components still write their own logs. Logger only appends the new records.

The daily file and `merge.state` are stored in the shared log folder from `config\logging_policy.ini`. The default folder is `logs`, relative to the installation root. `logger.ini` does not choose that folder.

`auto_core.exe` deletes `.log` files in the shared log folder from a background thread. It runs once after Auto Core is ready, then at each local midnight. It uses the logging configuration resolved when the host started, so edits to `logging_policy.ini` apply after the host restarts. Retention removes files whose last write time is strictly older than `retention_days` elapsed UTC days. Size cleanup then deletes the oldest remaining files until `max_directory_mb` is met. Files named with today's local date are kept and still count toward that size. `merge.state` is not a log and is not deleted.

`auto_core.exe` also merges component logs on the interval in `[log_merge]` when `components.list` enables `log_merger`. `0` turns that timer off. `log_merger off` skips automatic merging. The merge reads each source up to the size it had when the pass started. Bytes written after that snapshot are picked up on a later pass, which may be the next Auto Core session after a detached shutdown merge.

## Configuration

`config\logging_policy.ini`

`[log_merge]`

| Setting | Default | Values | Description |
| --- | --- | --- | --- |
| `merge_interval_seconds` | `5` | `0` or a positive whole number of seconds | Seconds between merges while Auto Core is open and `log_merger` is on. `0` turns off the periodic merge. A missing section or key uses `5`. An explicit value, including `0` and `60`, is kept. |
| `merge_logs_on_shutdown` | `on` | `on`, `off` | After the other components stop, `on` runs one more merge when `log_merger` is on. This is independent of the interval. |

`auto_core_settings.exe --config --area logging-policy` edits this section. `components.list` names the master switch `log_merger`. `auto_core_components.exe --edit` adds that line when `app\auto_core_logger.exe` is present and the name is new. A saved `log_merger` line stays, including `log_merger off`, when that executable is missing. `--disable log_merger` can write the off line while the executable is missing. `--enable log_merger` needs the executable and an existing list.

`log_merger` off skips the periodic worker and the detached shutdown merge. The two policy values stay as saved. `log_merger` on lets those values control their own paths. An interval of `0` disables periodic merging and leaves the shutdown setting alone. A missing `log_merger` line, or a missing or unreadable `components.list`, leaves both automatic paths on. The choice is read once when Auto Core starts. A positive interval merges once when Auto Core is ready, then again on each interval. `log_merger` does not start a process. An entry named `logger` has no effect.

`auto_core_logger.exe --refresh` and `--merge` still run when `log_merger` is off.

## Files and data

| File | What it is for |
| --- | --- |
| `YYYY-MM-DD_main.log` | The merged daily log in the shared log folder. |
| `merge.state` | How far each component log has been read. Logger writes this file. |
| `merge.journal` | A publish that has not been recorded in `merge.state` yet. The next merge removes it. |

Each component appends to one daily file, `logs\components\<component>\YYYY-MM-DD_<component>.log`. Every area of that component uses the same file. A line names its area before the event when the area is not the file stem, so a writer editor line merges as `[timestamp] [writer] [writer_editor] [event] message`. Settings messages, initialization, the Auto Core, components policy, crash recovery, logging policy, shutdown, startup, and warnings configuration streams, Keymap, and Components append to `logs\components\auto_core\YYYY-MM-DD_auto_core.log`, the same daily file `auto_core.exe` uses. Taskbar areas append to `logs\components\taskbar\YYYY-MM-DD_taskbar.log`, including taskbar calls made from other executables. Logger reads those files and leaves them in place. The Main-log tag is the complete filename suffix after `YYYY-MM-DD_`, without `.log`. Files already written under an area folder, including earlier Settings, Keymap, and Components logs, stay there and remain eligible for merge and retention, with their existing filename tags. A refresh reads only bytes after the saved offset for each file, including unread entries already in the `auto_core` daily file, and leaves a line with no newline unread.

`auto_core_logger.exe --refresh` updates today's Main log and prints `Main log file updated.` when that succeeds, including when nothing new was appended. A failed update prints the merge error. `--merge` reads every date. `--merge --shutdown` is the same merge with a shutdown label. Main launches that command after the other component writers have stopped when `log_merger` and `merge_logs_on_shutdown` are both on, and does not wait for it.

When `merge.state` is missing, the merge replaces every Main log from the component logs. It does not append those sources from offset zero. After `merge.state` exists, `--refresh` reads only today's component logs. `--merge` still merges every date. Both paths take the same merge lock. Merge diagnostics are written to the daily `auto_core` log with the area `auto_core_logger`.

Logger writes `merge.journal` before it changes a Main log and writes `merge.state` after that change. If the state file is not replaced, or the process exits between the two, the next merge reads the journal before it uses saved offsets. A finished Main log keeps its new bytes and those offsets advance, so the records are not written again. A partial append is cut back to the previous size. A replacement Main log is published only after its new file is complete, so an interrupted replacement leaves the previous Main log in place.

## Component tools

| Tool | What it is for |
| --- | --- |
| `auto_core.exe` | Deletes old logs and runs the periodic merge while Auto Core is open. |
| `auto_core_logger.exe --refresh` | Updates today's Main log and exits. |
| `auto_core_logger.exe --merge` | Merges every date and exits. |
| `auto_core_logger.exe --merge --shutdown` | The detached final merge. `--shutdown` only labels `--merge`. |
