# Spotify

Spotify controls playback through the Spotify Web API, inserts the current track and queue, switches playback devices, and keeps a local listening history.

Playback commands need a client ID and a completed authorization. Sign-in uses Authorization Code with PKCE on a loopback callback. There is no client secret.

`auto_launch_oauth` does not start authorization at startup. It starts authorization only when the saved authorization is incomplete, the local refresh expiration is missing or past, or Spotify rejects the refresh. The seven-day warning does not start sign-in.

A refresh token expires about six months after authorization. Refreshing it does not extend that lifetime. The component warns seven days before that local expiration and requires sign-in again when the marker passes or Spotify rejects the refresh. `tokens.map` belongs to this computer. Copying it can invalidate one of the copies, because a refresh often replaces the saved token.

## Configuration

`config\spotify.ini`

`[spotify]`

| Setting | Default | Values | Description |
| --- | --- | --- | --- |
| `directory` | `components\spotify` | A directory path | Folder that stores Spotify data. A relative path is relative to the installation root. An absolute path is used as stored. |
| `auto_launch_oauth` | `off` | `on`, `off` | Launch Spotify authorization when interactive reauthorization is required. Only `on` turns this on. |
| `logging` | `on` | `on`, `off` | Controls logging for the Spotify component. |

## Commands

| Command | What it does |
| --- | --- |
| `spotify_play_pause` | Toggles play and pause. |
| `spotify_next_song` | Skips to the next track. |
| `spotify_print_songs` | Inserts the tracks heard since the last time this command ran, including the current track. |
| `spotify_get_queue` | Inserts the current queue. |
| `spotify_switch_player` | Moves playback to the next device listed in `devices.list`. |

## Files and data

These files live in the Spotify directory.

| File | What it is for |
| --- | --- |
| `client.id` | The Spotify application client ID, as the whole file. A blank file is not authorized. |
| `tokens.map` | Saved authorization. It is created by `spotify_oauth.exe`, not by seeding. Do not copy it to another PC. |
| `song.format` | One template for the current track. Tokens are `{name}`, `{artist}`, `{album}`, `{duration}`, `{track_number}`, `{disc_number}`, `{release_date}`, `{explicit}`, `{id}`, `{uri}`, and `{album_type}`. The default is `[{name}] [{duration}] [{artist}] [{album}]`. |
| `devices.list` | Playback devices, one `name = id` line each. The split is the last `=`. Names are compared without regard to letter case, and the last line for a name wins. |
| `history.db` | Listening history: name, artist, album, duration, play count, and when the track was first and last heard. |

## Component tools

| Tool | What it is for |
| --- | --- |
| `spotify_ac.exe` | Playback, queue text, device switching, and history recording. |
| `spotify_config.exe` | Edits `spotify.ini`. Initialization can also ask for the client ID and start authorization. |
| `spotify_editor.exe` | Edits `client.id` and `devices.list`. |
| `spotify_oauth.exe` | Signs in and writes `tokens.map`. |
| `spotify_formatter.exe` | Edits `song.format`. |
| `spotify_db.exe` | Creates a missing `history.db`. |
| `spotify_settings.exe` | Opens the client and device editor, authorization, and song formatting. |
