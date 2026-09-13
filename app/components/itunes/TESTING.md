# iTunes component tests

The iTunes component uses the repository's Catch2 test framework. The default
suite is self-contained: it does not launch iTunes, initialize COM, or remove
music files.

From the repository root, build and run the tests with:

```powershell
msbuild app\components\itunes\itunes_tests.vcxproj /m /p:Configuration=Release /p:Platform=x64
.\out\obj\itunes_tests\itunes_tests.exe "~[live]"
```

Run only the fast iTunes unit tests with:

```powershell
.\out\obj\itunes_tests\itunes_tests.exe "[itunes][unit]"
```

The default suite also includes `[windows-integration]` tests. These create
uniquely named local pipes and verify both numeric and named iTunes command
routing without starting Auto Core or iTunes.

The completed refactor baseline is 37 test cases with 138 assertions. Coverage
includes:

- configuration resolution and malformed-value behavior;
- queue and track formatting;
- history recording and draining;
- stable protocol values and command names;
- named-command registry construction and dispatch;
- playback, removal, and retry coordination through fakes;
- dedicated serial-executor ownership, nesting, failure propagation, and
  stopping;
- numeric and named command routing through real local Windows pipes.

Tests that require the installed iTunes application must use the `[live]` tag.
Normal automated runs must exclude them with `~[live]`. A live test must state
whether it starts iTunes, changes playback, modifies the library, or touches a
media file.

The fake removal tests verify coordination and status reporting only. They do
not invoke iTunes `Delete` or move a real file to the Recycle Bin. Real deletion
coverage is deferred to the opt-in live harness and must use recoverable test
media.

The component architecture, command behavior, deletion caveats, and current
Windows privilege limitation are documented in `docs/itunes.md`.

Test cases should describe observable behavior, remain independent of execution
order, and use `REQUIRE` for prerequisites and `CHECK` for expected results.
