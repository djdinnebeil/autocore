/**
 * \file writer_notepad.ixx
 * \brief Open Writer text files in Notepad.
 */
export module writer_notepad;

import std;
import auto_core.core.component;

export namespace writer_notepad {

    [[nodiscard]]
    bool open_in_notepad(
        ac::Component& component,
        const std::filesystem::path& path
    );

    [[nodiscard]]
    bool create_or_open_daily_note(ac::Component& component);

} // namespace writer_notepad
