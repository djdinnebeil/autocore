# Configuration

Auto Core configuration files are stored in:

```text
config\
```

Most settings can be changed through `Auto Core Settings.lnk`. The individual
configuration programs are under `bin\`:

```text
bin\<name>_config.exe
```

For example:

```text
bin\taskbar_config.exe
bin\spotify_config.exe
bin\writer_config.exe
```

Auto Core also includes configuration programs for Main settings such as logging policy, shutdown behavior, crash recovery, component policy, and warnings.

## Auto Core configuration

The following files configure Auto Core itself:

| File | Configuration program |
| --- | --- |
| `config\auto_core.ini` | `auto_core_config.exe` |
| `config\components_policy.ini` | `components_policy_config.exe` |
| `config\crash_recovery.ini` | `crash_recovery_config.exe` |
| `config\shutdown.ini` | `shutdown_config.exe` |
| `config\logging_policy.ini` | `logging_policy_config.exe` |
| `config\warnings.ini` | `warnings_config.exe` |

`logging_policy.ini` also defines the `retention_days` and `max_directory_mb`
cleanup-policy values. `auto_core.exe` runs `auto_core_logging_cleanup.exe`
after startup is ready; at this stage that utility reports the current values
and exits without changing any files.

Use Auto Core Settings, or run the corresponding program under `bin\`, to view
or change these settings.

## Component configuration

Each configurable component has its own configuration file and configuration program:

```text
config\components\<component>.ini
<component>_config.exe
```

Component-specific settings are documented in the corresponding manual page:

- [Dash](components/dash.md)
- [iTunes](components/itunes.md)
- [Journal](components/journal.md)
- [Logger](components/logger.md)
- [Server](components/server.md)
- [Slash](components/slash.md)
- [Spotify](components/spotify.md)
- [Taskbar](components/taskbar.md)
- [Wake](components/wake.md)
- [Writer](components/writer.md)

## Component policy

`components_policy.ini` contains settings that control how Auto Core manages components.

The enabled component list itself is stored separately:

```text
components.list
```

`bin\components_editor.exe` manages `components.list`.

`components_policy.ini` and `components.list` serve different purposes.

## Warnings

`warnings.ini` contains warning switches. `missing_winkey_mapping` warns when Auto Core is not in taskbar positions 1 through 10. `unmapped_key` warns when a keymap key has no command. Both default to `on`.

## Keymap

Key bindings are stored in:

```text
keymap.map
```

`bin\keymap_editor.exe` creates a starter `keymap.map` when the file is
missing. Edit an existing map as a text file. Available command names are
listed in `keymap\keymap_commands.txt`; component-specific lists are under
`keymap\components\`. Restart Auto Core after editing the map.

## Paths

Some configuration settings accept a directory.

A relative path is resolved from the Auto Core installation directory. An absolute path is used directly.

For component-specific path settings, see that component's manual page.
