# Logging policy configuration tests

`logging_policy_config_tests.vcxproj` covers seed precedence, `--disable`,
canonical prompt order, preservation of an existing file, failed writes,
the logging bootstrap prompts, and the first-run owner sequence.
`logging_policy_config.exe` stays outside that sequence because the logging
bootstrap launches it.

Build and run Release x64:

```powershell
msbuild logging_policy_config_tests.vcxproj /p:Configuration=Release /p:Platform=x64
..\..\..\..\..\obj\logging_policy_config_tests\logging_policy_config_tests.exe
```
