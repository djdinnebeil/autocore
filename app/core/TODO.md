# Auto Core DLL follow-up work

This file records improvements that are intentionally deferred and do not
block completion of the current DLL version.

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
