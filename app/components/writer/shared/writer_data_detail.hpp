/**
 * \file writer_data_detail.hpp
 * \brief Resolution of the Writer directory and its application-data stores.
 *
 * `session_prompts.list`, `task_list.txt`, and the notes directory are
 * written only by `writer_editor.exe`. Seed text lives in `defaults.ixx`.
 * A missing notes directory is not part of the required baseline: declining
 * it during `--init` is success.
 */
#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace writer::data {

    struct Layout {
        std::filesystem::path directory;
        std::filesystem::path session_prompts;
        std::filesystem::path task_list;
        std::filesystem::path notes;
        std::string notes_subdirectory;
    };

    [[nodiscard]] bool is_relative_subdirectory(
        const std::filesystem::path& path
    );

    /**
     * \brief Resolves Writer application data against the installation root.
     *
     * A relative `directory` joins `installation_root`. An absolute path is
     * used as-is. A missing or empty directory uses
     * `<installation_root>/writer`. `notes` must be a relative subdirectory
     * of the Writer directory. A missing, empty, or non-relative notes value
     * uses `notes`. Nothing is created.
     */
    [[nodiscard]] Layout resolve_layout(
        const std::optional<std::filesystem::path>& directory,
        const std::optional<std::filesystem::path>& notes,
        std::string_view notes_label,
        const std::filesystem::path& installation_root
    );

    /**
     * \brief True when `session_prompts.list` or `task_list.txt` is absent.
     *
     * The notes directory is ignored. An unreadable path counts as missing
     * so a later initialization can retry it.
     */
    [[nodiscard]] bool baseline_incomplete(
        const std::filesystem::path& writer_directory
    );

} // namespace writer::data
