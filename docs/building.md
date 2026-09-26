# Building

Auto Core is intended for developer-configured use. Shared Visual Studio settings live in [`msbuild/AutoCore.props`](../msbuild/AutoCore.props). From the repository root, [`scripts/build-all.ps1`](../scripts/build-all.ps1) runs the Release x64 sequence below.

Build artifacts go to repo-root `obj/`, `lib/`, and `dist/`. Intermediate objects are under `obj/`. The core DLL `OutDir` is `lib/` (`auto_core.dll`, `auto_core.lib`, and gitignored `auto_core.exp`). After Link, Windows PowerShell runs [`scripts/copy-dist-dlls.ps1`](../scripts/copy-dist-dlls.ps1), which copies vendored runtime DLLs from [`third_party/*/bin/`](../third_party/) into `dist/bin/` and copies `lib/auto_core.dll` into `dist/bin/`. Every project that imports the props runs that script after Link. Building `server_ac.exe` copies [`defaults/server/`](../defaults/server/) into `dist/components/server/`. `dist/` is gitignored. Product third-party libraries live under `third_party/<dependency>/` (`include/`, `lib/`, `bin/`). Catch2 is amalgamated test source at `third_party/catch2/`. `msbuild/` contains source-controlled Visual Studio property sheets, not compiler output.

Wipe `obj/` anytime. Rebuilding the core DLL overwrites `lib/` in place. Paths are repo-relative in [`msbuild/AutoCore.props`](../msbuild/AutoCore.props); a clone does not edit that file.

Release binaries use the DLL CRT (`/MD`). If you run a `dist/` tree you did not build on that PC, install the matching **MSVC v145** redistributable. TagLib binary redistribution notes are in [NOTICE.md](../NOTICE.md).

## Requirements

- Windows 11
- Visual Studio 2026 (version 18+) with C++23 / MSVC v145 (Desktop development with C++ workload)
- Windows PowerShell 5.1+ (`powershell.exe`) for the post-Link DLL copy
- Basic knowledge of C++

## Setup

1. Clone or copy the project into a local development directory.
2. Shared paths live in `msbuild/AutoCore.props` (repo-relative; do not edit after a clone).
3. Build the core DLL, then the main executable, then any required component executables (`.\scripts\build-all.ps1` from the repo root).
4. Runtime files belong under `dist`. The whole tree is gitignored. Tracked portable defaults live in [`defaults/`](../defaults/). See [configuration.md](configuration.md).

## Solutions

There is no root `.sln`. Nested children live under `app/components/<name>/main` and `config` (Spotify also `oauth`; Spotify and iTunes also `star`). Each of those folders has its own `.sln`. Open these Visual Studio solutions:

| Project | Solution |
| --- | --- |
| Core DLL | [`app/core/vs/auto_core_dll.sln`](../app/core/vs/auto_core_dll.sln) |
| Main executable | [`app/main/runtime/vs/auto_core.sln`](../app/main/runtime/vs/auto_core.sln) |
| Auto Core config | [`app/main/config/auto_core/auto_core_config.sln`](../app/main/config/auto_core/auto_core_config.sln) |
| Components config | [`app/main/config/components/components_config.sln`](../app/main/config/components/components_config.sln) |
| Components editor | [`app/main/editors/components/components_editor.sln`](../app/main/editors/components/components_editor.sln) |
| Keymap config | [`app/main/config/keymap/keymap_config.sln`](../app/main/config/keymap/keymap_config.sln) |
| Keymap editor | [`app/main/editors/keymap/keymap_editor.sln`](../app/main/editors/keymap/keymap_editor.sln) |
| Shutdown config | [`app/main/config/shutdown/shutdown_config.sln`](../app/main/config/shutdown/shutdown_config.sln) |
| Crash recovery config | [`app/main/config/crash_recovery/crash_recovery_config.sln`](../app/main/config/crash_recovery/crash_recovery_config.sln) |
| Dash | [`app/components/dash/main/dash.sln`](../app/components/dash/main/dash.sln) |
| Dash config | [`app/components/dash/config/dash_config.sln`](../app/components/dash/config/dash_config.sln) |
| iTunes | [`app/components/itunes/main/itunes.sln`](../app/components/itunes/main/itunes.sln) |
| iTunes config | [`app/components/itunes/config/itunes_config.sln`](../app/components/itunes/config/itunes_config.sln) |
| iTunes star | [`app/components/itunes/star/itunes_star.sln`](../app/components/itunes/star/itunes_star.sln) |
| Journal | [`app/components/journal/main/journal.sln`](../app/components/journal/main/journal.sln) |
| Journal config | [`app/components/journal/config/journal_config.sln`](../app/components/journal/config/journal_config.sln) |
| Logger | [`app/components/logger/main/logger.sln`](../app/components/logger/main/logger.sln) |
| Logger config | [`app/components/logger/config/logger_config.sln`](../app/components/logger/config/logger_config.sln) |
| Server | [`app/components/server/main/server.sln`](../app/components/server/main/server.sln) |
| Server config | [`app/components/server/config/server_config.sln`](../app/components/server/config/server_config.sln) |
| Slash | [`app/components/slash/main/slash.sln`](../app/components/slash/main/slash.sln) |
| Slash config | [`app/components/slash/config/slash_config.sln`](../app/components/slash/config/slash_config.sln) |
| Spotify | [`app/components/spotify/main/spotify.sln`](../app/components/spotify/main/spotify.sln) |
| Spotify config | [`app/components/spotify/config/spotify_config.sln`](../app/components/spotify/config/spotify_config.sln) |
| Spotify OAuth | [`app/components/spotify/oauth/spotify_oauth.sln`](../app/components/spotify/oauth/spotify_oauth.sln) |
| Spotify star | [`app/components/spotify/star/spotify_star.sln`](../app/components/spotify/star/spotify_star.sln) |
| Taskbar | [`app/components/taskbar/main/taskbar.sln`](../app/components/taskbar/main/taskbar.sln) |
| Taskbar config | [`app/components/taskbar/config/taskbar_config.sln`](../app/components/taskbar/config/taskbar_config.sln) |
| Wake | [`app/components/wake/main/wake.sln`](../app/components/wake/main/wake.sln) |
| Wake config | [`app/components/wake/config/wake_config.sln`](../app/components/wake/config/wake_config.sln) |
| Writer | [`app/components/writer/main/writer.sln`](../app/components/writer/main/writer.sln) |
| Writer config | [`app/components/writer/config/writer_config.sln`](../app/components/writer/config/writer_config.sln) |

Production tests:

| Suite | Project |
| --- | --- |
| Core DLL | [`app/core/vs/auto_core_tests.vcxproj`](../app/core/vs/auto_core_tests.vcxproj) |
| Main | [`app/main/runtime/vs/auto_core_main_tests.vcxproj`](../app/main/runtime/vs/auto_core_main_tests.vcxproj) |
| iTunes | [`app/components/itunes/tests/itunes_tests.vcxproj`](../app/components/itunes/tests/itunes_tests.vcxproj) |
| Spotify | [`app/components/spotify/tests/spotify_tests.vcxproj`](../app/components/spotify/tests/spotify_tests.vcxproj) |

iTunes and Spotify test notes: [`app/components/itunes/tests/TESTING.md`](../app/components/itunes/tests/TESTING.md), [`app/components/spotify/tests/TESTING.md`](../app/components/spotify/tests/TESTING.md). `build-all.ps1` does not build tests. Test `OutDir` is `obj\<project>\`. From the repository root, Release x64:

```powershell
msbuild app\core\vs\auto_core_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64
.\obj\auto_core_tests\auto_core_tests.exe

msbuild app\main\runtime\vs\auto_core_main_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64
.\obj\auto_core_main_tests\auto_core_main_tests.exe

msbuild app\components\itunes\tests\itunes_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64
.\obj\itunes_tests\itunes_tests.exe "~[live]"

msbuild app\components\spotify\tests\spotify_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64
.\obj\spotify_tests\spotify_tests.exe "~[live]"
```

## MSBuild order

From the repository root, Release x64, run [`scripts/build-all.ps1`](../scripts/build-all.ps1). That script lists every nested `main` / `config` / `oauth` / `star` solution. Build the DLL before any executable that links `auto_core.lib`, or use the tracked `lib/auto_core.lib`. The linker searches `lib/`.

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
| `AutoCoreBinDir` | `dist\bin\` (executables, `auto_core.dll`, and copied vendor DLLs) |
| `AutoCoreSymbolsDir` | `dist\symbols\` |

Application `OutDir` is `dist\bin\`. The core DLL `OutDir` is `lib\`. `IntDir` is `obj\<project>\`.

An optional local directory junction or symlink for `obj` is not required for a clone. If you use one, point it at `obj` yourself; do not commit it.
