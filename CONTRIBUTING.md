# Contributing

Auto Core is a Windows 11 C++23 project. Production shape is **Release x64**.
Beta testers follow the same path: build, then run `dist\auto_core.exe`.

## Before you start

1. Read [README.md](README.md) (using vs building vs extending).
2. Skim [docs/development.md](docs/development.md),
   [docs/new-component.md](docs/new-component.md), and
   [docs/building.md](docs/building.md).
3. Do not commit `dist/` (live config, keymap, Spotify, journal, taskbar,
   server extras, logs, crash dumps, writer, notes) or `*.local.ixx` /
   `*.local.ini`. Tracked portable defaults live in
   [`defaults/`](defaults/) (documentation and tests only; Auto Core does
   not read them).

## Build

Visual Studio **2026** (version **18+**) / MSVC **v145** with the Desktop
C++ workload is required. From the repository root:

```powershell
.\scripts\build-all.ps1
```

Or follow the MSBuild order in [docs/building.md](docs/building.md). The
linker searches `lib/auto_core.lib`. There is no root `.sln`. After Link,
Windows PowerShell runs
[`scripts/copy-dist-dlls.ps1`](scripts/copy-dist-dlls.ps1), which copies
vendor runtime DLLs from `third_party/*/bin/` and `lib/auto_core.dll` into
`dist/` (`dist/` stays gitignored). Building `server_ac.exe` copies
`defaults/server/` into `dist/server/`.

## Adding a component

Follow [docs/new-component.md](docs/new-component.md): new project under
`app/components/<name>/`, import `msbuild/AutoCore.props` as
`..\..\..\msbuild\AutoCore.props`, advertise keymap names in the child's
hello catalog, and bind them in live `dist/keymap/bindings.ini`. A generic
child does not need a Main `register_with` entry.

## Tests

Build the DLL first, or use the tracked `lib/auto_core.dll`. Test
`OutDir` is `obj\<project>\`. Default Catch2 runs should exclude
`[live]` (`"~[live]"`). `build-all.ps1` does not build tests.

```powershell
msbuild app\core\vs\auto_core_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64
.\obj\auto_core_tests\auto_core_tests.exe

msbuild app\components\itunes\itunes_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64
.\obj\itunes_tests\itunes_tests.exe "~[live]"

msbuild app\components\spotify\spotify_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64
.\obj\spotify_tests\spotify_tests.exe "~[live]"
```

iTunes and Spotify notes: [app/components/itunes/TESTING.md](app/components/itunes/TESTING.md),
[app/components/spotify/TESTING.md](app/components/spotify/TESTING.md).

## Security reports

See [SECURITY.md](SECURITY.md). Do not file public issues that include
secrets or exploit steps.
