# Adding a new component

Create a generic host child from **File → New → Project**. After this workflow, Main discovers the child from a list name. You do not rebuild `auto_core.exe` or `auto_core.dll`.

This guide uses:

`example` → `example_ac.exe` → `ac_example_pipe` → `print_example`

Do not use `logger`, `dash`, or `slash` as a new v1 list name (those are known specials). Production shape is **Release | x64**. Prerequisites are in [building.md](building.md): Visual Studio 2026 (18+) / MSVC v145, Desktop development with C++, Windows 11, Windows PowerShell 5.1+ for the post-Link DLL copy. Those are toolchain requirements, not filesystem paths to type into the project.

Each heading is tagged **source**, **compile**, or **runtime configuration**.

## Project creation (source)

1. File → New → Project.
2. Console App (C++, Windows, Console). Not CMake.
3. **Name:** `example` (lowercase).
4. **Location:** `<repo>\app\components` (the clone path is yours; do not paste someone else's checkout into the `.vcxproj`).
5. Place solution and project in the same directory.
6. Application type: Console. Precompiled header: **None**.

There is no root `.sln`. The wizard may create `example.sln` next to `example.vcxproj`. That file is only for opening this child in Visual Studio. It is not a contribution to a central solution.

Delete the wizard `example.cpp` (and any `pch` files). Auto Core sources are `.cxx`.

## Release | x64 (compile)

Toolbar: **Release** and **x64**. Set C/C++ and Linker options on that configuration. Debug / Win32 can remain unused.

## Import AutoCore.props (compile)

`msbuild/AutoCore.props` owns include, library, `dist\`, and `obj\` paths from the repository root. Do not type Additional Include Directories, Additional Library Directories, Output Directory, or Intermediate Directory by hand. Do not edit `AutoCore.props`.

### Property Manager sequence

1. View → Other Windows → **Property Manager**.
2. Expand `example` → **Release | x64**.
3. Right-click **Release | x64** → **Add Existing Property Sheet**.
4. Choose `<repo>\msbuild\AutoCore.props`.
5. **Save the project.**
6. **Inspect `example.vcxproj`** in a text editor (or unload the project and open the XML).
7. Ensure there is **exactly one** intended `AutoCore.props` import.
8. The committed form must be:

```xml
<Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
<ImportGroup Label="Shared">
  <Import Project="..\..\..\msbuild\AutoCore.props" />
</ImportGroup>
```

Property Manager often writes an absolute `Import Project="C:\…\msbuild\AutoCore.props"` under a per-configuration `PropertySheets` group. If that line exists, delete it. Do not keep both an absolute per-config import and the relative Shared import.

`$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props` imports (exists-guarded) are Visual Studio defaults. Leave them. They are not Auto Core paths.

After a correct import, General → Output Directory is repo `dist\` and Intermediate Directory is repo `obj\example\`.

## Required project properties (compile)

Configuration Properties → **General** (Release | x64):

| Setting | Value |
| --- | --- |
| Configuration Type | Application (`.exe`) |
| Platform Toolset | **v145** |
| Character Set | Unicode |
| **Target Name** | `example_ac` |
| Whole Program Optimization | Use Link Time Code Generation |

**C++ Language Standard:** ISO C++23 (`/std:c++latest`). **C Language Standard:** latest (`/std:clatest`).

If a vcpkg page exists: App-local dependencies = **No**.

## Required C/C++ settings (compile)

**C/C++ → General** (Release | x64):

| Setting | Value |
| --- | --- |
| Warning Level | Level3 |
| SDL checks | Yes |
| **Scan Sources for Module Dependencies** | **Yes** |
| Enable C++ Modules | No |
| Build ISO C++23 Standard Library Modules | No |

Without Scan Sources for Module Dependencies, `import <Windows.h>` fails with C7612.

**C/C++ → Preprocessor:** `NDEBUG;_CONSOLE;%(PreprocessorDefinitions)`

**C/C++ → Language:** Conformance mode Yes (`/permissive-`).

**C/C++ → Code Generation:** Runtime Library **Multi-threaded DLL (`/MD`)**.

Do not add include directories. Props already supplies `app\core\include` and vendor headers.

## Linker settings (compile)

**Linker → Input → Additional Dependencies:**

```text
auto_core.lib;%(AdditionalDependencies)
```

Add vendor `.lib` names only if this exe calls those libraries. A v1 smoke child needs only `auto_core.lib`.

**Linker → System:** SubSystem **Console**.

**Linker → Debugging:** Generate Debug Information Yes. Generate Program Database File: `$(AutoCoreSymbolsDir)$(TargetName).pdb`.

Do not add library directories. Props already searches `lib\` and vendor libs. Do not add a Project Reference from Main to this project.

## Shared modules and resources (source)

Add → Existing Item. These must compile (`ClCompile` / `ResourceCompile`). Paths are relative to `app\components\example\`.

**Your file**

- `main.cxx`

**Protocol / registry**

- `..\..\shared\command_registry.ixx`
- `..\..\shared\protocols\component_protocol.ixx`

**Core module interfaces** (MSVC only builds `.ixx` files listed in this project; do not add `app\core\src\*.cxx`)

- `..\..\core\modules\clipboard.ixx`
- `..\..\core\modules\clock.ixx`
- `..\..\core\modules\component.ixx`
- `..\..\core\modules\encoding.ixx`
- `..\..\core\modules\error.ixx`
- `..\..\core\modules\formatting.ixx`
- `..\..\core\modules\ini.ixx`
- `..\..\core\modules\keyboard.ixx`
- `..\..\core\modules\log_protocol.ixx`
- `..\..\core\modules\logging_config.ixx`
- `..\..\core\modules\paths.ixx`
- `..\..\core\modules\pipes.ixx`
- `..\..\core\modules\thread.ixx`

**Resource**

- `..\..\resources\resource.rc`

## Minimum `main.cxx` (source)

The list name, `ac::Component` name, and `pipe_name(...)` argument must all be `"example"`. Advertise a **plain** command name (no `()`). Connect as the **client**; Main creates `ac_example_pipe`. Finish local setup, then send hello, then `dispatcher.process`. Unknown commands log and stay running. Shutdown request `1` stops the dispatcher.

```cpp
import std;
import auto_core.core.clock;
import auto_core.core.component;
import auto_core.core.pipes;
import command_registry;
import component_protocol;

import <Windows.h>;

namespace {

constexpr std::string_view component_name = "example";

ac::Component& example_component() {
    static ac::Component component {component_name};
    return component;
}

command_registry::Registry create_example_command_registry() {
    command_registry::Registry registry;
    registry.add("print_example", [] {
        example_component().log_and_print(
            "this is print_example() from within example_ac.exe"
        );
    });
    return registry;
}

} // namespace

int main() {
    auto registry = create_example_command_registry();
    auto& component = example_component();
    component.connect_to_logger();

    const auto pipe_name = ac::protocol::component::pipe_name(component_name);
    auto connection = ac::pipes::connect_to_pipe_server(pipe_name);
    if (!connection) {
        return 1;
    }

    ac::pipes::Pipe pipe = std::move(*connection);
    ac::pipes::CommandDispatcher dispatcher;
    dispatcher.set_command(
        ac::protocol::component::to_wire(
            ac::protocol::component::Request::invoke
        ),
        [&pipe, &registry, &dispatcher, &component] {
            const auto expression = ac::pipes::read_string(pipe);
            if (!expression) {
                dispatcher.request_stop();
                return;
            }
            auto action = registry.resolve(*expression);
            if (!action) {
                component.log_and_print("Unknown example command: {}", *expression);
                return;
            }
            action();
        }
    );
    dispatcher.set_command(
        ac::protocol::component::to_wire(
            ac::protocol::component::Request::shutdown
        ),
        [&dispatcher, &component] {
            component.log_and_log("shutdown signal received");
            dispatcher.request_stop();
        }
    );

    if (!ac::pipes::send_string(
            pipe,
            ac::protocol::component::make_hello(registry.autocomplete_values())
        )) {
        return 1;
    }
    if (!dispatcher.process(pipe)) {
        return 1;
    }
    component.log_and_log("program terminated");
    return 0;
}
```

`make_hello` advertises `termination_policy = graceful` by default. Pass
`TerminationPolicy::force_allowed` as its second argument only when Main may
use `TerminateProcess` after the shared shutdown deadline.

See [`app/components/simple_test/main.cxx`](../app/components/simple_test/main.cxx) for a complete smoke child, including optional `--generate-keymap-command-registry`.

Worker threads should enter through `ac::thread::run_with_exception_handling`.

Optional post-build (editor aid only; not required for hosting):

```bat
if not exist "$(TargetDir)keymap\components" mkdir "$(TargetDir)keymap\components"
"$(TargetPath)" --generate-keymap-command-registry "$(TargetDir)keymap\components\example.keymap_commands.txt"
```

Main rewrites `dist\keymap\keymap_commands.txt` from hello at startup.

## Build verification (compile)

1. Confirm **Release | x64**.
2. Build → Build Solution.
3. Output must be `<repo>\dist\example_ac.exe`.

If the exe lands under `app\components\example\x64\Release\`, `AutoCore.props` is missing or not the relative Shared import.

Do not rebuild Main or the DLL for this child. `scripts/build-all.ps1` is optional later wiring, not a linker edge.

## Enable the child (runtime configuration)

Edit live `dist\config\components.list` (gitignored). Auto Core does not overwrite that file once it exists:

```text
example on
```

List `logger`, `dash`, or `slash` only as those known specials, not as a new v1 child. Do not edit [`app/core/src/config_defaults.hpp`](../app/core/src/config_defaults.hpp) or [`defaults/config/components.list`](../defaults/config/components.list) unless the portable seed should include the name — that rebuilds `auto_core.dll`.

## Bind a command (runtime configuration)

Edit live `dist\keymap\bindings.ini`. Use the advertised name with **no** parentheses:

```ini
numpad_3 = {print_example, make_print_choice("42nd", true)}
```

`print_example()` looks up a factory and fails. There is no `()` normalization.

## Restart and test (runtime configuration)

1. Close Auto Core if it is running.
2. Start the **already-built** `dist\auto_core.exe`.
3. Confirm console lines `component: example` and `pipe: ac_example_pipe`.
4. Confirm `print_example` in regenerated `dist\keymap\keymap_commands.txt`.
5. Press the bound key. The child should print `this is print_example() from within example_ac.exe`.

## Portability rules

- Clone to any normal local path. Do not put that path in a committed `.vcxproj`.
- One relative import: `..\..\..\msbuild\AutoCore.props`. `AutoCore.props` derives `dist`, `obj`, `app`, `lib`, and `third_party` from `$(MSBuildThisFileDirectory)`. It does not use `$(SolutionDir)`.
- Do not set Additional Include Directories, Additional Library Directories, Output Directory, or Intermediate Directory for those trees.
- Shared `.ixx` and `resource.rc` items stay relative (`..\..\core\modules\…`).

## Git / source-control rules

**Normally commit**

- `.vcxproj`, `.vcxproj.filters`
- `main.cxx` and other source
- A `.sln` **only when this component intentionally has its own solution** (to open that child in Visual Studio). Auto Core has no root `.sln`.
- `msbuild/AutoCore.props` only if you are changing shared build policy (this workflow does not)
- Tracked samples under `defaults/` if the portable seed should mention the child (optional; rebuilds the DLL)

**Normally remain local / ignored**

- `.vs/`, `*.user`, `*.suo`
- `obj/`, `dist/`
- Live `dist\config\components.list` and `dist\keymap\bindings.ini`
- Absolute property-sheet imports
- Per-user IDE caches

Required-for-everyone settings belong in the `.vcxproj` or `AutoCore.props`, not a `.user` file.

## Common mistakes

- Leaving an absolute `C:\…\AutoCore.props` import, or **two** imports (absolute + relative).
- Target Name `example` instead of `example_ac` (host looks for `example_ac.exe` next to `auto_core.exe`).
- `print_example()` in `bindings.ini`.
- Adding `example` to `config_defaults.hpp` “so Main knows it” — that rebuilds the DLL and is not required.
- A Project Reference / dependency from `auto_core.vcxproj` to the child.
- Compiling `app\core\src\*.cxx` into the child (those live in the DLL).
- Using `logger`, `dash`, or `slash` as a new v1 project name.
- Building Debug and expecting `dist\` to match a Release host.
- Editing Additional Include/Library Directories because Output Directory still looks like `x64\Release`.

## Adding a New Auto Core Component

1. **Source.** Create `app/components/<name>/` from File → New → Project. Write `main.cxx` that speaks `ac.component.v1` and advertises plain command names.
2. **Compile.** Import `..\..\..\msbuild\AutoCore.props` once (relative). Set Target Name `<name>_ac`, C++ latest, scan for modules, `/MD`, link `auto_core.lib`. Add the shared `.ixx` files and `resource.rc`. Build **only** that project, Release | x64. Confirm `dist\<name>_ac.exe`.
3. **Runtime configuration.** Append `<name> on` to live `dist/config/components.list`. Bind advertised names in live `dist/keymap/bindings.ini` with no `()`. Restart existing `dist/auto_core.exe`. Press the key.

Auto Core knows how to host a component. It does not need to know which components exist when `auto_core.exe` is compiled.
