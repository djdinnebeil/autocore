# Writer tests

Release x64. Build the DLL first, or use the tracked `lib/auto_core.lib`. `build-all.ps1` does not build this project. The executable is `obj\writer_tests\writer_tests.exe`.

```powershell
msbuild app\components\writer\tests\writer_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64
.\obj\writer_tests\writer_tests.exe "[writer][unit]"
```

`writer_data_tests.cxx` covers the seeded `writer.ini` text, the starter bytes for `session_prompts.list` and `task_list.txt`, resolution of `[writer] directory` and `notes_subdirectory`, and the required baseline. The notes directory is not part of that baseline. That suite is not the acceptance source for the `notes_subdirectory` contract below.

Field-test `notes_subdirectory` against the installed `config/components/writer.ini`. A relative value joins the resolved Writer directory. An absolute value, including a drive path and a UNC path, is used directly and is not appended to that directory. A missing, empty, unsupported rooted-but-not-absolute (`\notes`, `C:notes`), unresolvable, or exception-producing stored value falls back to `<resolved writer directory>\notes`. `writer_config.exe` does not save those fallback inputs. `[writer] directory`, `session_prompts.list`, and `task_list.txt` stay on the resolved Writer directory. This is not general filesystem path-syntax validation.

`writer_config.exe` writes `config/components/writer.ini` only. `--init` and `--seed` then delegate missing `session_prompts.list`, `task_list.txt`, and the notes directory to `writer_editor.exe`. An existing file is not replaced. A custom `directory` is followed; the compiled `writer` and `notes` values are used only when that configuration is missing. `writer_ac.exe` does not create these stores.
