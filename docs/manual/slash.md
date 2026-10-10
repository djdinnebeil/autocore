# Slash

Slash empties the Recycle Bin and reports what was removed. It runs once per command.

An empty bin prints `Recycle bin is empty` and does not delete anything. A report is inserted only after the bin is emptied. `silent` prints one console line and does not insert text.

## Configuration

`config\slash.ini`

`[slash]`

| Setting | Default | Values | Description |
| --- | --- | --- | --- |
| `mode` | `verbose` | `verbose`, `concise`, `silent` | `verbose` inserts a categorized report. `concise` inserts `Recycle Bin emptied: N items`. `silent` prints `Recycle bin emptied` and does not insert text. |
| `logging` | `on` | `on`, `off` | Controls logging for the Slash component. |

The verbose report lists the bin contents, then separate sections for files, folders, archive files, and music files when those items are present. Music files include tag information when it can be read.

## Commands

| Command | What it does |
| --- | --- |
| `report_and_empty_recycle_bin` | Empties the Recycle Bin, then reports according to `mode`. |

## Component tools

| Tool | What it is for |
| --- | --- |
| `slash_ac.exe` | Runs `report_and_empty_recycle_bin`. |
| `slash_config.exe` | Edits `slash.ini`. |
| `slash_settings.exe` | Opens Slash configuration. |
