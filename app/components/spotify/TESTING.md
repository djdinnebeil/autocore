# Spotify component tests

The Spotify component uses the repository's Catch2 test framework. The default
suite is self-contained: it does not contact Spotify, open the Spotify desktop
application, change playback, write OAuth files, or modify the history database.

From the repository root, build and run the tests with:

```powershell
msbuild app\components\spotify\spotify_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64
.\obj\spotify_tests\spotify_tests.exe "~[live]"
```

Run only the fast contract tests with:

```powershell
.\obj\spotify_tests\spotify_tests.exe "[spotify][unit]"
```

The default suite also includes `[windows-integration]` tests. These create
uniquely named local pipes and verify numeric and named Spotify command routing
without starting Auto Core or `spotify_ac.exe`.

The current baseline is 10 test cases with 74 assertions. Coverage includes:

- stable protocol values and resource names;
- canonical runtime-command naming and uniqueness;
- successful HTTP response classification across the full `2xx` range;
- named-command registry construction and dispatch;
- numeric and named command routing through real local Windows pipes;
- unknown named commands and truncated named-command payloads.

Tests that require the Spotify service or installed desktop application must
use the `[live]` tag. Normal automated runs must exclude them with `~[live]`.
A live test must state whether it opens Spotify, changes playback, transfers an
active device, downloads album art, writes credentials or tokens, inserts text,
or modifies `spotify_history.db`.

The component contract, authorization files, command behavior, persistence,
and known limitations are documented in `docs/spotify.md`.
