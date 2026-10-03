/**
 * \file wake_history_detail.hpp
 * \brief Wake history filenames and capture apply rules.
 *
 * `current.event`, `previous.event`, and `wake_events.log` are Wake
 * operational history. `wake_config.exe` may test whether they exist.
 * Only `wake_ac.exe` applies a capture.
 */
#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

namespace wake::history {

    inline constexpr std::string_view current_event_name = "current.event";
    inline constexpr std::string_view previous_event_name = "previous.event";
    inline constexpr std::string_view events_log_name = "wake_events.log";

    enum class ApplyResult {
        unchanged,
        recorded,
        failed
    };

    /**
     * \brief True when all three canonical history files exist.
     *
     * Presence is the whole check. An empty or unreadable file still counts.
     * This function does not open the files.
     */
    [[nodiscard]] inline bool all_history_files_present(
        const std::filesystem::path& directory
    ) {
        const std::string_view names[] {
            current_event_name,
            previous_event_name,
            events_log_name
        };
        for (const std::string_view name : names) {
            std::error_code error;
            const bool present = std::filesystem::exists(
                directory / std::string {name},
                error
            );
            if (error || !present) {
                return false;
            }
        }
        return true;
    }

    /**
     * \brief Line-oriented body stored in `current.event` and `previous.event`.
     *
     * Each line keeps a trailing newline. An empty capture stays empty.
     */
    [[nodiscard]] std::string normalize_body(std::string_view captured);

    /**
     * \brief Writes a successful capture using the Wake rotation rules.
     *
     * Reads `previous.event` and `wake_events.log` before modifying
     * `current.event`. A file that exists but cannot be read fails the apply
     * and leaves the history files unchanged.
     */
    [[nodiscard]] ApplyResult apply_capture(
        const std::filesystem::path& directory,
        std::string_view captured,
        std::string_view timestamp
    );

    /**
     * \brief Creates any missing history file as an empty file.
     *
     * Existing files are not opened or replaced.
     */
    [[nodiscard]] bool create_missing_history_files(
        const std::filesystem::path& directory
    );

} // namespace wake::history
