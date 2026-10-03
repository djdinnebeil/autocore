#include "writer_data_detail.hpp"

namespace writer::data {

    namespace {

        [[nodiscard]] std::filesystem::path normalized_directory(
            const std::optional<std::filesystem::path>& configured,
            const std::filesystem::path& fallback,
            const std::filesystem::path& installation_root
        ) {
            if (!configured || configured->empty()) {
                return fallback;
            }

            try {
                std::filesystem::path resolved {*configured};
                if (resolved.is_relative()) {
                    resolved = installation_root / resolved;
                }
                return resolved.lexically_normal();
            }
            catch (...) {
                return fallback;
            }
        }

        [[nodiscard]] bool store_missing(const std::filesystem::path& path) {
            std::error_code error;
            const bool present = std::filesystem::exists(path, error);
            return error || !present;
        }

    } // namespace

    bool is_relative_subdirectory(const std::filesystem::path& path) {
        return !path.empty() &&
            !path.has_root_name() &&
            !path.has_root_directory();
    }

    Layout resolve_layout(
        const std::optional<std::filesystem::path>& directory,
        const std::optional<std::filesystem::path>& notes,
        const std::string_view notes_label,
        const std::filesystem::path& installation_root
    ) {
        const auto writer_directory = normalized_directory(
            directory,
            (installation_root / "writer").lexically_normal(),
            installation_root
        );

        const bool notes_relative =
            notes && is_relative_subdirectory(*notes) && !notes_label.empty();
        const std::string subdirectory = notes_relative
            ? std::string {notes_label}
            : std::string {"notes"};
        const auto notes_directory = notes_relative
            ? (writer_directory / *notes).lexically_normal()
            : (writer_directory / "notes").lexically_normal();

        return Layout {
            writer_directory,
            writer_directory / "session_prompts.list",
            writer_directory / "task_list.txt",
            notes_directory,
            subdirectory
        };
    }

    bool baseline_incomplete(const std::filesystem::path& writer_directory) {
        return store_missing(writer_directory / "session_prompts.list") ||
            store_missing(writer_directory / "task_list.txt");
    }

} // namespace writer::data
