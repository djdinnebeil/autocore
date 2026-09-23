# Journal

`journal_ac.exe` owns journal titles, series tables in `journals.db`, and
print-choice aliases. Settings live in `config/journal.ini`. Defaults are
`app/components/journal/shared/defaults.ixx`.

| Key | Default |
| --- | --- |
| `directory` | `journal` |
| `series` | empty (last series table) |
| `remote_sync` | `disable` |
| `day_rollover_hour` | `0` |

Only `journal_config.exe` writes `journal.ini`. Missing or malformed: in-memory
defaults and `report_ini_unavailable`. `journal_choices.ini` under the data
directory is **data**, not `journal.ini`; `journal_config.exe` still seeds a
missing choices file.

See [Runtime configuration](configuration.md).
