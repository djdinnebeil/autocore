# Writer

`writer_ac.exe` inserts text and reads Writer data. `writer_editor.exe` is the only Auto Core writer of that data. Settings live in `config/writer.ini`. Defaults are `app/components/writer/shared/defaults.ixx`.

| Key | Default |
| --- | --- |
| `directory` | `writer` |
| `notes_subdirectory` | `notes` |
| `logging` | `on` |

`directory` is resolved against the installation root. `notes_subdirectory` must be a relative path and is resolved against the Writer directory, not the installation root. The defaults resolve to `<installation_root>\writer` and `<installation_root>\writer\notes`. A missing, empty, or non-relative `notes_subdirectory` uses `<writer directory>\notes`. `writer_ac.exe` logs an invalid stored value and does not rewrite the file. An old `notes_directory` key is ignored.

Only `writer_config.exe` writes the INI. If it is missing or malformed, `writer_ac.exe` calls `report_ini_unavailable` and uses the compiled defaults in memory. It does not create the file and it does not launch `writer_config.exe` or `writer_editor.exe`.

`writer_editor.exe` owns:

- `<writer directory>\session_prompts.list`
- `<writer directory>\task_list.txt`
- the resolved notes directory and files under it

A fresh seed creates this tree:

```text
config/writer.ini
writer/session_prompts.list
writer/task_list.txt
writer/notes/
```

`writer.ini` is:

```ini
[writer]
directory = writer
notes_subdirectory = notes
logging = on
```

`session_prompts.list` is:

```text
I am running Auto Core.
I am installing a new component in Auto Core.
```

`task_list.txt` is:

```text
Add a new task to task_list.txt to show up here.
```

`writer/notes/` is created empty. No sample note is written.

## Initialization

`--seed` is checked first, so `--init --seed` is `--seed` and never prompts.

Both walks cover the whole Writer surface and only fill stores that are absent. An existing file, including an empty file, is left byte for byte. An existing notes directory and its contents are left alone. Nothing is rolled back if a later step fails. A valid `writer.ini` stays if application-data creation fails. Rerunning `--init` or `--seed` resumes by skipping what is already there.

Changing `directory` or `notes_subdirectory` later does not move existing Writer data. The next initialization uses the stored paths. The compiled `writer` and `notes` values are used only when that configuration is missing and seed is writing the default INI.

- `--seed` writes the compiled INI when `config/writer.ini` is missing, leaves an existing file unchanged, then runs `writer_editor.exe --seed`.
- `--init` prompts only when the INI is missing:

```text
Writer directory [writer]:
Notes subdirectory [notes]:
Enable logging [on]:
```

Enter keeps the value in brackets. `notes_subdirectory` must be a relative path. `cancel`, or closing input, on one of these prompts returns nonzero and does not launch the editor. After the INI exists or has just been written, `--init` runs `writer_editor.exe --init`.

`writer_editor.exe --seed` creates each missing store from the recommended text above, including an empty notes directory. `writer_editor.exe --init` prompts only for missing stores:

```text
Session prompt [I am running Auto Core.]:
Session prompt [I am installing a new component in Auto Core.]:
Task [Add a new task to task_list.txt to show up here.]:
Create notes directory "notes"? [Y/n]:
```

The notes prompt uses the configured `notes_subdirectory`. Enter means yes. `n` skips that directory, returns success, and continues. Declining it is not cancellation. `cancel` or closed input on a session-prompt or task prompt returns nonzero. Files already written stay.

A missing `writer_editor.exe` is reported by name and `writer_config.exe` returns nonzero.

No-argument `writer_config.exe` is the settings menu for `directory` and `notes_subdirectory`. It does not launch the editor. If the INI is missing, that menu still asks for the three settings and writes only `writer.ini`. No-argument `writer_editor.exe` stays the edit menu. `--add-task`, `--add-session-prompt`, `--daily-note`, and that menu require `writer.ini`. If it is missing, they report that Writer must be initialized first, create nothing, and exit nonzero.

The edit menu is:

```text
writer_editor

1. Add new session prompt
2. Add task
3. Create new note
4. Exit
```

## Star

`writer_star.exe` does not open that menu. It delegates:

```text
writer

1. Initialize
   or Configure
   then Complete initialization, when the INI exists and
   session_prompts.list or task_list.txt is missing
2. Add a new task
3. Add a new session prompt
4. Create or open daily note
5. Enable
   or Disable
6. Exit
```

**Initialize** and **Complete initialization** launch `writer_config.exe --init`. **Configure** launches `writer_config.exe` with no arguments. The notes directory is not part of that incomplete check, because declining it is a successful initialization choice. Star writes nothing. Choosing an edit action before the INI exists is rejected by the editor.

## Runtime

`writer_ac.exe` keeps the keymap command `create_new_note_in_notepad`. That command starts `writer_editor.exe --daily-note` and does not create the note itself. `--daily-note` creates the day's note when that command runs, which creates the notes directory if the command needs it. Session prompt selection reads `session_prompts.list` and logs a missing or empty file. `launch_task_list` opens `task_list.txt` in Notepad and does not create it. A missing task list is logged by the ready banner and omitted; an empty file prints "Nothing pending today."

See [Runtime configuration](configuration.md).
