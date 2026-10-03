# Wake tests

Release x64. `build-all.ps1` does not build this project. The executable is `obj\wake_tests\wake_tests.exe`.

```powershell
msbuild app\components\wake\tests\wake_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64
.\obj\wake_tests\wake_tests.exe "[wake][history]"
```

`wake_history_tests.cxx` covers the first capture, an identical repeat, a later change, a missing companion, an empty capture, an unreadable companion, and legacy names left in place. The provision check is file presence only. It does not read history contents and it does not run `powercfg`.

`wake_config.exe` writes `config/wake.ini` only. `--init`, `--seed`, and `--init --seed` then launch `wake_ac.exe --snapshot` only when `current.event`, `previous.event`, or `wake_events.log` is missing. An existing file is not opened. A missing `wake_ac.exe` leaves the INI in place and returns non-zero.
