# Logger

Each executable writes its own logs under `logs/components/<name>/` while it runs:

- `{date}_{name}.log` is the comprehensive log
- `{date}_{name}.main.log` is an exact subset of that file

`logger_ac.exe` reads those `.main.log` files and appends new records to `logs/YYYY-MM-DD_main.log`. Components do not send log lines to it. The component name in the merged line is the parent directory, so `components/spotify_star/` becomes `[spotify_star]`.

When `merge_interval_seconds` is `1` or more and `components.list` has `logger` on, Main hosts `logger_ac.exe` on `ac_logger_pipe`. That process merges once at startup and again on each interval. On shutdown it wakes immediately, stops the worker, and exits. It does not merge again and it does not start another logger.

Main then stops the other children and waits for their final `.main.log` writes. If a hosted logger is still running, Main signals it to exit and waits a fresh `shutdown_timeout_ms`. It does not force-terminate that process. After the hosted logger has exited, `merge_logs_on_shutdown = on` starts a detached `logger_ac.exe --once` and does not wait. `--once` loads config, merges once, updates `merge.state`, and exits. A launch with no arguments still merges once when the pipe is absent. Any other connection failure is reported and does not merge.

At most one `logger_ac.exe` may access `merge.state` and the merged daily log files at a time. Main enforces this during shutdown by waiting for the hosted logger to exit before launching `logger_ac.exe --once`.

`merge_interval_seconds = 0` disables the hosted periodic logger without disabling `merge_logs_on_shutdown`; a detached `logger_ac.exe --once` may still run at shutdown. Shutdown `off` starts neither.

Offsets live in `logs/merge.state`. A missing state file rebuilds represented dates from the start. A truncated source, or a missing merged file whose bytes were already consumed, rebuilds that date. State advances only after the merged output for that call is written. If that state replace fails after the lines are already appended, the next merge can write those same records again.

`config/logger.ini` is host configuration. `logger_config.exe` lives under [`app/components/logger/config`](../app/components/logger/config) and is the only writer. Defaults are in [`app/main/shared/defaults.ixx`](../app/main/shared/defaults.ixx). When the file is missing, Main prints that `logger_config.exe` should be run and that built-in defaults are in use. The file is not created.

| Key | Default |
| --- | --- |
| `directory` | `logs` (relative to the installation root) |
| `merge_interval_seconds` | `60` (`0` disables the hosted logger; invalid values use `60`) |
| `merge_logs_on_shutdown` | `on` (`on`/`off`, with `true`/`false` aliases) |
| `write_logs_to_console` | `off` (`on`/`off`, with `true`/`false` aliases) |

`merge_interval_seconds` and `merge_logs_on_shutdown` are independent. `write_logs_to_console` adds console output to `log` and `log_main`. `print` and `log_print` still write to the console once.

See [Runtime configuration](configuration.md).
