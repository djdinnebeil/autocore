# Contributing

Auto Core is a Windows 11 C++23 project. Production shape is **Release x64**.
Beta testers follow the same path: build, then run `dist\bin\auto_core.exe`.

## Before you start

1. Read [README.md](README.md) (using vs building vs extending).
2. Skim [docs/development.md](docs/development.md),
   [docs/new-component.md](docs/new-component.md), and
   [docs/building.md](docs/building.md).
3. Do not commit `dist/` (live config, keymap, Spotify, journal, taskbar,
   server files, logs, crash dumps, writer, notes) or `*.local.ixx` /
   `*.local.ini`. Portable defaults are compiled in each child's
   `shared/defaults.ixx` and in `app/main/shared/defaults.ixx`.

## Build

Visual Studio **2026** (version **18+**) / MSVC **v145** with the Desktop
C++ workload is required. From the repository root:

```powershell
.\scripts\build-all.ps1
```

Or follow the MSBuild order in [docs/building.md](docs/building.md). The
linker searches `lib/auto_core.lib`. The repository workspace is
`app/AutoCore.sln`. `auto_core_dll` publishes `lib/auto_core.dll` to
`bin/auto_core.dll`. Application executables publish to tracked `bin/`.
[`scripts/publish-dist.ps1`](scripts/publish-dist.ps1) is the only normal
publisher of `dist/bin/`. [`scripts/copy-vendor-dlls.ps1`](scripts/copy-vendor-dlls.ps1)
copies vendor runtime DLLs from `third_party/*/bin/` into `bin/` when
those DLLs are added or updated, then runs `publish-dist.ps1`. Builds do
not run that script (`dist/` stays gitignored; `bin/` is tracked). The build does not plant the Server
data directory. `server_editor.exe --seed` creates missing `port.id` and
`document_root.id`. `server_builder.exe` creates missing starter files
under a relative document root (default `dist/components/server/site`).

## Adding a component

Follow [docs/new-component.md](docs/new-component.md): new project under
`app/components/<name>/runtime/` (and `config/`, `shared/`), import
`msbuild/AutoCore.props` as `..\..\..\..\msbuild\AutoCore.props`, advertise keymap names in the child's
hello catalog, and bind them in live `dist/keymap.map`. A generic
child does not need a Main `register_with` entry.

## Tests

Build the DLL first, or use the tracked `lib/auto_core.dll`. Test
`OutDir` is `obj\<project>\`. Default Catch2 runs should exclude
`[live]` (`"~[live]"`). `build-all.ps1` does not build tests.

```powershell
msbuild app\core\tests\auto_core_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64
.\obj\auto_core_tests\auto_core_tests.exe

msbuild app\components\itunes\tests\itunes_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64
.\obj\itunes_tests\itunes_tests.exe "~[live]"

msbuild app\components\spotify\tests\spotify_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64
.\obj\spotify_tests\spotify_tests.exe "~[live]"
```

iTunes and Spotify notes: [app/components/itunes/tests/TESTING.md](app/components/itunes/tests/TESTING.md),
[app/components/spotify/tests/TESTING.md](app/components/spotify/tests/TESTING.md).

## Security reports

See [SECURITY.md](SECURITY.md). Do not file public issues that include
secrets or exploit steps.
