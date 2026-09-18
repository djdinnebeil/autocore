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
auto_start = true
tab_end = 4
```

| Setting | Default | Behavior |
| --- | ---: | --- |
| `auto_start` | `false` | Only lowercase `true` or `false` is applied. Any other present value keeps the default. |
| `tab_end` | `3` | Stops queue-item formatting when this numbered tab is reached. A malformed integer preserves the current/default value. |

The distributed configuration currently enables automatic startup and uses a
`tab_end` value of `4`.

## Canonical runtime commands

Canonical component commands are case-sensitive and use the lowercase
`itunes_` prefix. The Release build writes them to
`dist/keymap/components/itunes.keymap_commands.txt`.

| Command | Behavior |
| --- | --- |
| `itunes_play_pause` | Toggles playback and refreshes the current-track history. |
| `itunes_next_song` | Advances to the next track and wakes the playback monitor. |
| `itunes_print_next_up` | Reads tab-separated queue text from the clipboard, formats each row with brackets through `tab_end`, and inserts the result while preserving the clipboard through the component insertion facility. |
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

The component formats a track as:

```text
[name] [artist] [album] [minutes:seconds]
```

History is held only in the `itunes_ac.exe` process. Consecutive observations
of the same formatted track are not duplicated. `itunes_print_songs` drains
the history, so a later call reports only tracks observed after the previous
drain. Restarting the component clears the history.

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
criteria are tracked in the project [`TODO.md`](TODO.md).

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
| `itunes_track.cxx` | Track metadata retrieval, remaining-duration calculation, formatting, and history recording. |
| `itunes_monitor.ixx`, `itunes_monitor.cxx` | Playback-change notification and periodic monitoring. |
| `itunes_runtime.ixx`, `itunes_runtime.cxx` | Serial executor, injectable runtime boundaries, retry policy, and command coordination. |
| `itunes_pipe.ixx`, `itunes_pipe.cxx` | Numeric and named pipe-command registration and dispatch. |
| `itunes_registry.ixx`, `itunes_registry.cxx`, `itunes_registry_default.cxx` | Named-command registry construction and production action binding. |
| `itunes_commands.cxx` | User-facing history and queue command behavior. |
| `itunes_removal.ixx`, `itunes_removal.cxx` | iTunes library deletion and Windows Recycle Bin adapter. |
| `itunes_config.cxx`, `itunes_config_detail.*` | Configuration loading and pure setting resolution. |
| `itunes_formatting_detail.*` | Pure queue-item formatting. |
| `itunes_track_detail.*` | Pure track formatting and history operations. |
| `itunes_component.ixx` | Component logging identity and logger refresh declaration. |
| `main.cxx` | Component process startup, pipe loop, and orderly shutdown. |

## Verification status

The completed refactor passed:

- 37 Catch2 test cases with 138 assertions;
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
`app/components/itunes/TESTING.md`.
