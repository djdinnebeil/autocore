# Writer

`writer_ac.exe` inserts text, opens notes, and maintains `gpt_prompts.txt` /
`task_list.txt`. Settings live in `config/writer.ini`. Defaults are
`app/components/writer/shared/defaults.ixx`.

| Key | Default |
| --- | --- |
| `directory` | `writer` |
| `notes_directory` | `notes` |

Only `writer_config.exe` writes the INI. If it is missing or malformed,
`writer_ac.exe` calls `report_ini_unavailable` and uses those defaults in
memory. Main may resolve writer paths the same way and does not create the
file.

`writer_config.exe` prompts for the data directory on first write (blank
stores `writer`, always stores `notes_directory = notes`), then offers a stub
menu for paths and empty prompt/task files.

See [Runtime configuration](configuration.md).
