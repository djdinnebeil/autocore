# Building

Auto Core is intended for developer-configured use. Shared Visual Studio settings live in [`msbuild/AutoCore.props`](../msbuild/AutoCore.props). From the repository root, [`scripts/build-all.ps1`](../scripts/build-all.ps1) runs the Release x64 sequence below.

Build artifacts go to repo-root `obj/`, `lib/`, `bin/`, and `dist/`. Intermediate objects are under `obj/`. The core DLL `OutDir` is `lib/` (`auto_core.dll`, `auto_core.lib`, and gitignored `auto_core.exp`). After `auto_core_dll` links, its `PublishAutoCoreDll` target copies `lib/auto_core.dll` to `bin/app/auto_core.dll`. Main executables publish to `bin/app/`. Component executables publish to `bin/components/`. [`scripts/update-dist.ps1`](../scripts/update-dist.ps1) is the only normal publisher of `dist/app/` and `dist/components/`; [`scripts/build-all.ps1`](../scripts/build-all.ps1) runs it after a successful build. [`scripts/copy-vendor-dlls.ps1`](../scripts/copy-vendor-dlls.ps1) is a manual refresh of vendored runtime DLLs from [`third_party/*/bin/`](../third_party/) into `bin/app/`, then it runs `update-dist.ps1`. Normal builds do not run that script. The build does not create the Server data directory. `server_ac.exe --config --seed` writes a missing `config/components/server.ini`, then `server_ac.exe --editor --seed` creates missing `port.id` and `document_root.id`, then `server_ac.exe --builder --seed` creates missing `index.html` and `styles.css` under the configured relative document root (default `dist/components/server/site`). `dist/` is gitignored. Product third-party libraries live under `third_party/<dependency>/` (`include/`, `lib/`, `bin/`). Catch2 is amalgamated test source at `third_party/catch2/`. `msbuild/` contains source-controlled Visual Studio property sheets, not compiler output.

Wipe `obj/` anytime. Rebuilding the core DLL overwrites `lib/` in place. Paths are repo-relative in [`msbuild/AutoCore.props`](../msbuild/AutoCore.props); a clone does not edit that file.

Release binaries use the DLL CRT (`/MD`). If you run a `dist/` tree you did not build on that PC, install the matching **MSVC v145** redistributable. TagLib binary redistribution notes are in [NOTICE.md](../NOTICE.md).

## Requirements

- Windows 11
- Visual Studio 2026 (version 18+) with C++23 / MSVC v145 (Desktop development with C++ workload)
- Windows PowerShell 5.1+ (`powershell.exe`) for [`scripts/build-all.ps1`](../scripts/build-all.ps1) and the manual vendor DLL refresh [`scripts/copy-vendor-dlls.ps1`](../scripts/copy-vendor-dlls.ps1)
- Basic knowledge of C++

## Setup

1. Clone or copy the project into a local development directory.
2. Shared paths live in `msbuild/AutoCore.props` (repo-relative; do not edit after a clone).
3. Build the core DLL, then the main executable, then any required component executables (`.\scripts\build-all.ps1` from the repo root).
4. Runtime files belong under `dist`. The whole tree is gitignored. Portable defaults are compiled in each child's `shared/defaults.ixx` and in [`app/main/shared/defaults.ixx`](../app/main/shared/defaults.ixx). See [configuration.md](configuration.md).

## Solutions

A `.sln` is a workspace. A `.vcxproj` is one EXE, DLL, or test executable. A directory is an ownership boundary. An executable does not get its own `.sln`.

[`app/AutoCore.sln`](../app/AutoCore.sln) is the production solution. Building it Release x64 builds the shipped DLL and executables. It does not contain test projects.

[`app/Tests.sln`](../app/Tests.sln) builds the complete test suite. A Release x64 build of that solution builds every test project and `auto_core_dll`. Build one suite by selecting its project, or by invoking its `.vcxproj` with MSBuild.

The eleven family workspaces are:

| Workspace | Solution |
| --- | --- |
| Core | [`app/core/Core.sln`](../app/core/Core.sln) |
| Main | [`app/main/Main.sln`](../app/main/Main.sln) |
| Dash | [`app/components/dash/Dash.sln`](../app/components/dash/Dash.sln) |
| iTunes | [`app/components/itunes/iTunes.sln`](../app/components/itunes/iTunes.sln) |
| Journal | [`app/components/journal/Journal.sln`](../app/components/journal/Journal.sln) |
| Server | [`app/components/server/Server.sln`](../app/components/server/Server.sln) |
| Slash | [`app/components/slash/Slash.sln`](../app/components/slash/Slash.sln) |
| Spotify | [`app/components/spotify/Spotify.sln`](../app/components/spotify/Spotify.sln) |
| Taskbar | `auto_core_taskbar` in [`app/AutoCore.sln`](../app/AutoCore.sln), project [`app/main/taskbar/auto_core_taskbar.vcxproj`](../app/main/taskbar/auto_core_taskbar.vcxproj) |
| Wake | [`app/components/wake/Wake.sln`](../app/components/wake/Wake.sln) |
| Writer | [`app/main/writer/Writer.sln`](../app/main/writer/Writer.sln) |

Component hosted executables live in `runtime/`. Other responsibility directories (`config`, `settings`, `db`, `editor`, `builder`, `formatter`, `oauth`, `clock`, `cloud`, `series`, `shared`, `tests`) stay beside it. `db/` is the SQLite module compiled into `journal_ac.exe`. `--run` and `--series` use that same in-process store. To work on iTunes history, open `iTunes.sln` and build `itunes`, or run MSBuild on `app\components\itunes\runtime\itunes.vcxproj`.

Production tests:

| Suite | Project |
| --- | --- |
| Core DLL | [`app/core/tests/auto_core_tests.vcxproj`](../app/core/tests/auto_core_tests.vcxproj) |
| Main | [`app/main/tests/auto_core_main_tests.vcxproj`](../app/main/tests/auto_core_main_tests.vcxproj) |
| iTunes | [`app/components/itunes/tests/itunes_tests.vcxproj`](../app/components/itunes/tests/itunes_tests.vcxproj) |
| Journal | [`app/components/journal/tests/journal_tests.vcxproj`](../app/components/journal/tests/journal_tests.vcxproj) |
| Server | [`app/components/server/tests/server_tests.vcxproj`](../app/components/server/tests/server_tests.vcxproj) |
| Spotify | [`app/components/spotify/tests/spotify_tests.vcxproj`](../app/components/spotify/tests/spotify_tests.vcxproj) |
| Writer | [`app/main/writer/tests/writer_tests.vcxproj`](../app/main/writer/tests/writer_tests.vcxproj) |

iTunes, Spotify, Journal, Server, and Writer test notes: [`app/components/itunes/tests/TESTING.md`](../app/components/itunes/tests/TESTING.md), [`app/components/spotify/tests/TESTING.md`](../app/components/spotify/tests/TESTING.md), [`app/components/journal/tests/TESTING.md`](../app/components/journal/tests/TESTING.md), [`app/components/server/tests/TESTING.md`](../app/components/server/tests/TESTING.md), [`app/main/writer/tests/TESTING.md`](../app/main/writer/tests/TESTING.md). From the repository root, Release x64, [`app/Tests.sln`](../app/Tests.sln) builds the complete suite. `build-all.ps1` builds [`app/AutoCore.sln`](../app/AutoCore.sln) and does not build tests. Test `OutDir` is `obj\<project>\`. To build one suite:

```powershell
msbuild app\core\tests\auto_core_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64
.\obj\auto_core_tests\auto_core_tests.exe

msbuild app\main\tests\auto_core_main_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64
.\obj\auto_core_main_tests\auto_core_main_tests.exe

msbuild app\components\itunes\tests\itunes_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64
.\obj\itunes_tests\itunes_tests.exe "~[live]"

msbuild app\components\spotify\tests\spotify_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64
.\obj\spotify_tests\spotify_tests.exe "~[live]"

msbuild app\components\journal\tests\journal_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64
.\obj\journal_tests\journal_tests.exe "[journal][unit]"

msbuild app\components\server\tests\server_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64
.\obj\server_tests\server_tests.exe "[server][unit]"

msbuild app\main\writer\tests\writer_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64
.\obj\writer_tests\writer_tests.exe "[writer][unit]"
```

## MSBuild order

From the repository root, Release x64, run [`scripts/build-all.ps1`](../scripts/build-all.ps1). It builds [`app/AutoCore.sln`](../app/AutoCore.sln). That default build is the shipped DLL and executables. It does not enumerate test projects. Build the complete test suite with [`app/Tests.sln`](../app/Tests.sln). [`scripts/build-all-components.ps1`](../scripts/build-all-components.ps1) builds the eleven family solutions in order, Core first. Build the DLL before any executable that links `auto_core.lib`, or use the tracked `lib/auto_core.lib`. The linker searches `lib/`.

## Output directories

[`AutoCore.props`](../msbuild/AutoCore.props) defines the directories and injects include, library, `IntDir`, and `OutDir` settings into every project that imports it:

| Macro | Location |
| --- | --- |
| `AutoCoreRootDir` | Repository root |
| `AutoCoreIntDir` | `obj\` (intermediate objects; safe to delete) |
| `AutoCoreLibDir` | `lib\` (canonical `auto_core.lib` and `auto_core.dll`) |
| `AutoCoreThirdPartyDir` | `third_party\` (per-dependency `include\`, `lib\`, `bin\`) |
| `AutoCoreMacroDir` | `app\core\include\` (`ac_api.hpp`) |
| `AutoCoreDistDir` | `dist\` (installation root) |
| `AutoCoreBinDir` | `bin\` |
| `AutoCoreBinAppDir` | `bin\app\` (Main executables, `auto_core.dll` published by `auto_core_dll`, and vendor DLLs from `scripts/copy-vendor-dlls.ps1`) |
| `AutoCoreBinComponentsDir` | `bin\components\` (`<name>_ac.exe`) |
| `AutoCoreSymbolsDir` | `symbols\` (repository root; linker `.pdb` files) |

Main application `OutDir` is `bin\app\`. Component application `OutDir` is `bin\components\`. The core DLL `OutDir` is `lib\`. `IntDir` is `obj\<project>\`. [`scripts/update-dist.ps1`](../scripts/update-dist.ps1) copies `bin\app\` into `dist\app\`, and `bin\components\*_ac.exe` into `dist\components\`. A third-party `*_ac.exe` already in `dist\components\` is left in place. `dist\app` still replaces its own `.exe` and `.dll` files, which removes component EXEs published there by older builds.

An optional local directory junction or symlink for `obj` is not required for a clone. If you use one, point it at `obj` yourself; do not commit it.
