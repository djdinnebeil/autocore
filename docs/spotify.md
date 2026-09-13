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
    -> in-memory history and spotify_history.db
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

Run `spotify_oauth.exe` to authorize and register Connect devices. The helper
shares `config/spotify.ini` with `spotify_ac.exe` (no separate OAuth INI). If
that live file is missing, the helper prompts for the application-data
directory, suggesting `.\spotify` as the default to accept, and writes
`[spotify] directory`. Blank input stores `directory = spotify`. If the file
already exists, the helper uses that value and does not rewrite it. Relative
paths resolve against the executable directory; an absolute path is used as-is.

The helper then presents a menu:

1. Generate new tokens. Enter a Spotify client ID and local port. The helper
   starts a loopback HTTP listener at `http://127.0.0.1:<port>/callback`,
   opens the authorization page using Authorization Code with PKCE, exchanges
   the returned code, and writes `spotify_codes.ini` and `spotify_tokens.ini`
   under the configured Spotify data directory (default `spotify/`, which is
   `dist/spotify` when run from `dist`).
   Existing `[devices]` keys are preserved.
2. Retrieve current devices. Reads `client_id` from `spotify_codes.ini` in that
   directory. If that
   value is missing, the helper reports it and returns to the menu. Otherwise
   it refreshes the access token, calls the Spotify devices endpoint, and
   upserts each Connect device into `[devices]` using Spotify's `name` in
   lowercase as the key and `id` as the value. Existing keys are updated,
   including renaming a mixed-case key to lowercase. Names not in the
   response are kept. Duplicate names keep the last ID received.

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

The component refreshes its access token after approximately 55 minutes using
`client_id` and the stored refresh token. Spotify refresh tokens expire six
months after the original authorization; refreshing does not extend that
lifetime. The helper records a local expiration marker 179 days after
authorization. The component warns seven days before that marker and requires
reauthorization when it expires or Spotify returns `invalid_grant`.

Run `spotify_oauth.exe` once on each machine after upgrading: option 1 for tokens,
then option 2 so `spotify_codes.ini` receives the current Connect names and IDs.

Do not copy `spotify_tokens.ini` between machines. Each machine must run
`spotify_oauth.exe` so it receives its own refresh token. A second PKCE login on
another machine does not revoke the first machine's tokens. Sharing one
refresh token file can invalidate the other copy: PKCE refresh often rotates
the refresh token, and the machine that still holds the old value then
receives `invalid_grant`.

## Local files

Paths are relative to the directory containing the executables, normally
`dist`, unless `config/spotify.ini` sets an absolute `[spotify] directory`:

| File | Format and purpose |
| --- | --- |
| `config/spotify.ini` | `[spotify] directory` (portable default `spotify`). Live file is under gitignored `dist/`; tracked sample is [`defaults/config/spotify.ini`](../defaults/config/spotify.ini). Auto Core never reads `defaults/`. A missing live file is written once from the portable default. `spotify_oauth.exe` prompts before writing that file when it is absent. |
| `spotify/spotify_codes.ini` | INI with `[auth] client_id` and `[devices]` keys named by Spotify Connect. Written by `spotify_oauth.exe` (option 1 preserves devices; option 2 registers them) and updated by `spotify_ac.exe` when Connect names or IDs change. |
| `spotify/spotify_tokens.ini` | INI `[tokens]` section: access token, refresh token, `authorized_at`, and `refresh_expires_at`. Written by `spotify_oauth.exe` and refreshed by `spotify_ac.exe`. Machine-local; do not copy to another PC. |
| `spotify/spotify_history.db` | SQLite listening-history database. |
| `keymap/components/spotify.keymap_commands.txt` | Generated canonical runtime-command manifest. |
| `cover.jpg` | Album art written relative to the component's current working directory and overwritten on the next successful download. |

`spotify_codes.ini` and `spotify_tokens.ini` contain secrets. They must not be committed,
logged, or shared. Their current plain-text storage is a known security
limitation.

## Track formatting and history

Tracks and queue entries are formatted as:

```text
[name] [artist 1, artist 2] [album] [minutes:seconds]
```

The monitor normally polls every 15 seconds, shortens the wait near the end of
a track, and wakes early after a next-track command. A newly formatted current
track is appended to in-memory history and upserted into `spotify_history.db` using
`(name, artist, album)` as its identity. The database must already contain a
compatible `track_history` table with a uniqueness constraint that supports
that conflict target; schema creation and migration are not currently owned by
the component.

`spotify_print_songs` drains the in-memory history. Restarting the component also
clears that memory, but does not clear the SQLite database.

## Device and desktop behavior

Configured Connect devices live under `[devices]` in `spotify_codes.ini`. Keys are
Spotify Connect names stored in lowercase:

```ini
[devices]
iphone = 71c59fb04f1dee04b8c63204cf8458f259ae4274
desktop = 7e25f03bb115bd0fffd363b0f4a2b820f6cbc7e9
```

`spotify_oauth.exe` option 2 is the registration path. `spotify_ac.exe` keeps the same
keys current: matching names update their IDs and are rewritten in lowercase,
new names are appended, and names absent from the current API response are
left in place. If two devices share a name, the last ID received wins.

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
- Device keys in `spotify_codes.ini` are Spotify Connect names stored in lowercase;
  the history schema is not created or migrated.
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
| `spotify_track.cxx` | Current-track and queue retrieval, metadata formatting, and album art. |
| `spotify_history.cxx` | SQLite listening-history persistence. |
| `spotify_monitor.ixx` | Playback-monitor ownership, timing, wakeup, and shutdown. |
| `spotify_windows.cxx` | Desktop activation and Spotify-window detection. |
| `spotify_registry.*` | Canonical named-command registry and production action binding. |
| `spotify_pipe.*` | Numeric and named pipe-command registration and dispatch. |
| `spotify_component.ixx` | Component logging identity and logger connection. |
| `main.cxx` | Process startup, pipe loop, and orderly shutdown. |

The main-process adapter is implemented by
`app/main/modules/spotify_component.ixx` and `app/main/src/spotify_component.cxx`.

## Verification status

The contract baseline passes 9 Catch2 test cases with 66 assertions. Coverage
includes stable protocol resources and values, canonical command names,
registry construction and dispatch, and numeric/named routing through real
local Windows pipes. The Release x64 `spotify_ac.exe` build also passes.

The non-live suite does not contact Spotify, start the desktop client, change
playback, write OAuth state, download art, insert text, or modify the history
database. Build commands and test tags are documented in
`app/components/spotify/TESTING.md`.
