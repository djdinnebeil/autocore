# iTunes

iTunes controls the desktop iTunes application: playback, current-track text, a formatted clipboard queue, and listening history.

`auto_start` starts iTunes when this component starts.

## Configuration

`config\itunes.ini`

`[itunes]`

| Setting | Default | Values | Description |
| --- | --- | --- | --- |
| `directory` | `components/itunes` | A directory path | iTunes data directory. A relative path is relative to the installation root. An absolute path is used as stored. |
| `auto_start` | `on` | `on`, `off` | Start iTunes when the iTunes component starts. |
| `logging` | `on` | `on`, `off` | Controls logging for the iTunes component. |

## Commands

| Command | What it does |
| --- | --- |
| `itunes_play_pause` | Toggles play and pause. |
| `itunes_next_song` | Skips to the next track. |
| `itunes_stop_song` | Stops playback and inserts the current track text. |
| `itunes_print_songs` | Inserts the tracks heard since the last time this command ran. |
| `itunes_print_next_up` | Formats tab-separated iTunes library rows on the clipboard and inserts the result. `print_next_up_song_list` is the same command. |
| `itunes_remove_song` | Removes the current track from iTunes and moves its file to the Recycle Bin. |

## Files and data

These files live in the iTunes directory.

| File | What it is for |
| --- | --- |
| `song.format` | One template for the current track. Tokens are `{name}`, `{artist}`, `{album}`, and `{duration}`. The default is `[{name}] [{artist}] [{album}] [{duration}]`. |
| `library.format` | How clipboard library rows are formatted. `format` is either one surrounding pair, such as `[column]`, or one separator between columns, such as `column - column`. `column_count` is how many left-to-right columns to keep. The defaults are `[column]` and `4`. `column_count` must be an integer of `1` or greater. |
| `history.db` | Listening history: track title, artist, album, duration, and when the track was heard. |

`itunes_formatter.exe` edits the two format files. `itunes_db.exe` creates a missing `history.db`.

`itunes_remove_song` removes the iTunes library entry first, then asks Windows to move the file to the Recycle Bin. If that move fails, the library deletion is not undone. Check the Recycle Bin after removal.

Listening history is approximate. Playback is sampled about every five seconds. A playback change, including skip, is observed sooner.

Auto Core cannot attach to an iTunes process that is already running at a lower privilege level. Start Auto Core before iTunes, or close iTunes and start it again at the same privilege level as Auto Core.

## Component tools

| Tool | What it is for |
| --- | --- |
| `itunes_ac.exe` | Playback, formatting, and history recording. |
| `itunes_config.exe` | Edits `itunes.ini`. |
| `itunes_formatter.exe` | Edits `song.format` and `library.format`. |
| `itunes_db.exe` | Creates a missing `history.db`. |
| `itunes_settings.exe` | Opens configuration and song formatting. |
