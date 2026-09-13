# Auto Core DLL follow-up work

This file records improvements that are intentionally deferred and do not
block completion of the current DLL version. Product-level follow-ups are
tracked in [docs/TODO.md](../../docs/TODO.md).

## Reap completed taskbar publisher threads

**Status:** Deferred performance hardening

The taskbar authority creates one publisher thread for each connected client.
When a client disconnects, the thread finishes but its `std::thread` object
remains in `publisher_threads` until `stop_authority()` joins and clears the
collection. Normal Auto Core operation has a small, stable number of clients,
so this does not block the current phase. A long-running authority exposed to
repeated component reconnects can nevertheless accumulate finished thread
objects and their bookkeeping memory.

Future work should reap completed publishers during normal operation or move
client publication to a design with bounded worker ownership. The shutdown
path must continue to join every joinable thread, and snapshot publication
must remain safe while clients connect, disconnect, or stop concurrently.

### Acceptance criteria

- Completed publisher threads are reclaimed before authority shutdown.
- The number of retained worker objects is bounded by active clients plus a
  small constant.
- Refresh publication and shutdown remain race-free under repeated client
  connect/disconnect cycles.
- A stress test covers many sequential reconnects and concurrent refreshes.

## Cheapen taskbar `EnumWindows` matching

**Status:** Deferred investigation (benchmark before changing)

Activate and cycle do not feel slower than the old Word and File Explorer
`EnumWindows` callbacks (stack `WCHAR` buffers, `CabinetWClass`, or a title
suffix). The current matcher in `window_matches` still uses `EnumWindows` but
allocates a 64 KB image-path string per PID and may call COM
`AppUserModelID` per HWND. Investigate and measure; only then consider a
behavior-preserving cheapen (stack buffers or smaller path reads).

Do not restore the two hardcoded callbacks. Those only covered Explorer and
Word. Firefox and Firefox Private both use `firefox.exe` and are split by
`[window] application_id`; skipping HWND AUMID when `process_name` is set
would merge them.

Do not skip enumeration on the emulated path (no slot 1–10). Native
`one_shot` already skips the count.

### Acceptance criteria

- Benchmark first: keypress-time `matching_window_count` / `matching_windows`
  versus current, on a busy desktop, including Firefox, Firefox Private, and
  Explorer.
- Same INI AND fields (`process_name`, `class`, `executable_path`, HWND
  `application_id`).
- `stop_after = 2` remains for native cycle prep. Native `one_shot` still
  does not enumerate.
- Manual check: Word, Explorer, Firefox versus private, and one
  emulated-past-10 activate.

## Preserve common clipboard formats

**Status:** Deferred optimization

The current clipboard snapshot API preserves `CF_UNICODETEXT` only. This is
sufficient for Auto Core's present text-insertion workflow, but it does not
preserve other data a user may commonly copy, including:

- HTML and rich text
- Images and bitmaps
- File-drop lists copied from Explorer
- Registered application formats that use movable global memory

Future work should preserve a practical set of common formats rather than
promise support for every Windows clipboard format. Some formats use GDI
handles, private ownership rules, OLE data objects, or delayed rendering and
cannot be copied safely by treating every clipboard handle as an `HGLOBAL`.

### Suggested implementation order

Replacement Unicode memory is already allocated and populated before the
clipboard is opened and emptied. This keeps allocation and memory-locking
failures from disturbing the user's existing clipboard contents.

1. Use `GetClipboardSequenceNumber` to avoid restoring an old snapshot after
   the user or another application has copied new content.
2. Introduce a multi-format snapshot type and preserve common `HGLOBAL`-backed
   formats, with explicit size limits and ownership-safe RAII cleanup.
3. Add dedicated handling and tests for file lists and bitmap/image formats.
4. Consider OLE `IDataObject` preservation only if delayed-rendered or virtual
   file content becomes a demonstrated requirement.

### Acceptance criteria

- Unicode text continues to round-trip without changing current callers.
- HTML/rich-text clipboard content remains usable after Auto Core insertion.
- Copied images and Explorer file lists survive insertion.
- Auto Core does not overwrite clipboard content copied by the user while a
  paste is in progress.
- Unsupported formats are detected and handled deliberately rather than being
  silently treated as an empty clipboard.
- Partial capture or restoration failures do not leak handles or leave the
  clipboard empty when replacement data has not been prepared successfully.

### Current user-facing behavior

`Component::insert_text_preserving_clipboard_text` preserves and restores the
clipboard's Unicode-text representation, even when HTML, rich text, or other
application formats are also present. Legacy text is converted to Unicode.
Copied Explorer files and directories are restored as newline-separated full
paths after the requested text is inserted. Unsupported non-text content is
restored as a newline. Detailed conversion information is reported through
Component, while the active field receives the requested text. Empty requested
text produces a newline. This operation never prompts.

These fallbacks deliberately provide a visible completion signal but do not
preserve the original file, image, rich-text, or application clipboard object.
Explicit replacement operations leave the inserted text on the clipboard.

`Component::print_and_insert` uses replacement behavior by default and never
prompts. Callers must select an explicitly named preserving operation when the
previous clipboard contents are expected to survive.
