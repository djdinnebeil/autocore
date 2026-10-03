# Journal component tests

Journal tests use Catch2. They cover alias files, extended-hour tokens,
episode padding, `series.map` parsing and snapshot text, `remote_sync`
values, `auto_select_new_series` values, `firebase.id` text, the Firebase JSON body, and in-place
`series.db` migration. They do not start `journal_ac.exe`, open a Firebase
connection, or execute an alias action.

`series.map` snapshot text is rendered from database rows. Tests do not
treat snapshot lines as database input. Allocation does not rewrite
`series.map`. The initial-series test inserts `Auto Core` with padding `2`
through `journal_sqlite` and checks that `series_map::render` builds the
snapshot from that row.

`journal_builder.exe --seed` writes one demonstration alias into each missing
factory file. `journal_cloud.exe --seed` writes an empty `firebase.id`.
`journal_clock.exe --seed` writes `extended_hours = +0`. `journal_db.exe
--seed` adds the initial series only when `series.db` is missing.
`journal_series.exe` derives a missing `series.map` from the database.

The test binary links SQLite so it can migrate a temporary `series.db`.
Production `journal_ac.exe`, `journal_series.exe`, and `journal_cloud.exe`
do not.

From the repository root, build and run the tests with:

```powershell
msbuild app\components\journal\tests\journal_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64
.\obj\journal_tests\journal_tests.exe "[journal][unit]"
```
