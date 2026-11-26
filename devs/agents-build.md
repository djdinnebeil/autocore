# Auto Core build instructions for agents

Use this workflow after making changes that affect the Auto Core build. DJ handles testing and runtime validation.

## Build

Build on Windows 11 with the Visual Studio C++ toolchain required by the checkout. Open a PowerShell session where `devenv.com` is on PATH. See [Auto Core's build documentation](building.md) for project-specific setup.

Run [the agent build script](../scripts/agents-build.ps1) from the repository root.

```powershell
.\scripts\agents-build.ps1
```

The script builds the production solution, `app\AutoCore.sln`, in Release x64 and ends when the build completes.

## Resolve build failures

1. Read the build output and identify the first actionable error. Fix compilation or linker errors introduced by your changes. Address recoverable setup issues within the task's scope.
2. Rerun `.\scripts\agents-build.ps1` after each fix. Continue until the build exits successfully with zero compilation or linker errors.
3. If an unrelated failure or a missing prerequisite blocks further progress, report the failing project or stage, the relevant diagnostic, and what is needed to proceed.

## Report the result

Give DJ the final build result and any relevant warnings. Leave application launches, automated test runs, and runtime validation to DJ.
