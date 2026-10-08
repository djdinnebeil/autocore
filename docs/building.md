# Building

Auto Core is intended for developer-configured use. Shared Visual Studio settings live in [`msbuild/AutoCore.props`](../msbuild/AutoCore.props). From the repository root, [`scripts/build-all.ps1`](../scripts/build-all.ps1) runs the Release x64 sequence below.

Build artifacts go to repo-root `obj/`, `lib/`, `bin/`, and `dist/`. Intermediate objects are under `obj/`. The core DLL `OutDir` is `lib/` (`auto_core.dll`, `auto_core.lib`, and gitignored `auto_core.exp`). After `auto_core_dll` links, its `PublishAutoCoreDll` target copies `lib/auto_core.dll` to `bin/auto_core.dll`. Application executables publish to `bin/`. [`scripts/publish-dist.ps1`](../scripts/publish-dist.ps1) is the only normal publisher of `dist/bin/`; [`scripts/build-all.ps1`](../scripts/build-all.ps1) runs it after a successful build. [`scripts/copy-vendor-dlls.ps1`](../scripts/copy-vendor-dlls.ps1) is a manual refresh of vendored runtime DLLs from [`third_party/*/bin/`](../third_party/) into `bin/`, then it runs `publish-dist.ps1`. Normal builds do not run that script. The build does not create the Server data directory. `server_config.exe --seed` writes a missing `config/server.ini`, then `server_editor.exe --seed` creates missing `port.id` and `document_root.id`, then `server_builder.exe --seed` creates missing `index.html` and `styles.css` under the configured relative document root (default `dist/components/server/site`). `dist/` is gitignored. Product third-party libraries live under `third_party/<dependency>/` (`include/`, `lib/`, `bin/`). Catch2 is amalgamated test source at `third_party/catch2/`. `msbuild/` contains source-controlled Visual Studio property sheets, not compiler output.

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

## Shortcut identity

`Auto Core.lnk` at the installation root targets `bin\auto_core.exe` in that same root. The repository does not create this shortcut and does not install Auto Core.

Before the Task Manager field test, stamp the existing link with `System.AppUserModel.ID` = `Djdinn.AutoCore`. [`scripts/stamp_auto_core_shortcut.cxx`](../scripts/stamp_auto_core_shortcut.cxx) obtains `IPropertyStore` from `IShellLink`, calls `SetValue(PKEY_AppUserModel_ID, ...)`, commits, and saves the link.

From the repository root, Release x64:

```powershell
msbuild scripts\stamp_auto_core_shortcut.vcxproj /m /p:Configuration=Release /p:Platform=x64
.\obj\stamp_auto_core_shortcut\stamp_auto_core_shortcut.exe "<installation_root>\Auto Core.lnk"
```

The tool writes `obj\stamp_auto_core_shortcut\stamp_auto_core_shortcut.exe`. It is a field-test stamp, not a product runtime and not an installer. If the link is missing, the tool reports that and exits. Create the link so it targets `<installation_root>\bin\auto_core.exe`, then run the tool again.

## Solutions

A `.sln` is a workspace. A `.vcxproj` is one EXE, DLL, or test executable. A directory is an ownership boundary. An executable does not get its own `.sln`.

[`app/AutoCore.sln`](../app/AutoCore.sln) contains every product and test project. Building that solution Release x64 builds the shipped DLL and executables. Test projects are in the solution and are not built by that default build. Build a test by selecting its project, or by invoking its `.vcxproj` with MSBuild.

The twelve family workspaces are:

| Workspace | Solution |
| --- | --- |
| Core | [`app/core/Core.sln`](../app/core/Core.sln) |
| Main | [`app/main/Main.sln`](../app/main/Main.sln) |
| Dash | [`app/components/dash/Dash.sln`](../app/components/dash/Dash.sln) |
| iTunes | [`app/components/itunes/iTunes.sln`](../app/components/itunes/iTunes.sln) |
| Journal | [`app/components/journal/Journal.sln`](../app/components/journal/Journal.sln) |
| Logger | [`app/components/logger/Logger.sln`](../app/components/logger/Logger.sln) |
| Server | [`app/components/server/Server.sln`](../app/components/server/Server.sln) |
| Slash | [`app/components/slash/Slash.sln`](../app/components/slash/Slash.sln) |
| Spotify | [`app/components/spotify/Spotify.sln`](../app/components/spotify/Spotify.sln) |
| Taskbar | [`app/components/taskbar/Taskbar.sln`](../app/components/taskbar/Taskbar.sln) |
| Wake | [`app/components/wake/Wake.sln`](../app/components/wake/Wake.sln) |
| Writer | [`app/components/writer/Writer.sln`](../app/components/writer/Writer.sln) |

Component hosted executables live in `runtime/`. Other responsibility directories (`config`, `star`, `db`, `editor`, `builder`, `formatter`, `oauth`, `clock`, `cloud`, `series`, `shared`, `tests`) stay beside it. To work on `itunes_db`, open `iTunes.sln` and build that project, or run MSBuild on `app\components\itunes\db\itunes_db.vcxproj`.

Production tests:

| Suite | Project |
| --- | --- |
| Core DLL | [`app/core/tests/auto_core_tests.vcxproj`](../app/core/tests/auto_core_tests.vcxproj) |
| Main | [`app/main/tests/auto_core_main_tests.vcxproj`](../app/main/tests/auto_core_main_tests.vcxproj) |
| iTunes | [`app/components/itunes/tests/itunes_tests.vcxproj`](../app/components/itunes/tests/itunes_tests.vcxproj) |
| Journal | [`app/components/journal/tests/journal_tests.vcxproj`](../app/components/journal/tests/journal_tests.vcxproj) |
| Server | [`app/components/server/tests/server_tests.vcxproj`](../app/components/server/tests/server_tests.vcxproj) |
| Spotify | [`app/components/spotify/tests/spotify_tests.vcxproj`](../app/components/spotify/tests/spotify_tests.vcxproj) |
| Writer | [`app/components/writer/tests/writer_tests.vcxproj`](../app/components/writer/tests/writer_tests.vcxproj) |

iTunes, Spotify, Journal, Server, and Writer test notes: [`app/components/itunes/tests/TESTING.md`](../app/components/itunes/tests/TESTING.md), [`app/components/spotify/tests/TESTING.md`](../app/components/spotify/tests/TESTING.md), [`app/components/journal/tests/TESTING.md`](../app/components/journal/tests/TESTING.md), [`app/components/server/tests/TESTING.md`](../app/components/server/tests/TESTING.md), [`app/components/writer/tests/TESTING.md`](../app/components/writer/tests/TESTING.md). `build-all.ps1` does not build tests. Test `OutDir` is `obj\<project>\`. From the repository root, Release x64:

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

msbuild app\components\writer\tests\writer_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64
.\obj\writer_tests\writer_tests.exe "[writer][unit]"
```

## MSBuild order

From the repository root, Release x64, run [`scripts/build-all.ps1`](../scripts/build-all.ps1). It builds [`app/AutoCore.sln`](../app/AutoCore.sln). That default build is the shipped DLL and executables. It does not build tests. [`scripts/build-all-components.ps1`](../scripts/build-all-components.ps1) builds the twelve family solutions in order, Core first. Build the DLL before any executable that links `auto_core.lib`, or use the tracked `lib/auto_core.lib`. The linker searches `lib/`.

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
| `AutoCoreBinDir` | `bin\` (executables, `auto_core.dll` published by `auto_core_dll`, and vendor DLLs from `scripts/copy-vendor-dlls.ps1`) |
| `AutoCoreSymbolsDir` | `symbols\` (repository root; linker `.pdb` files) |

Application `OutDir` is `bin\`. The core DLL `OutDir` is `lib\`. `IntDir` is `obj\<project>\`. [`scripts/publish-dist.ps1`](../scripts/publish-dist.ps1) copies `bin\*.exe` and `bin\*.dll` into `dist\bin\`.

An optional local directory junction or symlink for `obj` is not required for a clone. If you use one, point it at `obj` yourself; do not commit it.
