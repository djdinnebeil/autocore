# Logging configuration tests

`logging_config_tests.vcxproj` covers seed precedence, `--disable`,
canonical prompt order, preservation of an existing file, failed writes,
the logging bootstrap prompts, and the first-run owner sequence.

Build and run Release x64:

```powershell
msbuild logging_config_tests.vcxproj /p:Configuration=Release /p:Platform=x64
..\..\..\..\..\obj\logging_config_tests\logging_config_tests.exe
```
