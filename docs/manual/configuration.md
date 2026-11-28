# Configuration

Auto Core configuration files are stored in:

```text
config\
```

Most settings can be changed through `app\auto_core_settings.exe`. Every component, including a third-party component, uses one executable in `components\`:

```text
components\<name>_ac.exe --config
```

The settings menu is `components\<name>_ac.exe` with no arguments, or `components\<name>_ac.exe --settings`. Place a new component executable in `components\`. Configuration files stay under `config\`.

Main settings use `app\auto_core_settings.exe --config`. With no further arguments it opens a menu; `--area <route>` opens one area directly.

## Auto Core configuration

The following files configure Auto Core itself:

| File | Configuration program |
| --- | --- |
| `config\auto_core.ini` | `auto_core_settings.exe --config --area auto-core` |
| `config\components_policy.ini` | `auto_core_settings.exe --config --area components-policy` |
| `config\crash_recovery.ini` | `auto_core_settings.exe --config --area crash-recovery` |
| `config\shutdown.ini` | `auto_core_settings.exe --config --area shutdown` |
| `config\logging_policy.ini` | `auto_core_settings.exe --config --area logging-policy` |
| `config\startup.ini` | `auto_core_settings.exe --config --area startup` |
| `config\warnings.ini` | `auto_core_settings.exe --config --area warnings` |
| `config\taskbar_policy.ini` | `auto_core_taskbar.exe --config` |

The sectionless `config\startup.ini` accepts these independent switches:

```ini
show_day_of_week = on
show_task_list = on
```

Both values default to `on` if the file or setting is unavailable or invalid.
The first controls the `Today is <day>` startup line. The second controls
whether Auto Core reads and displays Writer's `task_list.txt`; when disabled,
neither task content nor `Nothing pending today.` is displayed.

`config\auto_core.ini` also stores the installed version and the update check:

```ini
version = 1.0.1
check_for_updates = on
```

`version` records the compiled release and defaults to `1.0.1`. Incremental
releases use `MAJOR.MINOR.PATCH`. The finalized release of that line uses
`MAJOR.MINOR` and replaces every `MAJOR.MINOR.PATCH` release with the same
major and minor. `check_for_updates` accepts `on` or `off` and defaults to
`on`. When it is `on`, Auto Core checks GitHub once at startup and prints a
line if a newer version is published. `version.txt` may use either form.
`off` skips that request. A missing or invalid value uses the default.

`logging_policy.ini` defines `retention_days` and `max_directory_mb`. The
defaults are 10 days and 25 MB. `retention_days` is an elapsed UTC period:
a `.log` file is eligible when its last write time is strictly earlier than
now minus that many 24-hour periods. `0` uses now as the cutoff.
`max_directory_mb` then deletes the oldest remaining eligible files until
the `.log` files fit. One megabyte is 1,048,576 bytes. A file named with
today's local date, `YYYY-MM-DD_`, is kept by both steps and still counts
toward the size. `0` for either setting keeps those named logs. If they
alone exceed the cap, cleanup finishes and leaves the directory over the
limit.

`auto_core.exe` runs log cleanup on a background thread after startup is ready,
then again at each local midnight. It uses the logging configuration resolved
when the host started, so edits to `logging_policy.ini` apply after the host
restarts. Deletion counts are `log_debug` records on the `auto_core` log.
A directory or delete failure is `log_alert`.

The same file's `[log_merge]` section sets `merge_interval_seconds` (default
`5`; `0` turns the timer off) and `merge_logs_on_shutdown` (default `on`).
Auto Core Settings edits both sections. `components.list` entry `log_merger`
is the master switch for both automatic paths. Off skips them and leaves these
values saved. On lets each key control its own path. A missing `log_merger`
line leaves automatic merging on. `auto_core.exe` performs the periodic merge
in-process and does not start that worker when `log_merger` is off.
`auto_core_logger.exe --refresh` updates today's Main log, and `--merge`
merges every date, in either list state.

Use Auto Core Settings, or run the corresponding program under `app\`, to view
or change these settings.

`taskbar_policy.ini` stores Taskbar `mode` and `logging`. Taskbar data is
`<installation>\data\taskbar`. `activate_commands.map` is
`<installation>\keymap\activate_commands.map`. Ordinary shared commands are
`<installation>\keymap\shared_commands.map`. See the [Taskbar manual](components/taskbar.md).

## Component configuration

Each configurable component has its own configuration file:

```text
config\components\<component>.ini
```

Every component writes that file with `components\<component>_ac.exe --config`.

Component-specific settings are documented in the corresponding manual page:

- [Dash](components/dash.md)
- [iTunes](components/itunes.md)
- [Journal](components/journal.md)
- [Logger](components/logger.md)
- [Server](components/server.md)
- [Slash](components/slash.md)
- [Spotify](components/spotify.md)
- [Wake](components/wake.md)
- [Writer](components/writer.md)

## Component policy

`components_policy.ini` contains settings that control how Auto Core manages components.

The enabled component list itself is stored separately:

```text
components.list
```

`app\auto_core_components.exe` manages `components.list`.

`components_policy.ini` and `components.list` serve different purposes.

## Warnings

`warnings.ini` contains warning switches. `missing_winkey_mapping` warns when Auto Core is not in taskbar positions 1 through 10 and defaults to `on`. `unmapped_key` warns when a keymap key has no command and defaults to `off`.

## Keymap

Key bindings are stored in:

```text
keymap.map
```

`app\auto_core_keymap.exe --seed` creates a starter `keymap.map` when the file is
missing and does not overwrite an existing map. That starter ends with a blank
line and an empty `[commands]` section. Edit an existing map as a text file.
Running `auto_core_keymap.exe` with no arguments shows `Keymap commands`,
creates a missing `keymap.map` with `--seed`, then builds the command catalogs
and the final `[commands]` section. Success shows `Keymap commands successfully
built.` A failure shows `Keymap commands could not be built.` The menu then
offers `1. Rebuild keymap`, which repeats that seed-if-missing and build
sequence, and `2. Exit`. An empty choice exits. An existing map's bindings are
not replaced. `auto_core_keymap.exe --build` rewrites the command catalogs
and updates the final `[commands]` section of `keymap.map` from those command
names. Text before that section is kept. One blank line separates that text
from `[commands]` when the text does not already end with one. `[commands]`
contains command names only. A merged catalog line is `name [Provider] - description` when the description is non-empty, or `name [Provider]` when it is empty. Component lists under `keymap\components\` stay `name - description`. The description and the provider label are
not copied into `[commands]`.
Available command names are listed in `keymap\keymap_commands.txt`;
component-specific lists are under `keymap\components\`. Auto Core still
rewrites `keymap\keymap_commands.txt` from the live registry at startup.
Restart Auto Core after editing the map.

## Paths

Some configuration settings accept a directory.

A relative path is resolved from the Auto Core installation directory. An absolute path is used directly.

For component-specific path settings, see that component's manual page.
