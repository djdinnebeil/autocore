# Slash tests

`slash_tests.vcxproj` covers the configuration action selection, exact seed
text, interactive defaults, accepted mode and logging values, invalid-input
reprompting, preservation decisions, and no-argument Configure prompts.

Build and run Release x64:

```powershell
msbuild slash_tests.vcxproj /p:Configuration=Release /p:Platform=x64
..\..\..\..\obj\slash_tests\slash_tests.exe
```
