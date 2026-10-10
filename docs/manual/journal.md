# Journal

Journal inserts episode titles, timestamps, saved text, dice rolls, and numbered choices. A series stores the next episode number. The active series is the one used for the next title.

## Configuration

`config\journal.ini`

`[journal]`

| Setting | Default | Values | Description |
| --- | --- | --- | --- |
| `directory` | `components\journal` | A directory path | Folder that stores Journal data. A relative path is relative to the installation root. An absolute path is used as stored. |
| `auto_select_new_series` | `on` | `off` keeps the current series. Any other text, including `on`, selects the new series. | When a series is created, `on` makes that series active. `off` leaves the current active series. The comparison ignores surrounding spaces and letter case. The menu writes `on` or `off`. |
| `remote_sync` | `off` | `on`, `off` | `on` starts the Journal cloud runtime. The comparison ignores surrounding spaces and letter case. Any other text leaves sync off. The menu writes `on` or `off`. |
| `logging` | `on` | `on`, `off` | Controls logging for the Journal component. |

`remote_sync` is read when Journal starts. Turn it on, then restart Journal, before cloud sync runs.

## Commands

| Command | What it does |
| --- | --- |
| `print_episode_title` | Inserts the next episode title for the active series, then sends Ctrl+S. |
| `save_file_and_create_new_file` | Sends Ctrl+S, then Ctrl+N. If the window title changes, it inserts the next episode title. |
| `print_extended_timestamp` | Inserts the Journal clock time. |

An episode title is the series name, the padded episode number, the compact date, and the extended timestamp.

Alias names in the Journal list files are also commands. Restart Journal after changing an alias file.

| Factory | List file | What an alias does |
| --- | --- | --- |
| `make_print_choice` | `make_print_choice.list` | Asks how many choices, then inserts `Name selects N.` The stored lower bound is the smallest number. |
| `print_and_insert_into_journal` | `print_and_insert_into_journal.list` | Inserts the stored text. |
| `roll_dice` | `roll_dice.list` | Inserts a roll such as `2d6: 3 + 5 = 8`. Dice count is `1` through `100`. Sides are `2` through `1000`. |

A line looks like `alias = factory(arguments)`. `journal_builder.exe` edits these files.

The same factory names can also be used directly in a keymap, with arguments in parentheses.

## Files and data

These files live in the Journal directory.

| File | What it is for |
| --- | --- |
| `series.db` | Series names, the next episode number, and padding. Padding is the minimum width of the episode number. `09` is episode `9` with padding `2`. A wider number is not cut off. |
| `series.map` | The active series is `active = Name`. The snapshot below that line shows each series and its next number. Edit `active` to choose a series. Allocating an episode does not refresh the snapshot. |
| `make_print_choice.list`, `print_and_insert_into_journal.list`, `roll_dice.list` | Alias commands. |
| `extended_hour.clock` | One line, `extended_hours = token`. Tokens are `0`, `+0`, `1`, `+1`, through `12` and `+12`. `0` is a normal `00:00`–`23:59` clock. A plain number continues the previous day until that hour. A `+` number also keeps the exact hour as `24` or higher. `journal_clock.exe` edits this file. |
| `firebase.id` | One Firebase address. A blank file means cloud sync has nowhere to send. `journal_cloud.exe` edits this file. A push sends the series name and the next episode count. |

A new `series.db` starts with the series `Auto Core` and padding `2`. Add or change series with `journal_series.exe`.

## Component tools

| Tool | What it is for |
| --- | --- |
| `journal_ac.exe` | Episode titles, the clock, and alias commands. |
| `journal_config.exe` | Edits `journal.ini`. |
| `journal_series.exe` | Adds series and updates `series.map`. |
| `journal_db.exe` | Creates and updates `series.db`. |
| `journal_builder.exe` | Edits alias lists. |
| `journal_clock.exe` | Edits `extended_hour.clock`. |
| `journal_cloud.exe` | Edits `firebase.id` and, when remote sync is on, sends episode updates. |
| `journal_settings.exe` | Opens configuration, the alias builder, series, Firebase, and extended hours. |
