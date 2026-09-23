# Third-party notices

Auto Core vendors libraries under `third_party/<dependency>/`. Product
packages use `include/`, import libraries in `lib/`, and runtime DLLs in
`bin/`. Catch2 is amalgamated test source at `third_party/catch2/`. This
file is an attribution list, not a license grant for Auto Core itself. See
[LICENSE](LICENSE) for Auto Core terms. A build copies
`third_party/*/bin/*.dll` into `dist/` next to the executables.

## Catch2

- Location: `third_party/catch2/`
- License: Boost Software License 1.0
- Full text: [`third_party/catch2/LICENSE.txt`](third_party/catch2/LICENSE.txt)

## civetweb

- Location: `third_party/civetweb/`
- License: MIT License
- https://github.com/civetweb/civetweb

## cpr

- Location: `third_party/cpr/`
- License: MIT License
- https://github.com/libcpr/cpr

## curl (libcurl)

- Location: `third_party/curl/`
- License: curl license (MIT-style)
- https://curl.se/docs/copyright.html

## nlohmann/json

- Location: `third_party/json/include/json.hpp`
- License: MIT License
- https://github.com/nlohmann/json

## SQLite

- Location: `third_party/sqlite3/`
- License: public domain
- https://www.sqlite.org/copyright.html

## TagLib

- Location: `third_party/taglib/`
- License: GNU Lesser General Public License 2.1 or Mozilla Public License 1.1
- https://taglib.org/
- If you distribute binaries that link TagLib, follow LGPL/MPL obligations
  (including offering corresponding source for TagLib).

## zlib

- Location: `third_party/zlib/`
- License: zlib License
- https://zlib.net/zlib_license.html

## dbghelp

- Location: `third_party/dbghelp/`
- `dbghelp.dll` / `dbghelp.h` are Windows SDK redistributable components.
  Use is subject to the Microsoft Software License Terms for the Windows SDK.
