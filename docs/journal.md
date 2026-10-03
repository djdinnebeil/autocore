# Journal

`journal_ac.exe` owns journal titles. It allocates episode numbers through
`journal_db.exe --serve` and does not open `series.db`. Alias files are data
owned by `journal_builder.exe`. Series counters live in `series.db`, owned
only by `journal_db.exe`. Settings live in `config/journal.ini`. Defaults are
`app/components/journal/shared/defaults.ixx`.

| Key | Default |
| --- | --- |
| `directory` | `components\journal` |
| `auto_select_new_series` | `on` |
| `remote_sync` | `off` |
| `logging` | `on` |

Only `journal_config.exe` writes `journal.ini`. `--seed` and `--init --seed`
are the same noninteractive walk. A missing INI is written from the compiled
text. An existing INI is left unchanged. The command then runs
`journal_builder.exe --seed`, `journal_cloud.exe --seed`,
`journal_clock.exe --seed`, `journal_db.exe --seed`, and
`journal_series.exe --seed`. `--init` walks the same stores. A missing INI
is prompted and written. An existing INI is skipped. It then runs those
executables with `--init`. Prompts are `Journal directory
[components\journal]:`, `Auto-select new series [on]:`, `Remote sync
[off]:`, and `Enable logging [on]:`. Enter accepts the bracketed value.
`cancel` or EOF returns `1` and does not write that store or launch the
remaining owners. A child that is missing, fails to start, or returns
nonzero stops the rest of the walk. Files already created stay in place.
`journal_series.exe` runs only after `journal_db.exe` succeeds. It rewrites
a missing `series.map` from the database. It does not invent snapshot text,
and it does not create `series.db`. No-argument launch configures the INI
and seeds missing alias files. It does not launch the cloud, clock,
database, or series tools. Initialization does not update `components.list`.

Missing or malformed: in-memory
defaults and `report_ini_unavailable`. `remote_sync` is read once when
`journal_ac.exe` starts. Trimmed `on` (any ASCII case) starts
`journal_cloud.exe --serve`. `off` and every other value do not start it and
do not talk to Firebase. A change applies the next time Journal starts.

`journal_db.exe` is the only Journal executable that opens SQLite. It runs as
`journal_db.exe --serve`. `journal_ac.exe` launches that process and talks to
it on `ac_journal_db_pipe` with `ac.journal.db.v2`. `journal_series.exe` uses
the same pipe. Each call connects and disconnects, so both can run together.
The pipe is a private Journal contract, not `ac.component.v1`.
`journal_db.exe` is not a component and is not listed in `components.list`.
Episode allocation commits the new counter before the service returns the
number. The reply is the canonical series name, the allocated episode, the
next count, and that series' padding. `journal_db.exe --serve` creates
`series.db` when it first opens a missing file, with an empty schema and no
series. `journals.db` is not read. `--seed` and `--init` add the recommended
series only when they create a missing `series.db`. An existing database is
preserved exactly and is not opened. That includes an empty database already
created by `--serve`. Initialization does not insert a series into it. Add
one with `journal_series.exe`.

`series.db` schema version 2 stores `id`, `name`, `next_episode`, and
`padding`. An older database gains `padding` in place. Migrated rows use
padding `0`. A series added in `journal_series.exe` stores the padding entered
with its name, and `next_episode` starts at `1`. Padding is the minimum
display width of an episode number. A wider number is never truncated. `9` with padding `2`
displays as `09`. `123` with padding `2` stays `123`. When `series.db` is
missing, `--seed` adds `Auto Core` with padding `2`, so the next episode is
`1` and the generated snapshot line is that name plus `01`. `--init` asks
`Series name [Auto Core]:` and `Padding [2]:` before `journal_db.exe`
creates the file. `journal_series.exe` then refreshes a missing `series.map`
from the rows `journal_db.exe` returns. With no active name stored yet, that
refresh selects the latest series. `auto_select_new_series` still applies
when a series is added from the series menu.

The active series is the first line of `series.map` in the journal data
directory:

```text
active = Auto Core

[snapshot]
Personal 3
Auto Core 05
Research 009
```

Editing `active = Name` is how a series is selected. `journal_ac.exe` reads
that line and asks `journal_db.exe` to allocate it. A missing, blank, or
malformed line allocates the most recently added series (`ORDER BY id DESC`).
A name that is not a series is requested once, then retried once with that
same fallback. No other database error is retried. If there are no series,
allocation reports that and does not invent one.

`[snapshot]` lists each series and its next episode, padded for reading. It
is not written back to `series.db`. `journal_series.exe` is the only writer
of `series.map`. It rewrites the file only when Refresh is selected, the
active series changes, a series is added, a count changes, or padding
changes. Allocating an episode from `journal_ac.exe` leaves the snapshot
unchanged on purpose.

`auto_select_new_series` applies only after `journal_db.exe` creates a series.
Trimmed ASCII `on` and `off` are the accepted values. A missing key or any
other value stays `on`. `on` rewrites `active` to the canonical name from
the database. `off` keeps the current `active = Name`. Either way the
snapshot is regenerated. Selecting a series, editing `active` by hand,
changing a count or padding, and Refresh do not use this setting. A failed
add leaves `series.map` unchanged.

`firebase.id` in the journal data directory is one Firebase REST endpoint
string. It has no INI section, no `key = value` syntax, and no quotes.
Leading and trailing whitespace is trimmed, and a trailing newline is
allowed. Blank content means Firebase is not configured. An additional
nonblank line is malformed. `journal_cloud.exe` is the only reader and
writer, and it does not rename an existing `firebase.url`. With no arguments
it edits `firebase.id`. `--seed` creates a missing empty file and does not
contact Firebase. `--init` prompts `Firebase ID []:` when the file is
missing. Enter creates that empty file. A value is checked with the same
rules as a URL write and is stored without a remote request. An existing
file, including an empty one, is left unchanged. `journal_cloud.exe --serve`
is the long-running cloud process. It
does not prompt. On each push it reads `firebase.id` and `PUT`s
`{"name":"...","count":N}`, where `count`
is the next episode after the one just allocated. Padding, SQLite ids, and
local paths are not sent. A missing URL or a failed request is logged and
does not undo the local episode. Retrieval into `series.db` is not
implemented.

Alias definitions are one `<factory_name>.list` file per compiled factory
under the journal data directory:

```text
components\journal\make_print_choice.list
components\journal\print_and_insert_into_journal.list
components\journal\roll_dice.list
```

`journal_builder.exe` is the only writer. With no arguments it adds, edits,
and deletes aliases. `journal_builder.exe --seed` creates each missing
starter file and leaves an existing file unchanged. The starter lines are
`print_auto_core_choice = make_print_choice("Auto Core", 1)`,
`print_hello_journal = print_and_insert_into_journal("Hello Journal!")`,
and `r7d8 = roll_dice(7, 8)`. `--init` prompts for each missing file, with
those values as the defaults. Enter keeps them. An existing file is skipped.

`journal_ac.exe` reads the files at startup, materializes each alias, and
advertises the alias name. It does not rewrite the files. A malformed alias
is logged and skipped. Restart Journal after a change:

```text
alias change -> restart Journal -> new catalog
```

Extended timestamps come from `extended_hour.clock` in the journal data
directory. The portable default is `components\journal\extended_hour.clock`
(`dist/components/journal/extended_hour.clock` when Auto Core runs from
`dist`). A custom `[journal] directory` in `config/journal.ini` moves the
file with the rest of the journal data. The file is one line,
`extended_hours = <token>`, with no INI section. Valid tokens are `0`, `+0`,
`1`, `+1`, through `12`, `+12`. `n` extends while `hour < n`. `+n` does the
same and also renders exactly `n:00` as hour `24 + n`. `journal_clock.exe` is
the only writer. If `config/journal.ini` is missing, it runs
`journal_config.exe` first so the data directory exists. `--seed` writes
`extended_hours = +0` when the clock file is missing. `--init` prompts
`Extended hours [+0]:` when it is missing. An existing clock file is left
unchanged. `journal_ac.exe` reads
the token and does not rewrite the file. A missing or malformed file behaves
as token `0`.

A new alias of an existing factory does not require a rebuild. A new factory
is a new entry in `journal_factories` plus a rebuild of `journal_ac.exe` and
`journal_builder.exe`. Direct factory keymap expressions such as
`make_print_choice("Tabby", 0)` stay in the catalog.

`journal_star.exe` launches the builder as **Manager journal builder**,
`journal_series.exe` as **Manage series**, `journal_cloud.exe` as
**Configure Firebase**, and `journal_clock.exe` as **Configure extended
hours**. Star does not write `series.map`, `firebase.id`, or `series.db`.

See [Runtime configuration](configuration.md).
