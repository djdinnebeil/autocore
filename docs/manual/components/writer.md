# Writer

Writer inserts prompts, profile URLs, text files, dates, and clipboard text, and opens the daily note and the task list in Notepad.

## Configuration

`config\components\writer.ini`

`[writer]`

| Setting | Default | Values | Description |
| --- | --- | --- | --- |
| `directory` | `writer` | A directory path | Folder that stores Writer data. A relative path is relative to the installation root. An absolute path is used as stored. |
| `notes_subdirectory` | `notes` | A relative or absolute directory path | A relative path is a subdirectory beneath the Writer directory. An absolute path is used directly as the folder for new notes. |
| `logging` | `on` | `on`, `off` | Controls logging for the Writer component. |

The default notes folder is `<installation root>\writer\notes`. Changing the
Writer directory does not affect an absolute `notes_subdirectory`.

## Commands

| Command | What it does |
| --- | --- |
| `select_and_insert_session_prompt` | Lists `session_prompts.list` and inserts the chosen line. |
| `select_and_insert_text` | Lists filenames in `text` and inserts the chosen file unchanged, including its line endings. |
| `select_and_insert_profile_url` | Lists names in `profile_urls.txt` and inserts the chosen URL. |
| `create_new_note_in_notepad` | Creates or opens today's note in Notepad. |
| `launch_task_list` | Opens `task_list.txt` in Notepad. |
| `print_timestamp` | Inserts the local time as `HH:MM`. |
| `print_date_iso` | Inserts the local date as `YYYY-MM-DD`. |
| `print_date_compact` | Inserts the local date as `M-D-YY`. |
| `print_date_iso_with_timestamp` | Inserts `YYYY-MM-DD - HH:MM`. |
| `print_date_iso_with_timestamp_w` | Inserts the same date and time with an en dash. |
| `add_brackets_around_clipboard` | Inserts the clipboard text inside `[` and `]`. |
| `insert_<filename>` | Inserts one file from `text`. `writer_builder.exe` creates these names. |

`Hello World.txt` becomes `insert_hello_world`. The command reads that file when it runs and pastes the bytes unchanged, including line breaks. An empty file inserts nothing. `select_and_insert_text` is unchanged.

## Files and data

| File | What it is for |
| --- | --- |
| `session_prompts.list` | One prompt per line. The starter lines are `I am running Auto Core.` and `I am installing a new component in Auto Core.` |
| `profile_urls.txt` | One `name = url` line per profile. The menu lists the name and inserts the URL. A later `=` stays in the URL. Writer does not create this file. |
| `text\*.txt` | Text inserted by `select_and_insert_text` and by `insert_<filename>`. The menu lists filenames only. The file is read when chosen or when the generated command runs. |
| `insert_text.list` | `insert_<filename> = filename.txt` lines written by `writer_builder.exe`. `writer_ac.exe` loads them at startup and does not create the file. |
| `task_list.txt` | The task list opened by `launch_task_list`. |
| `notes\YYYY-MM-DD.txt` | The daily note. The folder is the notes subdirectory. |

`writer_editor.exe` creates a missing prompt file, task list, and notes folder. It does not create `text` or `profile_urls.txt`. Changing `directory` does not move existing files. Add, remove, or rename `.txt` files in `<writer directory>\text` while Writer is running; the menu reads that directory each time it opens. Edit `<writer directory>\profile_urls.txt` while Writer is running; that menu reads the file each time it opens.

## Component tools

| Tool | What it is for |
| --- | --- |
| `writer_ac.exe` | The commands above. The daily-note command opens the editor. |
| `writer_config.exe` | Edits `writer.ini`. |
| `writer_editor.exe` | Creates the prompt file, task list, and notes, and opens today's note. |
| `writer_settings.exe` | Opens configuration, and can add a task, add a session prompt, or open today's note. |
| `writer_builder.exe` | Rebuilds `insert_text.list` from `text\*.txt`. Restart Writer, then run `keymap_builder.exe`, before the new names are available. |
