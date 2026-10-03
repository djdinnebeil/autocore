# Writer tests

Release x64. Build the DLL first, or use the tracked `lib/auto_core.lib`. `build-all.ps1` does not build this project. The executable is `obj\writer_tests\writer_tests.exe`.

```powershell
msbuild app\components\writer\tests\writer_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64
.\obj\writer_tests\writer_tests.exe "[writer][unit]"
```

`writer_data_tests.cxx` covers the seeded `writer.ini` text, the starter bytes for `session_prompts.list` and `task_list.txt`, resolution of `[writer] directory` and `notes_subdirectory`, and the required baseline. The notes directory is not part of that baseline.

`writer_config.exe` writes `config/writer.ini` only. `--init` and `--seed` then delegate missing `session_prompts.list`, `task_list.txt`, and the notes directory to `writer_editor.exe`. An existing file is not replaced. A custom `directory` is followed; the compiled `writer` and `notes` values are used only when that configuration is missing. `writer_ac.exe` does not create these stores.
