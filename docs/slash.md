# Slash

`slash_ac.exe` is a one-shot recycle-bin helper. It is launched per command
and advertises `report_and_empty_recycle_bin`. When a report needs a scan,
that scan runs first. The Recycle Bin is emptied next. Success text is
inserted only after the empty succeeds. A failed empty is logged and the
process exits `1`. An empty bin prints `Recycle bin is empty` and exits
`0`. `silent` prints `Recycle bin emptied` on the console and does not
insert text.

`config/slash.ini` is `[slash] mode` (`verbose`, `concise`, or `silent`).
The default is `verbose`, which keeps the categorized report, including
music metadata. `concise` empties the bin and inserts
`Recycle Bin emptied: N items`. `silent` empties the bin, prints
`Recycle bin emptied` on the console, and does not insert text.
Compiled defaults are `app/components/slash/shared/defaults.ixx`.

Only `slash_config.exe` writes the file. `--seed` writes `mode = verbose`
when the file is missing and leaves an existing file unchanged. `--init`
prompts when the file is missing. `--init --seed` seeds without prompting.
A no-argument launch edits `mode`, or uses that same prompt when the file
is missing. `--init` and `--seed` do not update `components.list`.

A missing or unreadable file: `slash_ac.exe` reports, uses `verbose` in
memory, and does not create the file. A readable file with a missing or
invalid `mode` uses `verbose`, reports that, and is not rewritten.

`slash_ac.exe` embeds `AC_LAUNCH_DESCRIPTOR`. That resource names
`report_and_empty_recycle_bin` and tells Main to pass that command on the
command line and wait until the process exits. Enablement is `components.list`.
Main does not compile Slash's command names.

See [Runtime configuration](configuration.md).
