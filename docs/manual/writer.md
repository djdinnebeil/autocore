# Writer

Writer inserts prompts, dates, and clipboard text, and opens the daily note and the task list in Notepad.

## Configuration

`config\writer.ini`

`[writer]`

| Setting | Default | Values | Description |
| --- | --- | --- | --- |
| `directory` | `writer` | A directory path | Folder that stores Writer data. A relative path is relative to the installation root. An absolute path is used as stored. |
| `notes_subdirectory` | `notes` | A relative path | Notes folder inside the Writer directory. An absolute path is not accepted. |
| `logging` | `on` | `on`, `off` | Controls logging for the Writer component. |

The default notes folder is `<installation root>\writer\notes`.

## Commands

| Command | What it does |
| --- | --- |
| `select_and_insert_session_prompt` | Lists `session_prompts.list` and inserts the chosen line. |
| `create_new_note_in_notepad` | Creates or opens today's note in Notepad. |
| `launch_task_list` | Opens `task_list.txt` in Notepad. |
| `print_timestamp` | Inserts the local time as `HH:MM`. |
| `print_date_iso` | Inserts the local date as `YYYY-MM-DD`. |
| `print_date_compact` | Inserts the local date as `M-D-YY`. |
| `print_date_iso_with_timestamp` | Inserts `YYYY-MM-DD - HH:MM`. |
| `print_date_iso_with_timestamp_w` | Inserts the same date and time with an en dash. |
| `add_brackets_around_clipboard` | Inserts the clipboard text inside `[` and `]`. |
| `print_and_insert_special_utf8` | Inserts the test line `Testing std1::string: caf——Auto Core`. |
| `print_and_insert_special_utf16` | Inserts the test line `Testing std2::wstring: caf——Auto Core`. |
| `print_and_insert_testing` | Inserts both test lines. |

## Files and data

| File | What it is for |
| --- | --- |
| `session_prompts.list` | One prompt per line. The starter lines are `I am running Auto Core.` and `I am installing a new component in Auto Core.` |
| `task_list.txt` | The task list opened by `launch_task_list`. |
| `notes\YYYY-MM-DD.txt` | The daily note. The folder is the notes subdirectory. |

`writer_editor.exe` creates a missing prompt file, task list, and notes folder. Changing `directory` does not move existing files.

## Component tools

| Tool | What it is for |
| --- | --- |
| `writer_ac.exe` | The commands above. The daily-note command opens the editor. |
| `writer_config.exe` | Edits `writer.ini`. |
| `writer_editor.exe` | Creates the prompt file, task list, and notes, and opens today's note. |
| `writer_settings.exe` | Opens configuration, and can add a task, add a session prompt, or open today's note. |
