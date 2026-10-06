# Spotify component

The Spotify component provides playback control, queue and current-track
formatting, local listening history, playback-device transfer, and album-art
download. It runs as the separate `spotify_ac.exe` process and communicates with
Spotify through the Spotify Web API.

## Runtime architecture

```text
Auto Core keymap
    -> ac_spotify_pipe
    -> numeric or named command adapter
    -> Spotify command registry
    -> Spotify Web API / desktop activation / text insertion

playback monitor
    -> current-track endpoint
    -> in-memory history and history.db through spotify_db.exe
```

Auto Core owns the server side of the local named pipe and launches
`spotify_ac.exe`. The component connects as the client, registers numeric wire
commands and canonical named commands, and processes requests until Auto Core
sends shutdown or the pipe ends.

Main-side pipe writes are serialized so a numeric header and its optional
string payload cannot be interleaved by concurrent keymap invocations. The
component's playback monitor is an owned, joinable thread. Shutdown signals
the monitor, wakes it, and joins it before process exit. A Spotify Web API call
already in progress must return before the monitor can finish.

## Pipe contract

The pipe is named `ac_spotify_pipe`. Numeric values are compatibility-sensitive:

| Value | Command | Behavior |
| ---: | --- | --- |
| 0 | `shutdown` | Stops pipe processing and joins the playback monitor. |
| 1 | `play_pause` | Toggles playback. |
| 2 | `next_song` | Skips forward and wakes the playback monitor. |
| 3 | `print_songs` | Inserts accumulated current-track history. |
| 4 | `get_queue` | Retrieves, formats, and inserts the playback queue. |
| 5 | `update_component` | Refreshes the component log file. |
| 6 | `switch_player` | Transfers playback between configured desktop and mobile devices. |
| 7 | reserved | Intentionally unused; do not reuse without a compatibility decision. |
| 8 | `download_album_cover` | Downloads the largest advertised current-album image. |
| 9 | `invoke_named` | Reads one length-prefixed command name and dispatches it through the registry. |

A malformed or missing `invoke_named` payload marks the protocol failed, stops
pipe processing, and makes `spotify_ac.exe` exit with failure. An unknown but
well-formed name is logged and does not stop the pipe.

## Canonical runtime commands

The Release x64 build writes canonical names to
`dist/keymap/components/spotify.keymap_commands.txt`.

| Command | Behavior |
| --- | --- |
| `spotify_get_queue` | Retrieves the current queue, excludes entries already present in the component's 52-entry queue-history ring, appends the current item, and inserts the formatted text. |
| `spotify_print_songs` | Refreshes the current item, inserts accumulated in-memory history, and clears that history. |
| `spotify_play_pause` | Pauses active playback or attempts to resume it; if no usable active device exists, it activates Spotify and transfers playback to the configured desktop device. |
| `spotify_next_song` | Skips to the next item and wakes the playback monitor. |
| `spotify_switch_player` | Transfers playback between the configured desktop and mobile device IDs. |

Legacy Main-side names such as `print_spotify_songs`, `get_user_sp_queue`,
and the former `sp_*` spellings are no longer registered. Keymap
entries must use the canonical `spotify_` names.

Previous-track playback and album-cover download are not canonical runtime
commands. They remain internal component operations and numeric compatibility
paths respectively.

## Authorization

`spotify_config.exe` owns `config/spotify.ini`. The compiled default is:

```ini
[spotify]
directory = components\spotify
auto_launch_oauth = off
logging = on
```

`directory` is the portable data folder. Relative paths resolve against the
installation root; an absolute path is used as-is. A missing or empty
`directory` uses `<installation_root>/components/spotify`.
`auto_launch_oauth` is stored as `on` or `off`. A missing, empty, or any
other value stays `off`.
`spotify_ac.exe` reads the setting once at startup. A later Configure edit
applies the next time that process starts. The setting does not launch OAuth
when the process starts. `off` means a missing authorization is reported and
`spotify_oauth.exe` is not started. It does not disable refresh of an
authorization that is already stored in `tokens.map`.

Run `spotify_oauth.exe` to authorize. It does not register Connect devices
and it does not refresh tokens for the running component. OAuth does not write
`spotify.ini` or `client.id`. Missing or malformed INI: `spotify_ac.exe` uses
`shared/defaults.ixx` and `report_ini_unavailable`.

The oauth helper presents:

```text
spotify_oauth

1. Authorize Spotify
2. Abort
```

An unknown menu entry asks again. Abort and end-of-input return exit code 2.
Authorize Spotify reads `client.id` and does not prompt for it or launch
`spotify_editor.exe`. A missing or blank ID is an authorization failure
(exit 1) and leaves `tokens.map` unchanged. A usable ID continues to the
local-port prompt. The helper starts a loopback HTTP listener at
`http://127.0.0.1:<port>/callback`, opens the authorization page using
Authorization Code with PKCE, and then displays:

```text
Waiting for Spotify authorization...
Press Ctrl+C or close this window to abort.
```

During that wait it does not read an abort command from stdin. Ctrl+C,
Ctrl+Break, and closing the window exit 2 and do not write `tokens.map`.
A completed exchange writes a complete `tokens.map` (`access_token`,
`refresh_token`, `authorized_at`, `refresh_expires_at`) under the configured
Spotify data directory (default `components/spotify`, which is
`dist/components/spotify` when run from `dist`) and exits 0. Browser,
callback, and token failures exit 1. There is no device mode and no
runtime-refresh mode.

`spotify_config.exe`, `spotify_star.exe`, and `spotify_ac.exe` (only when
`auto_launch_oauth` is `on`) each start `spotify_oauth.exe` with
`CREATE_NEW_CONSOLE` and wait until that process ends. Other Star actions
stay on the Star console.

`spotify_editor.exe` is the only writer of `client.id` and `devices.list`:

```text
spotify_editor

1. Set client ID
2. Retrieve current devices
3. Exit
```

Set client ID, and `spotify_editor.exe --client-id`, both prompt
`Enter Spotify client ID:`. From the menu, empty input does not replace an
existing id. From `--client-id`, empty input leaves a usable id unchanged and
writes a blank file only when no usable id is stored. `spotify_editor.exe --seed`
creates a blank `client.id` when the file is missing and does not change an
existing file.
Retrieve current devices asks the running `spotify_ac.exe` for the current
Connect records and writes `devices.list`. It does not call Spotify and it
does not read `tokens.map`. If `spotify_ac.exe` is not running, the editor
reports that and leaves `devices.list` unchanged.

There is no client secret. The same Spotify developer application and
`client_id` can be used on every machine.

The Spotify developer application must allow that exact loopback redirect URI.
The helper requests the player read/write scopes used by this component along
with several broader library, playlist, profile, follow, and playback scopes.
Changing the requested scope set requires running `spotify_oauth.exe` again so the
user can grant the new contract.

The relevant Spotify references are the
[PKCE flow](https://developer.spotify.com/documentation/web-api/tutorials/code-pkce-flow),
[refreshing tokens](https://developer.spotify.com/documentation/web-api/tutorials/refreshing-tokens),
[scope catalog](https://developer.spotify.com/documentation/web-api/concepts/scopes),
[currently-playing endpoint](https://developer.spotify.com/documentation/web-api/reference/get-the-users-currently-playing-track),
[queue endpoint](https://developer.spotify.com/documentation/web-api/reference/get-queue),
and [transfer-playback endpoint](https://developer.spotify.com/documentation/web-api/reference/transfer-a-users-playback).

`spotify_ac.exe` refreshes its access token after approximately 55 minutes.
It reads `client.id` and the refresh token from `tokens.map`, then replaces
that same file. Spotify refresh tokens expire six
months after the original authorization; refreshing does not extend that
lifetime. The helper records a local expiration marker 179 days after
authorization. The component warns seven days before that marker and requires
reauthorization when it expires or Spotify returns `invalid_grant`.

`auto_launch_oauth` applies only to that existing interactive failure: the
token file is still incomplete, the local refresh expiration is missing or in
the past, or the refresh returns 400 `invalid_grant`. A usable access token,
a successful refresh, the seven-day warning, and other HTTP results, including
429 and a 400 that is not `invalid_grant`, do not start OAuth. When the
setting is `on`, `refresh_tokens` starts `spotify_oauth.exe` once, waits, and
reloads tokens before deciding again. It does not start another OAuth process
for that failure. The attempt clears only after a later refresh succeeds, so
a future expiration or `invalid_grant` can launch again. Restarting
`spotify_ac.exe` also clears it. When the setting is `off`, the component
keeps the diagnostic that `spotify_oauth.exe` must be run and does not start it.

`auto_launch_oauth` applies only to that existing interactive failure: the
token file is still incomplete, the local refresh expiration is missing or in
the past, or the refresh returns 400 `invalid_grant`. A usable access token,
a successful refresh, the seven-day warning, and other HTTP results, including
429 and a 400 that is not `invalid_grant`, do not start OAuth. When the
setting is `on`, `refresh_tokens` starts `spotify_oauth.exe` in a new console
once, waits, and reloads tokens before deciding again. It does not start
another OAuth process for that failure. The attempt clears only after a later
refresh succeeds, so a future expiration or `invalid_grant` can launch again.
Restarting `spotify_ac.exe` also clears it. When the setting is `off`, the
component keeps the diagnostic that `spotify_oauth.exe` must be run and does
not start it. The compiled default is `off`.

`--seed` writes the compiled INI when `spotify.ini` is missing, leaves an
existing INI unchanged, and then seeds a missing `history.db`, `song.format`,
and blank `client.id`. It never prompts, never creates `tokens.map`, and never
launches OAuth. If the client ID is blank or `tokens.map` is absent, it logs
that the files were seeded and
`Spotify initialization incomplete: User Authorization has not been completed.`
It does not log `Spotify initialization complete.`

`--init` prompts for a missing INI. Enter on the OAuth question stores `off`.
An existing INI is not rewritten. A missing `history.db` asks
`Create the listening database? [Y/n]:` and yes runs `spotify_db.exe --seed`.
A missing `song.format` runs `spotify_formatter.exe --init`. A missing or
blank `client.id` explains that the Spotify component requires user
authorization and offers `spotify_editor.exe --client-id`. The user may leave
the ID blank. OAuth starts only when the client ID is usable and `tokens.map`
is absent. It runs in its own console, and the parent waits. If both files
are already present, initialization does not prompt for the client ID and
does not contact Spotify.

`--init --seed` seeds the INI and those three files first, without prompts,
then continues with the client-ID and OAuth steps. A seeded blank `client.id`
is still an incomplete client configuration, so the interactive offer still
runs.

After `--init` or `--init --seed`, the config executable logs one final line:
`Spotify initialization complete.` or
`Spotify initialization incomplete: User Authorization has not been completed.`
OAuth success is the complete line. A blank client ID, OAuth abort (exit 2),
and OAuth failure (exit 1) are the incomplete line. `spotify_config.exe` still
returns 0 in those cases so Auto Core initialization can continue. That return
value means the config process finished. It does not mean Spotify
authorization succeeded. Failure to write a file or to start a helper returns
1. No-argument `spotify_config.exe` edits the INI only and does not seed
stores or run OAuth. Direct `spotify_formatter.exe` on a missing INI still
runs `spotify_config.exe --init`. `--seed` and `--init` on the database,
formatter, and editor require a readable INI and do not launch config
themselves.

Older `spotify_codes.ini`, `spotify_tokens.ini`, and `spotify_history.db` files
are ignored. Do not copy `tokens.map` between machines. Each machine must run
`spotify_oauth.exe` so it receives its own refresh token. A second PKCE login on
another machine does not revoke the first machine's tokens. Sharing one
refresh token file can invalidate the other copy: PKCE refresh often rotates
the refresh token, and the machine that still holds the old value then
receives `invalid_grant`.

## Local files

`config/spotify.ini` lives under the installation `config` directory. The other
Spotify files live in the resolved `[spotify] directory`. The compiled default
is `<installation_root>/components/spotify` (`dist/components/spotify` when
Auto Core runs from `dist`):

| File | Format and purpose |
| --- | --- |
| `config/spotify.ini` | `[spotify] directory` (portable default `components\spotify`), `auto_launch_oauth` (default `off`), and `logging` (default `on`) from `shared/defaults.ixx`. Live file is under gitignored `dist/`. Written only by `spotify_config.exe`. `auto_launch_oauth` is read once by `spotify_ac.exe` and does not launch OAuth at startup. Missing or malformed: `spotify_ac.exe` uses `shared/defaults.ixx` and `report_ini_unavailable`; oauth does not write this file. A 0 exit from `spotify_config.exe` means that process finished. The log line states whether Spotify authorization was completed. |
| `<spotify directory>/song.format` | Song-line template only, with no INI wrapper. Compiled default `[{name}] [{duration}] [{artist}] [{album}]`. Written only by `spotify_formatter.exe`. `spotify_ac.exe` reads it once and does not rewrite it. A missing, unreadable, or invalid file uses that compiled default in memory. `--seed` writes that default when the file is missing and does not overwrite an existing file. |
| `<spotify directory>/client.id` | Whole file is the Spotify client id, with surrounding whitespace ignored. Written only by `spotify_editor.exe`. Read by `spotify_oauth.exe` for the initial authorization and by `spotify_ac.exe` for the refresh POST. A missing file and a blank or whitespace-only file are both incomplete. `--seed` creates a blank file when it is missing. |
| `<spotify directory>/devices.list` | `name = id` lines. Names are matched in lowercase and the last id wins. Written only by `spotify_editor.exe` from records returned by the running `spotify_ac.exe`. Read by `spotify_ac.exe`. |
| `<spotify directory>/tokens.map` | `access_token`, `refresh_token`, `authorized_at`, and `refresh_expires_at`. Written by `spotify_oauth.exe` after a new authorization and by `spotify_ac.exe` when it refreshes. Initialization does not create, overwrite, or revalidate it. Its absence means Spotify user authorization has not completed. Machine-local; do not copy to another PC. |
| `<spotify directory>/history.db` | SQLite listening history. Created and owned by `spotify_db.exe`. `spotify_ac.exe` does not open it. `--seed` creates it when it is missing. Interactive `--init` asks first. |
| `keymap/components/spotify.keymap_commands.txt` | Generated canonical runtime-command manifest. |
| `cover.jpg` | Album art written relative to the component's current working directory and overwritten on the next successful download. |

`client.id` and `tokens.map` contain secrets. They must not be committed,
logged, or shared. Their current plain-text storage is a known security
limitation. `spotify_codes.ini`, `spotify_tokens.ini`, and `spotify_history.db`
are ignored.

## Track formatting and history

`spotify_formatter.exe` is the sole writer of `<spotify directory>/song.format`.
The file contains only the template. `spotify_ac.exe` compiles it once at
startup and applies that one template to the current track and to each queue
title. It does not launch the formatter and does not rewrite the file.

A missing file or a missing Spotify directory uses the compiled default. An
unreadable, oversized, or invalid file is logged, and the compiled default is
used. On read, one terminal LF, CRLF, or CR is stripped; any remaining CR or
LF is invalid. The formatter writes the template with no trailing newline.
The file must be valid UTF-8 and at most 4096 bytes. Empty text, unknown
tokens, and unclosed placeholders are invalid. There is no brace escaping.

The compiled default is:

```text
[{name}] [{duration}] [{artist}] [{album}]
```

With that default, a track is still:

```text
[name] [minutes:seconds] [artist 1, artist 2] [album]
```

Artists are joined with `", "` before the template is applied. A single
artist is the name alone. `{duration}` is `duration_ms / 1000` truncated to
seconds, then unpadded minutes, a colon, and zero-padded seconds (`0:00`,
`0:01`, `90:00`). It is not raw milliseconds.

Tokens read fields already present on the track object from the current
currently-playing and queue responses:

| Token | Source | Render |
| --- | --- | --- |
| `{name}` | `name` | string, or empty when missing |
| `{artist}` | `artists[].name` | joined with `", "` |
| `{album}` | `album.name` | string, or empty when missing |
| `{duration}` | `duration_ms` | local `m:ss` |
| `{track_number}` | `track_number` | decimal, including `0` |
| `{disc_number}` | `disc_number` | decimal, including `0` |
| `{release_date}` | `album.release_date` | string as returned |
| `{explicit}` | `explicit` | `true` or `false`; empty when missing |
| `{id}` | `id` | string, or empty when missing |
| `{uri}` | `uri` | string, or empty when missing |
| `{album_type}` | `album.album_type` | string, or empty when missing |

The formatter previews those tokens with fixed sample metadata and does not
call Spotify. If `config/spotify.ini` is missing, it runs
`spotify_config.exe --init` once, waits, and continues only when that INI is
readable. That `--init` follows the Spotify initialization contract, including
client-ID configuration and OAuth when those steps are still required. An
existing unreadable INI is reported and is not passed to `--init`.
`spotify_formatter.exe --seed` and `--init` require a readable INI and do not
launch `spotify_config.exe`. `--seed` writes the compiled template only when
`song.format` is missing. `--init` runs the format prompt only when the file
is missing. `cancel` writes nothing.

`spotify_star.exe` lists Manage client and devices, Authorize Spotify, then
Format song. Authorize Spotify starts `spotify_oauth.exe` in a new console
and waits. The star menu does not write `song.format`, `client.id`,
`devices.list`, or `tokens.map`. `spotify_db.exe` is not a Star action.

The formatted string is the in-memory current-track and queue-title identity.
`last_song`, print history, and the 52-entry queue ring compare that string.
Two different tracks can therefore collapse when a template omits the fields
that distinguish them. `history.db` does not use the formatted title. A conflict increments `playcount` and updates `last_played`, and leaves `duration` unchanged.
It still upserts on `(name, artist, album)`, with `artist` stored as the
`", "` join and duration stored as integer seconds.

The monitor normally polls every 15 seconds, shortens the wait near the end of
a track, and wakes early after a next-track command. A newly formatted current
track is appended to in-memory history and sent to `spotify_db.exe`, which
upserts `history.db` on `(name, artist, album)`. `spotify_db.exe` with no
arguments is the database manager. If `history.db` is missing, the menu
offers to create it. If the file exists, the menu offers `SELECT *` from
`track_history`. `spotify_db.exe --serve` creates the data directory and the
`track_history` table when they are missing. Playback continues if that
request fails.

`spotify_print_songs` drains the in-memory history. Restarting the component also
clears that memory, but does not clear the SQLite database.

## Device and desktop behavior

Configured Connect devices live in `devices.list`. Names are stored in
lowercase, and a name may contain `=`:

```text
iphone = 71c59fb04f1dee04b8c63204cf8458f259ae4274
desktop = 7e25f03bb115bd0fffd363b0f4a2b820f6cbc7e9
```

`spotify_editor.exe` writes that file from the records returned by the running
`spotify_ac.exe`. The runtime queries Spotify, lowercases names, and keeps the
last id when names collide. Names absent from that response are not kept.
The runtime does not write `devices.list`. Queue, current track, next,
previous, and album art do not require the device file. A device command with
an empty cache rereads `devices.list` once and, if it is still empty, reports
`No Spotify devices are configured.` once per process.

`get_device_code("desktop")` returns the stored Spotify device ID for that
exact key, or an empty string when the key is missing or still empty.
`spotify_switch_player` cycles among configured devices that already have IDs.
Local playback fallback prefers `desktop`, then `laptop`, then the first
configured device that has an ID.

Playback-control and transfer endpoints may require Spotify Premium and an
available Spotify Connect device.

When playback cannot start through the Web API, the component asks Auto Core's
native taskbar snapshot to activate the application registered as `spotify`,
waits for the desktop window, and transfers playback to that local device.

## Failure behavior and known limitations

- Authentication, HTTP status, JSON parsing, pipe, and database failures are
  reported through component logging where handled.
- Queue retrieval converts all exceptions into an empty result and a generic
  log message.
- Most Web API calls currently have no component-level timeout or retry policy.
- Spotify state is shared between the pipe thread and monitor; token refresh is
  mutex-protected, but the broader client is not yet a serial executor.
- Device names in `devices.list` are Spotify Connect names stored in lowercase.
  The database manager creates `history.db` only when that option is chosen,
  or when `spotify_db.exe --seed` runs and the file is missing.
  `spotify_db.exe --serve` creates the file when the service starts.
- Album art uses an implicit working-directory path.
- No live Spotify or OAuth flow was exercised as part of the contract test
  suite.

## Source layout

| File | Responsibility |
| --- | --- |
| `spotify_client.ixx`, `spotify_client.cxx` | Spotify client state and construction. |
| `spotify_commands.cxx` | Canonical component command actions and text insertion. |
| `spotify_auth.cxx` | Credential loading, token refresh, and reauthorization policy. |
| `spotify_playback.cxx` | Playback and Spotify Connect device operations. |
| `spotify_track.cxx` | Current-track and queue retrieval, template application, and album art. |
| `shared/song_template.ixx` | Generic `{token}` parse, validation, compile, and apply. |
| `shared/song_catalog.ixx` | Spotify token names, artist joining, duration rendering, and track JSON values. |
| `formatter/main.cxx` | `spotify_formatter.exe`, the only writer of `song.format`. |
| `spotify_history.cxx` | History request to `spotify_db.exe`. |
| `spotify_devices.cxx` | Private device-list request for `spotify_editor.exe`. |
| `spotify_db_client.ixx` | `spotify_ac.exe` client for `history.db`. |
| `shared/application_data.ixx` | `client.id` and `devices.list` parse and serialize. |
| `shared/token_store.ixx` | `tokens.map` parse, serialize, and coordinated replace. |
| `editor/main.cxx` | `spotify_editor.exe`, the only writer of `client.id` and `devices.list`. |
| `db/main.cxx` | `spotify_db.exe` no-argument database manager, `--serve`, and `--seed`, the only owner of `history.db`. |
| `spotify_monitor.ixx` | Playback-monitor ownership, timing, wakeup, and shutdown. |
| `spotify_windows.cxx` | Desktop activation and Spotify-window detection. |
| `spotify_registry.*` | Canonical named-command registry and production action binding. |
| `spotify_pipe.*` | Numeric and named pipe-command registration and dispatch. |
| `spotify_component.ixx` | Component logging identity and logger connection. |
| `main.cxx` | Process startup, pipe loop, and orderly shutdown. |

Main hosts Spotify as a generic `ac.component.v1` child. Invoke and shutdown
share one per-child mutex on the control pipe.

## Verification status

The contract baseline passes 44 Catch2 test cases with 209 assertions. Coverage
includes stable protocol resources and values, canonical command names,
registry construction and dispatch, numeric/named routing through real
local Windows pipes, song-format compile, render, load, and replace
behavior, `client.id`, `devices.list`, and `tokens.map` parsing, and the
`auto_launch_oauth` parse and launch decision. The Release x64
`spotify_ac.exe` and `spotify_config.exe` builds also pass.

The non-live suite does not contact Spotify, start the desktop client, change
playback, write `tokens.map`, open `history.db`, download art, or insert text.
Build commands and test tags are documented in
`app/components/spotify/tests/TESTING.md`.
