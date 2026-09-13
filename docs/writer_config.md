# Writer configuration

`writer_config.exe` shares `config/writer.ini` with `writer_ac.exe` and Main
(no separate config INI). Main (and any other seeder) writes a missing live
file from portable defaults (`directory = writer`, `notes_directory = notes`)
and does not prompt. If the live file is still missing when the helper starts,
it prompts for the Writer data directory, suggesting `.\writer` as the default
to accept. Blank input stores `[writer] directory = writer`. That first helper
write also stores `[writer] notes_directory = notes`. An existing live file is
never rewritten. A missing file at runtime keeps in-memory `<exe>/writer` and
`<exe>/notes` until a seeder writes the defaults.

After the INI exists, the helper shows a stub menu: print resolved paths,
create empty `gpt_prompts.txt` and `task_list.txt` if they are missing, open
the writer or notes folder in Explorer, or exit. Dated notes stay
create-on-demand in `writer_ac.exe`. Later slices can add GPT prompt and
task-list editors here.

See [Runtime configuration](configuration.md).
