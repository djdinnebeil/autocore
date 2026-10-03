# Wake

`wake_ac.exe` records `powercfg /lastwake` once at startup and stays
connected as a v1 child. Its hello advertises no commands. `shutdown` stops
the process. Any invoke is logged as an unknown command.

`wake_ac.exe --snapshot` performs that same capture once and exits. It does
not connect to the pipe. Any other argument fails.

`wake_config.exe` owns `config/wake.ini`. Defaults are
`app/components/wake/shared/defaults.ixx`.

```ini
[wake]
directory = components\wake
logging = on
```

`directory` is the Wake history folder. A relative path resolves against the
installation root. `logging` is the ordinary component-log switch. It does
not control the history files.

`--seed` writes the compiled file when `wake.ini` is missing and leaves an
existing file unchanged. `--init` asks for the directory and logging when
the file is missing and leaves an existing file unchanged. `--init --seed`
is `--seed`. The no-argument menu edits those keys and does not capture
history.

After the INI step, `--init`, `--seed`, and `--init --seed` check whether
`current.event`, `previous.event`, and `wake_events.log` exist. Presence is
enough, including an empty file. When all three exist, config does not
launch Wake. When one is missing, config runs `wake_ac.exe --snapshot` and
returns its exit code. A missing `wake_ac.exe` leaves `wake.ini` in place
and returns `1`.

`wake_ac.exe` writes the three history files in `[wake] directory`:

- `current.event` — latest `powercfg /lastwake` capture
- `previous.event` — the comparison baseline, the same body after a capture
- `wake_events.log` — append-only history of changes

A first successful capture writes the captured body to `current.event` and
`previous.event`, and appends one timestamped entry to `wake_events.log`.
The same capture again does not append. A different body appends one entry
and replaces `previous.event`. `powercfg` finishes before `current.event` is
modified. A failed or empty capture leaves an existing `current.event` in
place and creates any missing history file empty.

Those files are Wake history. They are written whether or not ordinary
logging is enabled, and they do not follow `[logging] directory`.

`{date}_wake.log` and `{date}_wake.main.log` stay ordinary component logs
under `logs/components/wake/` and follow the shared logging policy, including `[wake] logging`.

Only `wake_config.exe` writes `wake.ini`. Missing or malformed: `wake_ac.exe`
reports, uses `components\wake`, and does not create the INI.

See [Runtime configuration](configuration.md).
