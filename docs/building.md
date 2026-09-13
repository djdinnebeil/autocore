# Building

Auto Core is intended for developer-configured use. Shared Visual Studio settings live in [`app/build/AutoCore.props`](../app/build/AutoCore.props). From the repository root, [`scripts/build-all.ps1`](../scripts/build-all.ps1) runs the Release x64 sequence below.

Build artifacts go to repo-root `out/` and `dist/`. Intermediate objects are under `out/obj/`. The core DLL `OutDir` is `out/core/` (`auto_core.lib`, `auto_core.exp`, and a build copy of `auto_core.dll`). After that project links, `auto_core.dll` is copied into `dist/` and `lib/`, and `auto_core.lib` into `lib/`. After Link, Windows PowerShell runs [`scripts/copy-dist-dlls.ps1`](../scripts/copy-dist-dlls.ps1), which copies vendored runtime DLLs from [`third_party/*/bin/`](../third_party/) into `dist/` and copies `auto_core.dll` into `dist/` (`out/core` first, then `lib/`). Every project that imports the props runs that script after Link. Building `server_ac.exe` copies [`defaults/server/`](../defaults/server/) into `dist/server/`. `dist/` is gitignored. Product third-party libraries live under `third_party/<dependency>/` (`include/`, `lib/`, `bin/`). Catch2 is amalgamated test source at `third_party/catch2/`. `app/build/` contains source-controlled build configuration, not compiler output.

Wipe `out/obj/` anytime. Do not delete `out/core/` unless you intend to rebuild the core DLL. Paths are repo-relative in [`app/build/AutoCore.props`](../app/build/AutoCore.props); a clone does not edit that file.

Release binaries use the DLL CRT (`/MD`). If you run a `dist/` tree you did not build on that PC, install the matching **MSVC v145** redistributable. TagLib binary redistribution notes are in [NOTICE.md](../NOTICE.md).

## Requirements

- Windows 11
- Visual Studio 2026 (version 18+) with C++23 / MSVC v145 (Desktop development with C++ workload)
- Windows PowerShell 5.1+ (`powershell.exe`) for the post-Link DLL copy
- Basic knowledge of C++

## Setup

1. Clone or copy the project into a local development directory.
2. Shared paths live in `app/build/AutoCore.props` (repo-relative; do not edit after a clone).
3. Build the core DLL, then the main executable, then any required component executables (`.\scripts\build-all.ps1` from the repo root).
4. Runtime files belong under `dist`. The whole tree is gitignored. Tracked portable defaults live in [`defaults/`](../defaults/). See [configuration.md](configuration.md).

## Solutions

There is no root `.sln`. Open these Visual Studio solutions:

| Project | Solution |
| --- | --- |
| Core DLL | [`app/core/vs/auto_core_dll.sln`](../app/core/vs/auto_core_dll.sln) |
| Main executable | [`app/main/vs/auto_core.sln`](../app/main/vs/auto_core.sln) |
| Dash | [`app/components/dash/dash.sln`](../app/components/dash/dash.sln) |
| iTunes | [`app/components/itunes/itunes.sln`](../app/components/itunes/itunes.sln) |
| Journal | [`app/components/journal/journal.sln`](../app/components/journal/journal.sln) |
| Journal config | [`app/components/journal_config/journal_config.sln`](../app/components/journal_config/journal_config.sln) |
| Logger | [`app/components/logger/logger.sln`](../app/components/logger/logger.sln) |
| Server | [`app/components/server/server.sln`](../app/components/server/server.sln) |
| Server config | [`app/components/server_config/server_config.sln`](../app/components/server_config/server_config.sln) |
| Slash | [`app/components/slash/slash.sln`](../app/components/slash/slash.sln) |
| Spotify | [`app/components/spotify/spotify.sln`](../app/components/spotify/spotify.sln) |
| Spotify OAuth | [`app/components/spotify_oauth/spotify_oauth.sln`](../app/components/spotify_oauth/spotify_oauth.sln) |
| Taskbar | [`app/components/taskbar/taskbar.sln`](../app/components/taskbar/taskbar.sln) |
| Taskbar config | [`app/components/taskbar_config/taskbar_config.sln`](../app/components/taskbar_config/taskbar_config.sln) |
| Wake | [`app/components/wake/wake.sln`](../app/components/wake/wake.sln) |
| Writer | [`app/components/writer/writer.sln`](../app/components/writer/writer.sln) |
| Writer config | [`app/components/writer_config/writer_config.sln`](../app/components/writer_config/writer_config.sln) |

Production tests:

| Suite | Project |
| --- | --- |
| Core DLL | [`app/core/vs/auto_core_tests.vcxproj`](../app/core/vs/auto_core_tests.vcxproj) |
| iTunes | [`app/components/itunes/itunes_tests.vcxproj`](../app/components/itunes/itunes_tests.vcxproj) |
| Spotify | [`app/components/spotify/spotify_tests.vcxproj`](../app/components/spotify/spotify_tests.vcxproj) |

iTunes and Spotify test notes: [`app/components/itunes/TESTING.md`](../app/components/itunes/TESTING.md), [`app/components/spotify/TESTING.md`](../app/components/spotify/TESTING.md). `build-all.ps1` does not build tests. Test `OutDir` is `out\obj\<project>\`. From the repository root, Release x64:

```powershell
msbuild app\core\vs\auto_core_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64
.\out\obj\auto_core_tests\auto_core_tests.exe

msbuild app\components\itunes\itunes_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64
.\out\obj\itunes_tests\itunes_tests.exe "~[live]"

msbuild app\components\spotify\spotify_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64
.\out\obj\spotify_tests\spotify_tests.exe "~[live]"
```

## MSBuild order

From the repository root, Release x64:

```text
msbuild "app\core\vs\auto_core_dll.sln" /m /t:Build /p:Configuration=Release /p:Platform=x64
msbuild "app\main\vs\auto_core.sln" /m /t:Build /p:Configuration=Release /p:Platform=x64
msbuild "app\components\dash\dash.sln" /m /t:Build /p:Configuration=Release /p:Platform=x64
msbuild "app\components\itunes\itunes.sln" /m /t:Build /p:Configuration=Release /p:Platform=x64
msbuild "app\components\journal\journal.sln" /m /t:Build /p:Configuration=Release /p:Platform=x64
msbuild "app\components\journal_config\journal_config.sln" /m /t:Build /p:Configuration=Release /p:Platform=x64
msbuild "app\components\logger\logger.sln" /m /t:Build /p:Configuration=Release /p:Platform=x64
msbuild "app\components\server\server.sln" /m /t:Build /p:Configuration=Release /p:Platform=x64
msbuild "app\components\server_config\server_config.sln" /m /t:Build /p:Configuration=Release /p:Platform=x64
msbuild "app\components\slash\slash.sln" /m /t:Build /p:Configuration=Release /p:Platform=x64
msbuild "app\components\spotify\spotify.sln" /m /t:Build /p:Configuration=Release /p:Platform=x64
msbuild "app\components\spotify_oauth\spotify_oauth.sln" /m /t:Build /p:Configuration=Release /p:Platform=x64
msbuild "app\components\taskbar\taskbar.sln" /m /t:Build /p:Configuration=Release /p:Platform=x64
msbuild "app\components\taskbar_config\taskbar_config.sln" /m /t:Build /p:Configuration=Release /p:Platform=x64
msbuild "app\components\wake\wake.sln" /m /t:Build /p:Configuration=Release /p:Platform=x64
msbuild "app\components\writer\writer.sln" /m /t:Build /p:Configuration=Release /p:Platform=x64
msbuild "app\components\writer_config\writer_config.sln" /m /t:Build /p:Configuration=Release /p:Platform=x64

```

Build the DLL before any executable that links `auto_core.lib`, or use the tracked `lib/auto_core.lib` seed. The linker searches `out/core/` first, then `lib/`.

## Output directories

[`AutoCore.props`](../app/build/AutoCore.props) defines the directories and injects include, library, `IntDir`, and `OutDir` settings into every project that imports it:

| Macro | Location |
| --- | --- |
| `AutoCoreRootDir` | Repository root |
| `AutoCoreBuildDir` | `out\` |
| `AutoCoreIntDir` | `out\obj\` (intermediate objects; safe to delete) |
| `AutoCoreLinkDir` | `out\core\` (generated `auto_core.lib`, `.exp`, extra `auto_core.dll`) |
| `AutoCoreSeedDir` | `lib\` (tracked `auto_core.lib` and `auto_core.dll`) |
| `AutoCoreThirdPartyDir` | `third_party\` (per-dependency `include\`, `lib\`, `bin\`) |
| `AutoCoreMacroDir` | `app\core\include\` (`ac_api.hpp`) |
| `AutoCoreDistDir` | `dist\` (executables, `auto_core.dll`, and copied vendor DLLs) |
| `AutoCoreSymbolsDir` | `dist\symbols\` |

Application `OutDir` is `dist\`. The core DLL `OutDir` is `out\core\`. `IntDir` is `out\obj\<project>\`.

An optional local directory junction or symlink for `out\obj` is not required for a clone. If you use one, point it at `out\obj` yourself; do not commit it.
