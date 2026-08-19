# Auto Core library

## Configuration

`auto_core.ini` provides the shared, configuration-neutral INI parser. It
does not know which files or settings exist; the component that owns a setting
chooses the file, section, key, validation rules, and default value.

INI documents use this structure:

```ini
[section]
key = value
```

Section and key names are case-sensitive. Surrounding whitespace is ignored,
blank lines and lines beginning with `;` or `#` are ignored, and a value may
contain additional `=` characters. Empty values are distinct from missing
keys. Malformed settings and settings outside a section are ignored. If a key
is repeated in a section, the last value wins.

Configuration ownership is intentionally local:

- `auto_core.logging.config` owns DLL-wide logger settings from `[logger]` in
  `logger.ini`.
- The main executable owns its program title, journal clock, keymap mode, and
  taskbar settings.
- Optional components such as the server and iTunes own and parse their own
  settings.

Logger defaults keep logging enabled, console forwarding disabled, and use
`ac::paths::log_directory()`. A configured relative `directory` is resolved
against the executable directory. Logger settings are loaded on first use and
remain fixed for the process lifetime.

## Tests

The core tests use Catch2 and currently support the `Release|x64`
configuration. From a Developer PowerShell for Visual Studio, build the DLL
and test executable with:

```powershell
msbuild core\vs\auto_core_dll.sln `
    /m `
    /t:Build `
    /p:Configuration=Release `
    /p:Platform=x64
```

The output location is configured by `build\AutoCore.props`. Run the complete
suite directly from that location:

```powershell
& "<AutoCoreBuildDir>\auto_core_tests\auto_core_tests.exe"
```

Pass a Catch2 tag to run one module's tests:

```powershell
& "<AutoCoreBuildDir>\auto_core_tests\auto_core_tests.exe" "[ini]"
```

Unit tests use the `[unit]` tag and module-specific tags such as `[ini]` and
`[logging-config]`. Windows integration tests use `[windows-integration]` and
currently cover the error, path, and pipe modules. The clock module also has
an `[integration]` test that exercises its public local-time functions.

Catch2 is vendored under `third_party\catch2`; see its `readme.md` for the
pinned version and upstream source.
