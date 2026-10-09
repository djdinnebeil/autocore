# iTunes component

The iTunes component provides playback control, current-track formatting,
in-memory listening history, queue formatting, and optional track removal. It
runs as the separate `itunes_ac.exe` process and controls the desktop iTunes
application through its COM automation interface.

The refactored component uses lowercase `itunes` in code identifiers,
filenames, module names, configuration keys, and canonical runtime commands.
The product name remains **iTunes** in user-facing text and in the external COM
programmatic identifier `iTunes.Application`.

## Runtime architecture

```text
Auto Core keymap
    -> ac_itunes_pipe
    -> numeric or named command adapter
    -> command/runtime coordination
    -> itunes_client synchronous facade
    -> dedicated serial executor thread
    -> iTunes COM automation

playback monitor
    -> itunes_client synchronous facade
    -> the same serial executor thread
```

Auto Core owns the server side of the local named pipe and launches
`itunes_ac.exe`. There is no extra ready string. The component connects as the
client, sends `ac.component.v1` hello with its catalog, and processes invoke
and shutdown until Auto Core stops the child or the pipe fails.

Main hosts iTunes as a generic `ac.component.v1` child. Invoke and shutdown
share one per-child mutex on the control pipe.

All COM initialization, interface access, method invocation, interface release,
and `CoUninitialize` calls run on one dedicated owner thread. Commands and the
playback monitor wait synchronously for work submitted to that thread. This
keeps COM interface pointers within their owning execution context while
preserving the component's existing synchronous command behavior.

The playback monitor is a separate joinable thread. It samples the current
track and playback state at a maximum interval of approximately five seconds,
wakes early after relevant playback changes, and records a track only when its
formatted value changes. During shutdown, the component signals and joins the
monitor first, releases COM on the owner thread, and then joins the owner
thread. Shutdown is idempotent.

## Initialization and reconnection

If `auto_start` is enabled, the component attempts COM initialization
after logging is ready. Otherwise, the first command that requires iTunes
initializes it lazily.

Initialization is bounded to nine attempts with 250 milliseconds between
attempts. A failed retry sequence leaves the client uninitialized without
starting duplicate monitor threads; a later lazy operation can try again.

## Configuration

The component reads `dist/config/itunes.ini`:

```ini
[itunes]
directory = components/itunes
auto_start = on
logging = on
```

| Setting | Default | Behavior |
| --- | ---: | --- |
| `directory` | `components/itunes` | iTunes component-data directory. A relative path resolves against the installation root, so the default is `<installation_root>/components/itunes`. An absolute path is used as-is. A missing or empty value uses that default. The directory is not created when the INI is read or written. |
| `auto_start` | `on` | Only lowercase `on` or `off` is applied. Any other present value keeps the default. |
| `logging` | `on` | `on` or `off`. Enter during initialization accepts the bracketed value. |

Only `itunes_config.exe` writes `itunes.ini`. `--seed` and `--init --seed`
are the same noninteractive walk. A missing INI is written from the compiled
text. An existing INI is left unchanged. The command then runs
`itunes_db.exe --seed` and `itunes_formatter.exe --seed`. `--init` walks the
same stores. A missing INI is prompted and written. An existing INI is
skipped. It then runs `itunes_db.exe --seed` and
`itunes_formatter.exe --init`. Initialization never rewrites a store that
already exists. Enter accepts the bracketed default. `cancel` or EOF during
a prompt returns `1` and does not write that store or launch the remaining
owners. No-argument launch configures the INI only: it prompts with the
stored values and rewrites the file, and it does not launch the database or
the formatter. A leftover `tab_end` key is ignored. These commands do not
update `components.list`.

Missing or malformed: `itunes_ac.exe` calls `report_ini_unavailable` and uses
`shared/defaults.ixx` in memory, including the default directory.

## Song format

The song line is a template file, not an INI key:

```text
<itunes directory>/song.format
```

With the compiled directory that is
`<installation_root>/components/itunes/song.format`. The file contents are
the template itself. The compiled default, used when the file is missing, is:

```text
[{name}] [{artist}] [{album}] [{duration}]
```

`itunes_formatter.exe` is the sole writer. It reads `itunes.ini` to resolve
`directory`. `--seed` and `--init` require a readable INI and do not fall
back to the compiled directory. `--seed` creates a missing `song.format`
from the compiled template and leaves an existing file unchanged. `--init`
prompts for a missing `song.format` and skips an existing one. The
no-argument menu shows the supported tokens and a sample line, and writes
`song.format` only after the user accepts a valid template. Cancel or EOF
in that menu leaves an existing file unchanged. The write goes to
`song.format.new` and then replaces the destination. The formatter creates
the data directory only as part of that save. If `config/itunes.ini` is
missing, the menu runs `itunes_config.exe --init`, waits, and continues
only when that process succeeds and the INI can be read. An existing but
unreadable INI is reported and is not passed to `--init`.

`itunes_ac.exe` compiles the template once at startup. A missing file uses
the compiled default. An unreadable, oversized, or invalid file is logged,
and the compiled default is used. Runtime does not rewrite `song.format` and
does not launch `itunes_formatter.exe`.

Supported tokens are the fields already checked against iTunes COM:

| Token | COM property | Value | Render |
| --- | --- | --- | --- |
| `{name}` | `Name` | `VT_BSTR` | raw string |
| `{artist}` | `Artist` | `VT_BSTR` | raw string |
| `{album}` | `Album` | `VT_BSTR` | raw string |
| `{duration}` | `Duration` | `VT_I4` seconds | local `m:ss` |

`Location` is still read for track removal and is not a format token.
`Duration` is always read because remaining time uses it. `Name`, `Artist`,
and `Album` are always read so listening history can store track metadata.
The song template still inserts a token only when that template references
it. Any other format token is rejected. A further format token is added only
after a live check of the installed iTunes COM interface confirms its
property name, `VARIANT` type, and renderer. `TrackDatabaseID` is read for
listening-history identity and is not a format token.

## Library columns

Copied iTunes Library rows use a separate file:

```text
<itunes directory>/library.format
```

With the compiled directory that is
`<installation_root>/components/itunes/library.format`. The file controls
how many left-to-right columns are retained from tab-delimited iTunes
Library rows copied through the Clipboard, and how those columns are
rendered. The compiled default is:

```text
format = [column]
column_count = 4
```

`column_count` is a positive integer. `1` keeps the first column, `2` keeps
the first two, and `4` keeps the first four. Columns after `column_count`
are ignored. A row with fewer columns keeps the fields that
are present. A tab is a field boundary even when one side is empty, so a
trailing retained tab is an empty column.

`format` has two forms. `column` is the only reserved word.

One `column` token surrounds each retained column with one literal byte on
each side, then joins the results with one space. `[column]` renders
`Song<TAB>Artist` as `[Song] [Artist]`. `{column}` uses braces. An empty
row is one empty field, so the default format renders it as `[]`.

Two `column` tokens join the raw fields with the text between them. That
text must contain exactly one non-space, non-tab character. Spaces and tabs
around that character are kept. `column - column` renders
`Song<TAB>Artist<TAB>Album` as `Song - Artist - Album`. An empty row emits
an empty string. Any other expression is invalid.

`itunes_formatter.exe` is the sole writer of `library.format` and of
`song.format`. `--seed` creates a missing `library.format` from the compiled
default and leaves an existing file unchanged. `--init` prompts for a
missing `library.format` (`format`, then `column_count`) and skips an
existing file. Its menu is:

```text
1. Format song
2. Configure library columns
3. Exit
```

Configure library columns prompts for the format, then the column count,
showing the stored valid values or the compiled defaults. Empty input keeps
the shown value. Invalid input is rejected and asked again. `cancel` or EOF
writes nothing. After both values are accepted, the formatter writes the
complete file through `library.format.new`. The formatter creates the data
directory only as part of that save. `itunes_ac.exe` reads the file at
startup, compiles the format once, and does not rewrite it. A missing file
or a missing assignment uses that field's default without a log. An
existing file that cannot be read is logged and uses both defaults. An
invalid `format` or `column_count` is logged and falls back only that field.

## Star

`itunes_settings.exe` delegates and writes neither store. With `itunes.ini`
present and the component enabled, the shared menu is:

```text
itunes
1. Configure
2. Format song
3. Disable
4. Exit
```

When the INI is missing, item 1 is Initialize (`itunes_config.exe --init`).
Format song launches `itunes_formatter.exe`, which then offers song
formatting and library column configuration. Enable or Disable follows the
current `components.list` state and is performed by `components_editor.exe`.

## Canonical runtime commands

Canonical component commands are case-sensitive and use the lowercase
`itunes_` prefix.

| Command | Behavior |
| --- | --- |
| `itunes_play_pause` | Toggles playback and refreshes the current-track history. |
| `itunes_next_song` | Advances to the next track and wakes the playback monitor. |
| `itunes_print_next_up` | Reads tab-separated queue text from the clipboard, retains `column_count` columns, applies the compiled `library.format` expression, and inserts the result while preserving the clipboard through the component insertion facility. |
| `itunes_print_songs` | Refreshes the current track, inserts the accumulated history, and clears that in-memory history. |
| `itunes_stop_song` | Preserves the existing stop sequence (`Stop`, two `PlayPause` calls), refreshes the current track, and inserts its formatted value. |
| `itunes_remove_song` | Removes the current track from iTunes and then attempts to move its local file to the Windows Recycle Bin. See [Track removal safety](#track-removal-safety). |

Auto Core also retains the main-side compatibility command
`print_next_up_song_list`, which forwards to `itunes_print_next_up`.

Runtime keymap files created before the lowercase migration may still contain
mixed-case names such as `iTunes_play_pause`. Those names are legacy local
configuration and should be changed to their lowercase canonical equivalents.

The pipe protocol also contains process-control commands for shutdown, logger
refresh, previous-track playback, and named-command invocation. Their numeric
wire values are compatibility-sensitive and covered by characterization tests.

## Track history and formatting

With the default template the component formats a track as:

```text
[name] [artist] [album] [m:ss]
```

`245` seconds is `4:05`, `5` is `0:05`, and `0` is `0:00`. History identity
is that formatted string, so a different `song.format` changes which
observations count as the same track. History is held only in the
`itunes_ac.exe` process. Consecutive observations of the same formatted
track are not duplicated. `itunes_print_songs` drains the history, so a
later call reports only tracks observed after the previous drain.
Restarting the component clears the history.

That in-memory history is separate from `history.db`.

## Listening history database

`itunes_db.exe` is the only iTunes executable that opens SQLite. It owns
`<itunes directory>/history.db`. `itunes_ac.exe` does not open that file and
does not link SQLite. `itunes_db.exe` is not a component and is not listed in
`components.list`. It does not poll iTunes.

With no arguments, `itunes_db.exe` is the database manager. If `history.db`
is missing, the menu offers to create a listening database. If the file
exists, `Select *` prints listening-history rows in chronological order.
`itunes_db.exe --serve` creates the data directory and the database when
`itunes_ac.exe` starts it, then accepts one client on `ac_itunes_db_pipe`
using `ac.itunes.db.v1`. That pipe is a private iTunes contract, not
`ac.component.v1`. SQL stays off the pipe. `itunes_db.exe --seed` requires a
readable `itunes.ini`. It creates a missing `history.db` with the current
empty schema and does not open an existing database. `itunes_config.exe --seed` and `--init` delegate that creation.

`itunes_ac.exe` launches `itunes_db.exe --serve` and shuts it down on exit.
The existing playback monitor remains the timing source. Both normal sleeps
stay at about five seconds: one while iTunes is playing, and one while it is
not. Near the end of a track the playing sleep is still `min(remaining, 5)`.
A playback change still wakes the thread early, including `itunes_next_song`.
The monitor does not gain another loop, timer, or watcher.

Each sample that has a readable `TrackDatabaseID` is sent as `observe`:

```text
track_id
title
artist
album
duration_seconds
credit_seconds
observed_at
```

Playback position is used only inside `itunes_ac.exe` to calculate
`credit_seconds`. If the track id cannot be read, the sample is logged and
skipped. Tracks are not identified by title, artist, and album. If no track
is current, nothing is sent and the open history row is left unchanged.

`tracks` stores `track_id`, `title`, `artist`, `album`, `duration_seconds`,
`first_seen_at`, and `last_seen_at`. `listening_history` stores `id`,
`track_id`, `started_at`, `last_observed_at`, `ended_at`, and
`listened_seconds`, with a foreign key to `tracks`. There is no aggregate
table. Total time for a song is `SUM(listened_seconds) GROUP BY track_id`.

A listening-history row accumulates approximate listening time while the
same track remains the current observed track. Pauses, stops, seeks,
restarts, and repeats of that same track remain in the row. Observing a
different track closes the row. Returning to an earlier track later creates
a new row. `itunes_db.exe` remembers only the currently open row. It does
not search older rows and it does not detect pause, seek, or repeat mode.
The same `TrackDatabaseID` after a repeat keeps receiving credit on the
current row, including a shortened end-of-track interval.

Credit follows the wait that just finished:

```text
normal 5-second playing observation
    credit the scheduled sleep, which is 5 seconds or the shorter
    min(remaining, 5) interval

5-second idle observation with position < 5
    credit the current playback position

5-second idle observation with position >= 5
    credit 5 seconds

special Auto Core wake
    synchronize track identity
    credit 0

not playing
    credit 0
```

A zero credit still updates the open row when the track id is present. A
special wake from one track to another closes the previous row and opens
the new one immediately, with `listened_seconds` of 0. The next timed
observation applies the normal credit. An observed track change sets the
previous row's `ended_at` to that sample's `observed_at`.

The database records approximate observed listening duration, not exact
media-player telemetry. Direct use of `iTunes.exe` can be wrong by about
one poll because Auto Core sees the result at the next wake. This prototype
does not reconstruct seeks, pauses, or repeat mode, does not reconcile
iTunes play counts or last-played values, does not synchronize the iTunes
library, and does not continue an open row across process restarts.

A clean `itunes_db.exe --serve` shutdown sets `ended_at` on the open row to
the shutdown time. The next Auto Core run starts a new history row even if
the same song is still selected. A row left open by an abnormal exit is
sealed on the next database open with `ended_at = last_observed_at`.

Queue formatting is clipboard-driven and independent of the iTunes COM queue.
Each input line is split conceptually at tabs, carriage returns are removed,
and retained fields are enclosed in brackets.

## Track removal safety

`itunes_remove_song` is destructive and has two distinct steps:

1. Invoke `Delete` on the current iTunes track object, removing the library
   entry.
2. Ask Windows to move the track's recorded local path to the Recycle Bin with
   undo enabled and without displaying shell UI.

The second step can fail after the iTunes library entry has already been
removed. The component logs whether the file was moved, but it cannot roll the
library deletion back. The real deletion path has not been exercised during
this refactor's manual integration checks. Use it only with recoverable media
and verify the Recycle Bin result.

Automated tests use a fake recycler and do not modify the iTunes library or the
filesystem.

## Windows privilege limitation

> [!IMPORTANT]
> In the current design, elevated Auto Core cannot connect to an iTunes
> instance that is already running without Administrator privileges. Start
> Auto Core first so it activates iTunes in a compatible privilege context, or
> close iTunes and restart it as Administrator before starting Auto Core.

Support for attaching safely to an existing non-administrator iTunes process
is deferred. The investigation must favor least privilege and account for COM,
IPC, process-integrity, file-access, and spoofing risks; it must not weaken a
Windows security boundary merely to make attachment succeed. The acceptance
criteria are tracked in the project [`TODO.md`](../devs/TODO.md).

## Failure behavior and logging

- COM initialization failures are retried within the bounded policy and then
  logged clearly.
- Playback commands return without dereferencing a missing COM application if
  initialization fails.
- A malformed named-command payload stops pipe processing and makes the
  component exit with failure.
- An unknown but well-formed named command is logged and does not stop the
  pipe.
- Monitor exceptions are caught and logged before the monitor exits.
- Pipe and logger errors are reported through the normal component logging
  facilities.

## Source layout

| File | Responsibility |
| --- | --- |
| `itunes_client.ixx`, `itunes_client.cxx` | Public automation facade, component state, and client lifetime. |
| `itunes_com.cxx` | Bounded initialization, COM-owner lifecycle, and current-track COM lookup. |
| `itunes_playback.cxx` | Playback commands and player-state queries. |
| `itunes_track.cxx` | Track metadata retrieval, `TrackDatabaseID`, remaining-duration calculation, formatting, and history recording. |
| `itunes_monitor.ixx`, `itunes_monitor.cxx` | Playback-change notification, periodic monitoring, and listening-credit reports. |
| `itunes_db_client.ixx`, `itunes_db_client.cxx` | `itunes_ac.exe` client for `itunes_db.exe`. |
| `shared/itunes_db_protocol.ixx` | Private `ac.itunes.db.v1` observation pipe. |
| `shared/listening_credit.hpp` | Credit for one existing monitor wake. |
| `db/main.cxx` | `itunes_db.exe` database manager and `--serve`. |
| `db/itunes_sqlite.ixx`, `db/itunes_sqlite.cxx` | `history.db` schema and the open listening row. Compiled only into `itunes_db.exe`. |
| `itunes_runtime.ixx`, `itunes_runtime.cxx` | Serial executor, injectable runtime boundaries, retry policy, and command coordination. |
| `itunes_pipe.ixx`, `itunes_pipe.cxx` | Numeric and named pipe-command registration and dispatch. |
| `itunes_registry.ixx`, `itunes_registry.cxx`, `itunes_registry_default.cxx` | Named-command registry construction and production action binding. |
| `itunes_commands.cxx` | User-facing history and queue command behavior. |
| `itunes_removal.ixx`, `itunes_removal.cxx` | iTunes library deletion and Windows Recycle Bin adapter. |
| `itunes_config.cxx`, `itunes_config_detail.*` | Configuration loading, directory resolution, and pure setting resolution. |
| `shared/song_template_detail.*` | Generic `{token}` parse, compile, and apply. |
| `shared/itunes_metadata_detail.*` | Verified iTunes token catalog and duration rendering. |
| `shared/defaults.ixx` | Compiled `itunes.ini` text and the default song template. |
| `shared/library_format_detail.*` | `library.format` load, format compile, and serialization. |
| `formatter/main.cxx`, `formatter/entry.cxx` | `itunes_formatter.exe` editor for `song.format` and `library.format`. `entry.cxx` holds `main`. |
| `itunes_formatting_detail.*` | Pure queue-item formatting. |
| `itunes_track_detail.*` | Default track formatting and history operations. |
| `itunes_component.ixx` | Component logging identity and logger refresh declaration. |
| `main.cxx` | Component process startup, pipe loop, and orderly shutdown. |

## Verification status

The completed refactor passed:

- 49 Catch2 test cases with 205 assertions;
- Release x64 builds of `itunes_ac.exe` and `auto_core.exe`;
- real Auto Core integration checks for startup, playback commands, history and
  queue output, monitor polling, reconnection, and clean shutdown.

Automated coverage includes configuration resolution, formatting, protocol
values, registry behavior, track history, runtime coordination, initialization
retry, serial-executor behavior, and numeric/named routing through real local
Windows pipes. The non-live suite does not launch iTunes or initialize COM.

The next project phase is an opt-in live test harness. Live tests must use the
`[live]` tag and must identify any playback or filesystem side effects. Real
track deletion and non-administrator attachment remain outside the completed
refactor phase.

Build commands and test tags are documented in
`app/components/itunes/tests/TESTING.md`.
