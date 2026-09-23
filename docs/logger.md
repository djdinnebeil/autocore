# Logger

`logger_ac.exe` is the central named-pipe logger. Enable it from
`config/components.ini` `[components]`. Settings live in `config/logger.ini`. Defaults are
`app/components/logger/shared/defaults.ixx`.

| Key | Default |
| --- | --- |
| `directory` | `logs` |
| `write_to_console` | `false` (`on`/`off`, with `true`/`false` aliases) |

Only `logger_config.exe` writes the INI. Missing or malformed:
`logger_ac.exe` reports with `report_ini_unavailable` and uses defaults.
Main/`logging_config` may apply the same in-memory defaults and does not
write the file.

Logger internals stay in pre-release field-testing/maintenance. See
[Strategy](STRATEGY.md) and [Runtime configuration](configuration.md).
